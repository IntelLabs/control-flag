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

#include <algorithm>
#include <vector>
#include <utility>
#include <queue>

#include "trie.h"
#include "common_util.h"
#include "tree_abstraction.h"

void Trie::Insert(const std::string& expression, size_t line_no,
                  size_t contributor_id) {
  std::string short_expr =
    ExpressionCompacter::Get().Compact(expression);
  InsertShortExpr(short_expr, line_no, contributor_id);
}

void Trie::InsertShortExpr(const std::string& short_expression, size_t line_no,
                           size_t contributor_id) {
  struct TrieNode *node = this->root_;
  for (size_t i = 0; i < short_expression.length(); i++) {
    char c = short_expression[i];
    // Increment occurrences for every node along the path
    // to keep track of number of occurrences of different prefixes.
    node->num_occurrences_++;
    auto child_iterator = node->children_.find(c);
    if (child_iterator == node->children_.end()) {
      node->children_[c] = new TrieNode(c);
      node = node->children_[c];
    } else {
      node = child_iterator->second;
    }
    // Insert this char into the alphabet list supported by this trie.
    // This info is used in auto-correction feature.
    alphabets_.insert(c);
  }
  node->num_occurrences_++;  // num occurrences for leaf nodes.
  node->terminal_node_ = true;
  if (node->pattern_contributors_.find(contributor_id) ==
      node->pattern_contributors_.end()) {
    // initialize to 0.
    node->pattern_contributors_[contributor_id] = 0;
  }
  node->pattern_contributors_[contributor_id]++;
}

bool Trie::LookUp(const std::string& expression, size_t& num_occurrences,
                  float& confidence) const {
  std::string short_expr =
    ExpressionCompacter::Get().Compact(expression);
  return LookUpShortExpr(short_expr, num_occurrences, confidence);
}

bool Trie::LookUpShortExpr(const std::string& short_expression,
                           size_t& num_occurrences,
                           float& confidence) const {
  struct TrieNode *node = root_;

  const bool kFound = true;
  for (size_t i = 0; i < short_expression.length(); i++) {
    auto child_iterator = node->children_.find(short_expression[i]);
    if (child_iterator == node->children_.end()) {
      return !kFound;
    } else {
      node = child_iterator->second;
    }
  }

  if (node->terminal_node_ == false)
    return false;

  num_occurrences = node->num_occurrences_;
  confidence = node->confidence_;
  return kFound;
}

void Trie::VisitAllLeafNodes(VisitorCallbackFn callback_fn) const {
  // DFS based tree traversal.
  using queue_element_t = std::pair<const TrieNode*, std::string>;
  std::queue<queue_element_t> q;
  std::string kRootPathPrefix = "";
  q.push(queue_element_t(root_, kRootPathPrefix));

  while (!q.empty()) {
    auto node_prefix_pair = q.front();
    q.pop();
    const TrieNode* node = node_prefix_pair.first;
    const std::string& path_prefix = node_prefix_pair.second;

    if (node->terminal_node_) {
      callback_fn(path_prefix, node->num_occurrences_,
                  node->pattern_contributors_);
    }

    for (const auto& child : node->children_) {
      q.push(queue_element_t(child.second, path_prefix + child.second->c_));
    }
  }
}

void Trie::Print(bool sorted) const {
  struct Path {
    std::string path_string_;
    size_t num_occurrences_;
    PatternContributorsMap pattern_contributors_;

    Path(std::string path_string, size_t num_occurrences,
         const PatternContributorsMap& pattern_contributors) {
      path_string_ = path_string;
      num_occurrences_ = num_occurrences;
      pattern_contributors_ = pattern_contributors;
    }
  };

  std::vector<struct Path> paths;
  VisitAllLeafNodes([&](const std::string& path_string, size_t num_occurrences,
                        const PatternContributorsMap& pattern_contributors) {
    Path path(path_string, num_occurrences, pattern_contributors);
    paths.push_back(path);
  });

  if (sorted) {
    auto sort_by_occurrences = [](Path p1, Path p2) {
      return p1.num_occurrences_ >= p2.num_occurrences_;
    };
    std::sort(paths.begin(), paths.end(), sort_by_occurrences);
  }

  for (const auto& path : paths) {
    std::cout << ExpressionCompacter::Get().Expand(path.path_string_)
              << "," << path.num_occurrences_ << ","
              << path.pattern_contributors_.size();
    for (const auto& contributor : path.pattern_contributors_) {
      std::cout << ",(" << contributor.first << ";"
                << contributor.second << ")";
    }
    std::cout << std::endl;
  }
}

void Trie::PrintEditDistancesinTrainingSet() const {
  VisitAllLeafNodes([&](const std::string& path_string, size_t num_occurrences,
                        const PatternContributorsMap& pattern_contributors) {
    const NearestExpression::Cost kMaxEditDistance = 3;
    const size_t kMaxThreads = 1;
    auto nearest_expressions = SearchNearestExpressions(path_string,
                                                        kMaxEditDistance,
                                                        kMaxThreads);
    std::cout << "Expression is: "
              << ExpressionCompacter::Get().Expand(path_string) << std::endl;
    if (IsPotentialAnomaly(nearest_expressions, 1)) {
      std::cout << "Potential anomaly" << std::endl;
    }
    for (auto nearest_expression : nearest_expressions) {
      std::string long_expression = ExpressionCompacter::Get().Expand(
            nearest_expression.GetExpression());
      std::cout << "Did you mean:" << long_expression
               << " with editing cost:" << nearest_expression.GetCost()
               << " and occurrences: " << nearest_expression.GetNumOccurrences()
               << std::endl;
    }
    std::cout << std::endl;
  });
}
