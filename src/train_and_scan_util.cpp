// Copyright (c) 2021 Niranjan Hasabnis and Justin Gottschlich
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "train_and_scan_util.h"
#include "trie.h"
#include "common_util.h"
#include "parser.h"

// Explicit instantiation of templates
template int TrainAndScanUtil::ScanFile<LANGUAGE_C>(
  const std::string& test_file, std::ostream& log_file) const;
template int TrainAndScanUtil::ScanFile<LANGUAGE_VERILOG>(
  const std::string& test_file, std::ostream& log_file) const;
template int TrainAndScanUtil::ScanExpression<LANGUAGE_C>(
  const std::string& expression, std::ostream& log_file) const;
template int TrainAndScanUtil::ScanExpression<LANGUAGE_VERILOG>(
  const std::string& expression, std::ostream& log_file) const;

template <TreeLevel L, Language G>
void TrainAndScanUtil::ReportPossibleCorrections(const Trie& trie,
    const std::string& code_block_str,
    std::ostream& log_file) const {
  // We maintain different expression cacne per level since
  // there is no sharing of expressions between different
  // trie levels.
  static NearestExpressionsCache<L> expression_cache(scan_config_);

  // Search for nearest expressions based on edit distance.
  NearestExpressions nearest_expressions;

  // Lookup in cache. If that fails, insert into cache.
  if (expression_cache.LookUp(code_block_str,
                              nearest_expressions) == false) {
    Timer timer_trie_search;
    timer_trie_search.StartTimer();
    nearest_expressions = trie.SearchNearestExpressions(
          code_block_str, scan_config_.max_cost_,
          scan_config_.num_threads_);
    timer_trie_search.StopTimer();

    if (scan_config_.log_level_ >= LogLevel::DEBUG) {
      log_file << "Autocorrect search took "
               << timer_trie_search.TimerDiff() << " secs" << std::endl;
    }
    // Sort and rank results based on distance and occurrence.
    trie.SortAndRankResults(nearest_expressions);

    // Select only max results asked by the user.
    if (nearest_expressions.size() > scan_config_.max_autocorrections_)
      nearest_expressions.resize(scan_config_.max_autocorrections_);

    expression_cache.Insert(code_block_str, nearest_expressions);
  }

  auto print_autocorrect_results = [&]() {
    for (auto nearest_expression : nearest_expressions) {
      log_file << "Did you mean:" << nearest_expression.GetExpression()
               << " with editing cost:" << nearest_expression.GetCost()
               << " and occurrences: " << nearest_expression.GetNumOccurrences()
               << std::endl;
    }
    log_file << std::endl;
  };

  if (trie.IsPotentialAnomaly(nearest_expressions,
                              scan_config_.anomaly_threshold_)) {
    log_file << "Expression is Potential anomaly" << std::endl;
    print_autocorrect_results();
  } else {
    log_file << "Expression is Okay" << std::endl;
    if (scan_config_.log_level_ >= LogLevel::INFO) {
      print_autocorrect_results();
    }
  }
}

template <TreeLevel L, Language G>
bool TrainAndScanUtil::ScanExpressionForAnomaly(const Trie& trie,
    const code_block_t& code_block,
    std::ostream& log_file) const {
  float confidence = 0.0;
  size_t num_occurrences = 0;
  std::string code_block_str = NodeToString<L, G>(
                                            code_block);
  bool found_in_training_dataset = trie.LookUp(code_block_str,
                                          num_occurrences, confidence);

  // Pretty print
  auto print_details = [&](const std::string& status) {
    try {
      log_file << "Level:" << LevelToString<L>()
             << " Expression:" << code_block_str
             << " " << status
             << " in training dataset: ";
    } catch(std::exception& e) {
      // NodeToString can throw exception. Just skip printing in that case.
    }
  };
  print_details(found_in_training_dataset ? "found" : "not found");

  // Suggest expressions that are close to current expression.
  ReportPossibleCorrections<L, G>(trie, code_block_str, log_file);
  return found_in_training_dataset;
}

template <TreeLevel L, Language G>
bool TrainAndScanUtil::ScanExpressionForAnomaly(const Trie& trie,
    const std::string& source_file_contents,
    const code_block_t& code_block,
    std::ostream& log_file, const std::string& test_file) const {
  float confidence = 0.0;
  size_t num_occurrences = 0;
  std::string code_block_str = NodeToString<L, G>(code_block);
  bool found_in_training_dataset = trie.LookUp(code_block_str,
                                               num_occurrences, confidence);

  // Pretty print
  auto print_details = [&](const std::string& status) {
    log_file << "Level:" << LevelToString<L>()
             << " Expression:" << code_block_str
             << " " << status
             << " in training dataset: ";

    if (test_file != "" && source_file_contents != "") {
      TSPoint start = ts_node_start_point(code_block);
      log_file << "Source file: " << test_file << ":"
               << start.row << ":" << start.column << ":";
      log_file << OriginalSourceExpression(code_block,
                   source_file_contents) << std::endl;
    }
  };
  print_details(found_in_training_dataset ? "found" : "not found");

  // Suggest expressions that are close to current expression.
  ReportPossibleCorrections<L, G>(trie, code_block_str, log_file);
  return found_in_training_dataset;
}

template <Language G>
int TrainAndScanUtil::ScanExpression(const std::string& expression,
    std::ostream& log_file) const {
  ManagedTSTree ts_tree;
  try {
    static bool kReportParseErrors = true;
    ts_tree = GetTSTree<G>(expression, kReportParseErrors);
  } catch (std::exception& e) {
    log_file << "Error: " << e.what() << " ... skipping" << std::endl;
    return 0;
  }
  code_blocks_t code_blocks;
  CollectCodeBlocksOfInterest<G>(ts_tree, code_blocks);
  if (code_blocks.size() == 0) {
    log_file << "Error: No control structures (e.g., if statement) "
             << "found in the input" << std::endl;
    return 0;
  }

  for (auto expression : code_blocks) {
     ScanExpressionForAnomaly<LEVEL_ONE, G>(trie_level1_, "", expression,
                                            log_file, "");
  }
  return 0;
}

template <Language G>
int TrainAndScanUtil::ScanFile(const std::string& test_file,
    std::ostream& log_file) const {
  // Test it on if statements from evaluation source file.a
  ManagedTSTree ts_tree;
  std::string source_file_contents;
  try {
    ts_tree = GetTSTree<G>(test_file, source_file_contents);
  } catch (std::exception& e) {
    log_file << "Error:" << e.what() << " ... skipping" << std::endl;
    return 0;
  }

  code_blocks_t code_blocks;
  CollectCodeBlocksOfInterest<G>(ts_tree, code_blocks);

  size_t num_expressions_found = 0;
  size_t num_expressions_not_found = 0;
  size_t num_total_expressions = 0;
  size_t level1_hit = 0, level1_miss = 0;
  size_t level2_hit = 0, level2_miss = 0;

  for (auto code_block : code_blocks) {
    bool is_level1_hit = ScanExpressionForAnomaly<LEVEL_ONE, G>(trie_level1_,
                          source_file_contents, code_block, log_file,
                          test_file);
    bool is_level2_hit = ScanExpressionForAnomaly<LEVEL_TWO, G>(trie_level2_,
                          source_file_contents, code_block, log_file,
                          test_file);
    if (is_level1_hit) {
      level1_hit++;
    } else {
      level1_miss++;
    }
    if (is_level2_hit) {
      level2_hit++;
    } else {
      level2_miss++;
    }
    if (is_level1_hit || is_level2_hit) {
      num_expressions_found++;
    } else {
      num_expressions_not_found++;
    }
    num_total_expressions++;
  }

  if (scan_config_.log_level_ >= LogLevel::DEBUG) {
    log_file  << "SUMMARY " << test_file
              << ":Total/Found/Not_found/L1_hit/L1_miss/L2_hit/L2_miss="
              << num_total_expressions << ","
              << num_expressions_found << ","
              << num_expressions_not_found << ","
              << level1_hit << "," << level1_miss << ","
              << level2_hit << "," << level2_miss
              << std::endl;
  }

  return 0;
}

int TrainAndScanUtil::ReadTrainingDatasetFromFile(
      const std::string& train_dataset,
      std::ostream& log_file) {
  log_file << "Training: start." << std::endl;

  timer_trie_build_level1_.StartTimer();
  trie_level1_.Build<LEVEL_ONE>(train_dataset);
  timer_trie_build_level1_.StopTimer();
  log_file  << "Trie L1 build took: "
            << timer_trie_build_level1_.TimerDiff() << "s" << std::endl;

  timer_trie_build_level2_.StartTimer();
  trie_level2_.Build<LEVEL_TWO>(train_dataset);
  timer_trie_build_level2_.StopTimer();
  log_file << "Trie L2 build took: "
            << timer_trie_build_level2_.TimerDiff() << "s" << std::endl;

  log_file << "Training: complete." << std::endl;
  return 0;
}
