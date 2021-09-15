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

#include "test_common.h"
#include "common_util.h"

namespace {
TestResult ParseStringWithTSParser(const std::string& code_to_parse,
    bool parse_error_expected) {
  ManagedTSTree ts_tree;
  try {
    const bool kReportParseErrors = true;
    ts_tree = GetTSTree<LANGUAGE_C>(code_to_parse, kReportParseErrors);
    if (parse_error_expected) {
      return TEST_FAILURE;
    } else {
      return TEST_SUCCESS;
    }
  } catch(std::exception& e) {
    if (parse_error_expected) {
      return TEST_SUCCESS;
    } else {
      return TEST_FAILURE;
    }
  }
}

// Positive test
TestResult Test1() {
  const bool kParseErrorExpected = false;
  return ParseStringWithTSParser(
    "#include <stdio.h>\n"\
    "\n"\
    "int foo() {\n"\
    " int x;\n" \
    " if (x > 0) x++;\n" \
    " return x;\n"\
    "}", kParseErrorExpected);
}

// Negative test - parse error expected
TestResult Test2() {
  const bool kParseErrorExpected = true;
  return ParseStringWithTSParser(
    "#include <stdio.h>\n"\
    "\n"\
    "int foo() {\n"\
    " int ------ x;\n" \
    " if (x > 0) x++;\n" \
    " return x;\n"\
    "}", kParseErrorExpected);
}

// Positive test to parse single if expression
TestResult Test3() {
  const bool kParseErrorExpected = false;
  return ParseStringWithTSParser(" if (x > 0);", kParseErrorExpected);
}

// Negative test to parse single if expression
TestResult Test4() {
  const bool kParseErrorExpected = true;
  return ParseStringWithTSParser(" if (x ++ 0);", kParseErrorExpected);
}

}  // anonymous namespace

int main(int argc, char* argv[]) {
  assert(argc == 2);
  switch (atoi(argv[1])) {
    case 1: ReportTestResult(Test1()); break;
    case 2: ReportTestResult(Test2()); break;
    case 3: ReportTestResult(Test3()); break;
    case 4: ReportTestResult(Test4()); break;
    default: assert(1 == 0);
  }
  return 0;
}
