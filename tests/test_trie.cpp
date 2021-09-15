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

#include <unistd.h>
#include <string>

#include "trie.h"
#include "test_common.h"
#include "common_util.h"

namespace {
template <TreeLevel L>
TestResult BuildTrie(const std::string& training_data, Trie& trie) {
  try {
    char temporary_file[] = "/tmp/test_trie_build.XXXXXX";
    if (mkstemp(temporary_file) == -1)
      return TEST_FAILURE;

    std::ofstream stream(temporary_file);
    if (!stream.is_open())
      return TEST_FAILURE;
    stream << training_data;
    stream.close();

    trie.Build<L>(temporary_file);
    remove(temporary_file);
    return TEST_SUCCESS;
  } catch(std::exception& e) {
    return TEST_FAILURE;
  }
}

// Positive tests
TestResult Test1() {
  Trie trie;
  if (BuildTrie<LEVEL_ONE>(
    "//if (x > y)\n" \
    "0,AST_expression_ONE:(ifstmt (\">\")(var (x))(var (y)))\n" \
    "//if (x != y)\n" \
    "0,AST_expression_ONE:(ifstmt (\"!=\")(var (x))(var (y)))\n" \
    , trie) == TEST_SUCCESS)
    return TEST_SUCCESS;
  else
    return TEST_FAILURE;
}

TestResult Test2() {
  Trie trie;
  if (BuildTrie<LEVEL_TWO>(
    "//if (x > y)\n" \
    "0,AST_expression_TWO:(ifstmt (\">\")(var (x))(var (y)))\n" \
    "//if (x != y)\n" \
    "0,AST_expression_TWO:(ifstmt (\"!=\")(var (x))(var (y)))\n" \
    , trie) == TEST_SUCCESS)
    return TEST_SUCCESS;
  else
    return TEST_FAILURE;
}

// Negative test for trie build: ill-formatted training data.
TestResult Test3() {
  Trie trie;
  if (BuildTrie<LEVEL_ONE>(
    "//if (x > y)\n" \
    "0,ASTession_:(ifstmt (\">\")(var (x))(var (y)))\n" \
    "//if (x != y)\n" \
    "0,ASTession_:(ifstmt (\"!=\")(var (x))(var (y)))\n" \
    , trie) == TEST_FAILURE)
    return TEST_SUCCESS;
  else
    return TEST_FAILURE;
}

// Test for lookup in trie
TestResult Test4() {
  Trie trie;
  if (BuildTrie<LEVEL_ONE>(
    "//if (x > y)\n" \
    "0,AST_expression_ONE:(ifstmt (\">\")(var (x))(var (y)))\n" \
    "//if (x != y)\n" \
    "0,AST_expression_ONE:(ifstmt (\"!=\")(var (x))(var (y)))\n" \
    , trie) == TEST_FAILURE)
    return TEST_FAILURE;

  size_t num_occurrences = 0; float confidence = 0.0;
  bool found = trie.LookUp("(ifstmt (\">\")(var (x))(var (y)))",
                            num_occurrences, confidence);

  if (found == false || num_occurrences != 1)
    return TEST_FAILURE;
  else
    return TEST_SUCCESS;
}

// Negative test for lookup in trie
TestResult Test5() {
  Trie trie;
  if (BuildTrie<LEVEL_ONE>(
    "//if (x > y)\n" \
    "0,AST_expression_ONE:(ifstmt (\">\")(var (x))(var (y)))\n" \
    "//if (x != y)\n" \
    "0,AST_expression_ONE:(ifstmt (\"!=\")(var (x))(var (y)))\n" \
    , trie) == TEST_FAILURE)
    return TEST_FAILURE;

  size_t num_occurrences = 0; float confidence = 0.0;
  bool found = trie.LookUp("(while_stmt (\">\")(var (x))(var (y)))",
                            num_occurrences, confidence);
  if (found == true)
    return TEST_FAILURE;
  else
    return TEST_SUCCESS;
}

// Positive test for nearest expressions, ranking and anomaly detection
TestResult Test6() {
  Trie trie;
  if (BuildTrie<LEVEL_ONE>(
    "//if (x = y)\n" \
    "0,AST_expression_ONE:(ifstmt (\"=\")(var (x))(var (y)))\n" \
    "//if (x == y)\n" \
    "0,AST_expression_ONE:(ifstmt (\"==\")(var (x))(var (y)))\n" \
    "//if (x == y)\n" \
    "0,AST_expression_ONE:(ifstmt (\"==\")(var (x))(var (y)))\n" \
    "//if (x == y)\n" \
    "0,AST_expression_ONE:(ifstmt (\"==\")(var (x))(var (y)))\n" \
    "//if (x == y)\n" \
    "0,AST_expression_ONE:(ifstmt (\"==\")(var (x))(var (y)))\n" \
    "//if (x ++ y)\n" \
    "0,AST_expression_ONE:(ifstmt (\"++\")(var (x))(var (y)))\n" \
    "//if (x ++ y)\n" \
    "0,AST_expression_ONE:(ifstmt (\"++\")(var (x))(var (y)))\n" \
    "//if (x ++ y)\n" \
    "0,AST_expression_ONE:(ifstmt (\"++\")(var (x))(var (y)))\n" \
    "//if (x -- y)\n" \
    "0,AST_expression_ONE:(ifstmt (\"--\")(var (x))(var (y)))\n" \
    "//if (x -- y)\n" \
    "0,AST_expression_ONE:(ifstmt (\"--\")(var (x))(var (y)))\n" \
    "//if (x -- y)\n" \
    "0,AST_expression_ONE:(ifstmt (\"--\")(var (x))(var (y)))\n" \
    , trie) == TEST_FAILURE)
    return TEST_FAILURE;

  NearestExpression::Cost kMaxCost = 1;
  size_t kNumThreads = 1;
  auto nearest_expressions = trie.SearchNearestExpressions(
                              "(ifstmt (\"=\")(var (x))(var (y)))",
                              kMaxCost, kNumThreads);
  trie.SortAndRankResults(nearest_expressions);

  // We expect x = y and x != y being at max 1 edit away from x = 1.
  // x ++ y and x -- y are 2 edits away.
  if (nearest_expressions.size() != 2 ||
      nearest_expressions[0].GetExpression().compare(
        "(ifstmt (\"=\")(var (x))(var (y)))") != 0 ||
      nearest_expressions[1].GetExpression().compare(
        "(ifstmt (\"==\")(var (x))(var (y)))") != 0)
    return TEST_FAILURE;

  float kAnomalyThreshold1 = 1;
  float kAnomalyThreshold50 = 50;  // "x = y" has 1 occurrence in our
  // experiment; "x == y" has 4 occurrences. If we set threshold to
  // 0.5 (i.e., 50%), then "x = y" should be anomaly.
  bool is_assign_anomaly_at_50_pct = trie.IsPotentialAnomaly(
                                     nearest_expressions, kAnomalyThreshold50);
  bool is_assign_anomaly_at_1_pct = trie.IsPotentialAnomaly(
                                     nearest_expressions, kAnomalyThreshold1);
  if (is_assign_anomaly_at_50_pct && !is_assign_anomaly_at_1_pct)
    return TEST_SUCCESS;
  return TEST_FAILURE;
}
}  // anonymous namespace

int main(int argc, char* argv[]) {
  assert(argc == 2);
  switch (atoi(argv[1])) {
    case 1: ReportTestResult(Test1()); break;
    case 2: ReportTestResult(Test2()); break;
    case 3: ReportTestResult(Test3()); break;
    case 4: ReportTestResult(Test4()); break;
    case 5: ReportTestResult(Test5()); break;
    case 6: ReportTestResult(Test6()); break;
    default: assert(1 == 0);
  }
  return 0;
}
