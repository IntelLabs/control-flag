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

#include <string>

#include "tree_abstraction.h"
#include "test_common.h"

namespace {
TestResult CompactAndExpandExpression(const std::string& expression) {
  ExpressionCompacter& compacter = ExpressionCompacter::Get();
  auto compact_expression = compacter.Compact(expression);
  if (compact_expression.length() >= expression.length() ||
      expression != compacter.Expand(compact_expression)) {
    return TEST_FAILURE;
  } else {
    return TEST_SUCCESS;
  }
}

TestResult Test1() {
  return CompactAndExpandExpression("(plus (x 3))");
}

TestResult Test2() {
  return CompactAndExpandExpression("(multiply (x y))");
}

TestResult Test3() {
  return CompactAndExpandExpression("(multiply (div (x y) z))");
}

TestResult Test4() {
  return CompactAndExpandExpression("(multiply (div (x x) x))");
}

TestResult Test5() {
  return CompactAndExpandExpression("(multi_ply (div_ (x1 x1) x1))");
}

TestResult Test6() {
  return CompactAndExpandExpression("(mul##ti_ply (multiply (x1 x1) x1))");
}

TestResult Test7() {
  return CompactAndExpandExpression("(if_stmt (binary_op \">\" var num))");
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
    case 7: ReportTestResult(Test7()); break;
    default: assert(1 == 0);
  }
  return 0;
}
