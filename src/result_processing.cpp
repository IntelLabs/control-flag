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
  // occurrences. We will now rank these results on their likelyhood of
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

bool Trie::IsPotentialAnomaly(const NearestExpressions& expressions,
                              float anomaly_threshold) const {
  // If the percentage of the number of occurrences at edit distance 0 are below
  // anomaly_threshold in comparison to the total of maximum number of
  // occurrences at other edit distances (1, 2, ..) then
  // that likely indicates a potential anomaly.

  // Generate edit distance (cost) -> max occurrences map.
  std::unordered_map<NearestExpression::Cost,
                     NearestExpression::NumOccurrences> max_occurrences_at_cost;
  for (const auto& expression : expressions) {
    auto cost = expression.GetCost();
    auto occurrences = expression.GetNumOccurrences();
    auto it = max_occurrences_at_cost.find(cost);
    if (it == max_occurrences_at_cost.end())
      max_occurrences_at_cost[cost] = occurrences;
    else
      max_occurrences_at_cost[cost] = std::max(it->second, occurrences);
  }

  // Calculate total of max occurrences at all the cost levels.
  float total_occurrences = 0;
  for (const auto& cost_occurrence : max_occurrences_at_cost) {
    // Using 1/cost+1 as a weight or probability to use max occurrences.
    // Errors that are 1 distance away are more likely than those that are 2
    // distance away.
    float weight = 1.0;  // For costs 0 and 1, we want to use full weight.
    if (cost_occurrence.first > 1)
      weight = static_cast<float>(1) / (cost_occurrence.first + 1);
    float weighted_max_occurrences = weight *
                                    static_cast<float>(cost_occurrence.second);
    total_occurrences += weighted_max_occurrences;
  }

  if (total_occurrences == 0)
    return false;

  // Calculate percentage of every cost from total cost.
  std::unordered_map<NearestExpression::Cost, float>
    percent_occurrences_at_cost;
  float lowest_percent = 100;
  for (const auto& cost_occurrence : max_occurrences_at_cost) {
    auto cost = cost_occurrence.first;
    auto occurrence = cost_occurrence.second;
    percent_occurrences_at_cost[cost] = (occurrence * 100) / total_occurrences;
    lowest_percent = std::min(lowest_percent,
                              percent_occurrences_at_cost[cost]);
  }

  const NearestExpression::Cost kZeroCost = 0;
  if (percent_occurrences_at_cost.find(kZeroCost) !=
      percent_occurrences_at_cost.end()) {
    if (percent_occurrences_at_cost[kZeroCost] < anomaly_threshold &&
        percent_occurrences_at_cost[kZeroCost] == lowest_percent)
      return true;
  }
  return false;
}
