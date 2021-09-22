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

#ifndef SRC_TRAIN_AND_SCAN_UTIL_H_
#define SRC_TRAIN_AND_SCAN_UTIL_H_

#include <tree_sitter/api.h>

#include <iostream>
#include <string>
#include <atomic>
#include <shared_mutex>
#include <mutex>  // NOLINT [build/c++11]
#include <unordered_map>

#include "trie.h"
#include "common_util.h"

//----------------------------------------------------------------------------
// Class that provides Train and Scan functions of ControlFlag system
class TrainAndScanUtil {
 public:
  enum LogLevel {
    MIN = 0,
    ERROR = MIN,
    INFO,
    DEBUG,
    MAX = DEBUG
  };

  struct ScanConfig {
    size_t max_cost_ = 2;
    TreeLevel max_level_ = LEVEL_MAX;
    size_t max_autocorrections_ = 5;
    size_t num_threads_ = 1;
    float anomaly_threshold_ = 3;
    LogLevel log_level_ = LogLevel::ERROR;
  };

  friend class NearestExpressionCache;

  explicit TrainAndScanUtil(const ScanConfig& config) : scan_config_(config) {}

  int ReadTrainingDatasetFromFile(const std::string& train_dataset,
                                  std::ostream& log_file);
  template <Language G>
  int ScanFile(const std::string& test_file, std::ostream& log_file) const;
  template <Language G>
  int ScanExpression(const std::string& expression,
                     std::ostream& log_file) const;

 private:
  template <TreeLevel L, Language G>
  void ReportPossibleCorrections(const Trie& trie,
      const std::string& code_block_str, bool found_in_training_dataset,
      std::ostream& log_file) const;

  template <TreeLevel L, Language G>
  bool ScanExpressionForAnomaly(const Trie& trie,
      const code_block_t& code_block,
      std::ostream& log_file) const;

  template <TreeLevel L, Language G>
  bool ScanExpressionForAnomaly(const Trie& trie,
      const std::string& source_file_contents,
      const code_block_t& code_block,
      std::ostream& log_file, const std::string& test_file) const;

 private:
  Trie trie_level1_;
  Trie trie_level2_;
  Timer timer_trie_build_level1_;
  Timer timer_trie_build_level2_;

  ScanConfig scan_config_;
};

/// Cache nearest expressions for a given expression so that we
/// minimize computing nearest expressions over trie for every
/// given expression. Cache is shared by all the scanner threads.
template <TreeLevel L>
class NearestExpressionsCache {
 public:
    bool LookUp(const NearestExpression::Expression& code_block,
                NearestExpressions& nearest_expressions) {
      // shared lock for concurrent reads
      std::shared_lock lock(mutex_);
      const auto iter = cache_.find(code_block);
      if (iter == cache_.end()) {
        miss_++;
        return false;
      } else {
        nearest_expressions = iter->second;
        hit_++;
        return true;
      }
    }

    void Insert(const NearestExpression::Expression& code_block,
                NearestExpressions& nearest_expressions) {
      // unique lock for writing
      std::unique_lock lock(mutex_);
      cache_[code_block] = nearest_expressions;
    }

    explicit NearestExpressionsCache(
      const TrainAndScanUtil::ScanConfig& scan_config) :
      hit_(0), miss_(0), log_level_(scan_config.log_level_) {}
    ~NearestExpressionsCache() {
      if (log_level_ == TrainAndScanUtil::LogLevel::DEBUG)
        std::cout << "ExpressionCache statistics: hit/miss="
                  << hit_.load() << "/" << miss_.load() << std::endl;
    }

 private:
    std::shared_mutex mutex_;
    std::unordered_map<NearestExpression::Expression,
                       NearestExpressions> cache_;
    std::atomic<size_t> hit_;
    std::atomic<size_t> miss_;
    TrainAndScanUtil::LogLevel log_level_;
};

#endif  // SRC_TRAIN_AND_SCAN_UTIL_H_
