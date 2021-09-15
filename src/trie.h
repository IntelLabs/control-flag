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

#ifndef SRC_TRIE_H_
#define SRC_TRIE_H_

#include <unistd.h>

#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "tree_abstraction.h"

// Data type to return nearest neighbors of a target expression based on
// some distance metric (such as Levenshtein distance). string is for
// expression, size_t for distance, size_t for num_occurrence.
struct NearestExpression {
 public:
    using Expression = std::string;
    using Cost = size_t;
    using NumOccurrences = size_t;

 public:
    NearestExpression() : expression(""), cost(0), num_occurrences(0) {}
    NearestExpression(const Expression& expression, Cost cost,
                      NumOccurrences occurrences = 1) : expression(expression),
                      cost(cost), num_occurrences(occurrences) {}

    // hash function
    size_t operator()(const NearestExpression& e) const {
      return std::hash<std::string>()(e.GetExpression());
    }

    // equality operator
    bool operator == (const NearestExpression& e) const {
      return expression.compare(e.GetExpression()) == 0;
    }

    const Expression& GetExpression() const { return expression; }
    Cost GetCost() const                    { return cost; }
    NumOccurrences GetNumOccurrences() const  { return num_occurrences; }

 private:
    /// An expression that is near to target expression
    Expression expression;
    /// Edit distance between this expression and target.
    Cost cost;
    /// Number of occurrences of this expression in training dataset
    NumOccurrences num_occurrences;
};
using NearestExpressions = std::vector<NearestExpression>;
using NearestExpressionKey = NearestExpression;
using NearestExpressionHash = NearestExpression;
using NearestExpressionSet = std::unordered_set<NearestExpressionKey,
                                                NearestExpressionHash>;
using ExpressionCombinationsAtCost = std::unordered_map<
                                            NearestExpression::Expression,
                                            NearestExpression::Cost>;
// Map of GitHub accounts and their contributions of a certain pattern
using PatternContributorsMap = std::unordered_map<size_t, size_t>;

class Trie {
 public:
  struct TrieNode {
   public:
    TrieNode(char c, size_t num_occurrences = 0, bool terminal_node = false,
              float confidence = 0) : c_(c), num_occurrences_(num_occurrences),
              terminal_node_(terminal_node), confidence_(confidence) {
    }

    ~TrieNode() {
      for (auto& child : children_) {
        delete child.second;
      }
    }

    char c_;
    size_t num_occurrences_;
    // Needed because internal nodes could be terminal nodes in trie.
    bool terminal_node_;
    float confidence_;
    std::unordered_map<char, struct TrieNode*> children_;
    PatternContributorsMap pattern_contributors_;
  };

  Trie() : root_(new TrieNode(' ', 0, false, 0)) {}
  Trie(const Trie& trie) = delete;
  Trie& operator= (const Trie& trie) = delete;
  ~Trie() { delete root_; }

  template <TreeLevel L>
  void Build(const std::string& train_dataset) {
    std::ifstream stream(train_dataset.c_str());
    if (!stream.is_open()) {
      throw cf_file_access_exception("Open failed:" + train_dataset);
    }

    // We expect line to contain <github_id>,AST_expression_<LEVEL>:.
    std::string ast_expression_pattern = "AST_expression_";
    ast_expression_pattern += LevelToString<L>();
    ast_expression_pattern += ":";

    auto is_ast_expression = [&]
        (const std::string& line, size_t& comma_pos) -> bool {
      if (line.length() < ast_expression_pattern.length())
        return false;

      comma_pos = line.find_first_of(",");
      if (comma_pos == std::string::npos) return false;

      for (size_t i = 0; i < ast_expression_pattern.length(); i++)
        if (line[comma_pos + 1 + i] != ast_expression_pattern[i])
          return false;
      return true;
    };

    std::string line;
    size_t line_no = 1;
    while (std::getline(stream, line)) {
      // We include original C expressions as comment for AST expression.
      // We do not want to insert C expressions in trie, just AST expressions.
      size_t comma_pos = 0;
      if (is_ast_expression(line, comma_pos)) {
        std::string contributor_id_str = line.substr(0, comma_pos);
        size_t contributor_id =
          std::strtoul(contributor_id_str.c_str(), NULL, 10);
        Insert(line.substr(comma_pos + 1 + ast_expression_pattern.length(),
                           std::string::npos), line_no, contributor_id);
      }
      line_no++;
    }

    // Generate a list of all trie paths for parallel exploration of trie for
    // determining nearest neighbors in autocorrect.
    GenerateListOfAllTriePaths();
    cf_assert(all_trie_paths.size() > 0,
              "Invalid training data found: content does not look " \
              "generated by ControlFlag utility");
  }

  bool LookUp(const std::string& str, size_t& num_occurrences,
              float& confidence) const;

  void Print(bool sorted = false) const;
  void PrintEditDistancesinTrainingSet() const;

  // Find expressions that are "nearest" to the input expression within the
  // specified cost.
  NearestExpressions SearchNearestExpressions(
                  const NearestExpression::Expression& target_expression,
                  NearestExpression::Cost max_cost, size_t num_threads) const;
  // Sorts nearest possible expressions based on edit distance and
  // number of occurrences (ranking criteria).
  void SortAndRankResults(NearestExpressions& nearest_expressions) const;

  // Is base expression (at edit distance 0) from nearest_expressions an
  // anomaly at the percent threshold specified in anomaly_threshold?
  bool IsPotentialAnomaly(const NearestExpressions& nearest_expressions,
                          float anomaly_threshold) const;

 private:
  // Interface function that accepts regular string/expression
  void Insert(const std::string& str, size_t line_no, size_t contributor_id);

  // Internal function that accepts compacted string/expression
  void InsertShortExpr(const std::string& str, size_t line_no, size_t
                       contributor_id);
  bool LookUpShortExpr(const std::string& str, size_t& num_occurrences,
                       float& confidence) const;

  inline void GenerateListOfAllTriePaths() {
    // We expect to call this function only once per a trie.
    if (all_trie_paths.size() == 0) {
      VisitAllLeafNodes([&](const std::string& trie_path,
          size_t num_occurrences,
          PatternContributorsMap pattern_contributors) {
        all_trie_paths.push_back(std::make_pair(trie_path, num_occurrences));
      });
    }
  }

  // Visitor that calls VisitorCallbackFn for every expression/string in trie
  using VisitorCallbackFn = std::function<void(const std::string&, size_t,
                              PatternContributorsMap)>;
  void VisitAllLeafNodes(VisitorCallbackFn fn) const;

  // Find expressions that are nearest to 'target' expression within
  // edit distance of 'max_cost' using symmetric delete edit distance
  // algorithm. This algorithm is computationally efficient than Norvig's
  // edit distance algorithm because it reduces the candidates of an
  // erroneous expression by not considering inserts/replaces/transposes.
  //
  // For more details, refer to
  // https://medium.com/@wolfgarbe/1000x-faster-spelling-correction-algorithm-2012-8701fcd87a5f
  NearestExpressions SearchNearestExpressionUsingSymmetricDelete(
    const NearestExpression::Expression& target,
    NearestExpression::Cost max_cost) const;

  // Helper function used by symmetric delete edit distance algorithm to
  // generate candidate expressions that are 'max_cost' deletes away from
  // the 'target' expression.
  void GenerateExpressionCombinationsUsingDelete(
    const NearestExpression::Expression& target,
    NearestExpression::Cost max_cost,
    ExpressionCombinationsAtCost& combinations) const;

  // Generate a set of expressions that are 1, 2.. 'max_cost' distances
  // away from target expression, and then filter out invalid expressions
  // by checking them with dictionary.
  //
  // Algorithm performs in O(N) time, where N is length of the
  // 'target_expression'. Algorithm performance does not depend on size of
  // trie/dictionary.
  NearestExpressions SearchNearestExpressionsUsingCandidateGeneration(
    const NearestExpression::Expression& target_expression,
    NearestExpression::Cost max_cost) const;
  // Generate expressions that are 'cost' distance away from target_expression
  // through replacement, deletion and insertion.
  void GenerateCandidateExpressions(
    const NearestExpression::Expression& target_expression,
    NearestExpression::Cost cost,
    NearestExpressionSet& set) const;

  // Algorithm to generate corrections of possibly mis-spelled expression
  // Algorithm goes over whole trie (in other words, training dataset) and
  // calculates levenshtein distance between valid strings from trie and
  // target expression.
  //
  // Algorithm performs in O(N) time, where N is number of words in dictionary.
  NearestExpressions SearchNearestExpressionsUsingTrieTraversal(
    const NearestExpression::Expression& target_expression,
    NearestExpression::Cost max_cost, size_t max_threads) const;

  // Calculate edit distance between source and target expressions.
  NearestExpression::Cost CalculateEditDistance(const std::string& source,
    const std::string& target) const;

 private:
  /// Root of Trie
  struct TrieNode *root_;
  /// Set of alphabets from the language supported by trie. Generated while
  /// building trie.
  std::unordered_set<char> alphabets_;

  /// Vector to store all trie paths (patterns) and their occurrences
  /// This is used to perform finding nearest expressions of a given
  /// expression in parallel.
  std::vector<std::pair<std::string, size_t>> all_trie_paths;

  std::unordered_map<NearestExpression::Expression,
                     std::unordered_set<size_t>>
    symmetric_delete_trie_combinations_;
};
#endif  // SRC_TRIE_H_
