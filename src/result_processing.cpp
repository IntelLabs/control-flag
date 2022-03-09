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
#include "trie.h"

void Trie::SortAndRankResults(NearestExpressions& nearest_expressions) const {
  // We have N results, where each result contains a cost and number of
  // occurrences. We will now rank these results on their likelihood of
  // suggesting correct change.

  auto sort_expressions_by_score = [](const NearestExpression& e1,
                                      const NearestExpression& e2) -> bool {
    // Higher score is better.
    // return get_expression_score(e1) < get_expression_score(e2);
    // Sort by cost. If cost is same, then sort by occurrences.
    // Lower the cost is better. Higher the occurrences are better.
    return e1.GetCost() == e2.GetCost()
           ? e1.GetNumOccurrences() > e2.GetNumOccurrences()
           : e1.GetCost() < e2.GetCost();
  };

  std::sort(nearest_expressions.begin(), nearest_expressions.end(),
            sort_expressions_by_score);
}

// Expression is a potential anomaly if its occurrences at cost 0 are lesser
// than the occurrences of all the nearest expressions at other costs.
bool Trie::IsPotentialAnomaly(const NearestExpressions& expressions,
                              float anomaly_threshold) const {
  const NearestExpression::Cost kZeroCost = 0;
  NearestExpression base_expression;
  bool base_expression_found = false;

  for (const auto& expression : expressions) {
    if (expression.GetCost() == kZeroCost) {
      base_expression = expression;
      base_expression_found = true;
      break;
    }
  }

  // We should have atleast base expression and one more expression with
  // non-zero cost.
  if (base_expression_found == false ||
      expressions.size() <= 1)
    return false;

  bool exprs_with_non_zero_cost_found = false;
  for (const auto& expression : expressions) {
    if (expression.GetCost() != kZeroCost) {
      exprs_with_non_zero_cost_found = true;
      break;
    }
  }
  if (!exprs_with_non_zero_cost_found)
    return false;

  // Perform check based on anomaly threshold.
  for (const auto& expression : expressions) {
    if (expression.GetCost() == kZeroCost) continue;

    float occurrences_percent =
          (static_cast<float>(base_expression.GetNumOccurrences() * 100)) /
          static_cast<float>(expression.GetNumOccurrences());
    if (occurrences_percent > anomaly_threshold)
      return false;
  }

  return true;
}
