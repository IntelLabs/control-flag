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
template <Language G>
TestResult ParseStringWithTSParser(const std::string& code_to_parse,
    size_t expected_code_blocks) {
  ManagedTSTree ts_tree;
  std::string source_file_contents;
  try {
    const bool kReportParseErrors = true;
    ts_tree = GetTSTree<G>(code_to_parse, kReportParseErrors);

    code_blocks_t code_blocks;
    code_blocks.clear();
    CollectCodeBlocksOfInterest<G>(ts_tree, code_blocks);
    if (code_blocks.size() == expected_code_blocks) {
      return TEST_SUCCESS;
    } else {
      return TEST_FAILURE;
    }
  } catch(std::exception& e) {
    return TEST_FAILURE;
  }
}

// Positive test for Verilog with 1 expression
TestResult Test1() {
  const size_t kExpectedCodeBlocks = 1;
  return ParseStringWithTSParser<LANGUAGE_VERILOG>(
  "module test;\n" \
  "reg [7:0] a_reg, b_reg;\n" \
  "reg clk;\n" \
  "always@(posedge clk) begin\n" \
  "\t\tif((a_reg > 0) && (b_reg > 0)) begin\n" \
  "\t\tend\n" \
  "\tend\n" \
  "endmodule", kExpectedCodeBlocks);
}

// Positive test for Verilog with multiple duplicate expressions
TestResult Test2() {
  const size_t kExpectedCodeBlocks = 2;
  return ParseStringWithTSParser<LANGUAGE_VERILOG>(
  "module test;\n" \
  "reg [7:0] a_reg, b_reg;\n" \
  "reg clk;\n" \
  "always@(posedge clk) begin\n" \
  "\t\tif((a_reg > 0) && (b_reg > 0)) begin\n" \
  "\t\t\t\tif((a_reg > 0) && (b_reg > 0)) begin\n" \
  "\t\t\t\tend\n" \
  "\t\tend\n" \
  "\tend\n" \
  "endmodule", kExpectedCodeBlocks);
}

// Negative test for Verilog with no conditional expressions
TestResult Test3() {
  const size_t kExpectedCodeBlocks = 0;
  return ParseStringWithTSParser<LANGUAGE_VERILOG>(
  "module test;\n" \
  "reg [7:0] a_reg, b_reg;\n" \
  "reg clk;\n" \
  "always@(posedge clk) begin\n" \
  "\tend\n" \
  "endmodule", kExpectedCodeBlocks);
}

// Positive test for C language with 1 expression
TestResult Test4() {
  const size_t kExpectedCodeBlocks = 1;
  return ParseStringWithTSParser<LANGUAGE_C>(
    "#include <stdio.h>\n"\
    "\n"\
    "int foo() {\n"\
    " int x;\n" \
    " if (x > 0) x++;\n" \
    " return x;\n"\
    "}", kExpectedCodeBlocks);
}

// Positive test for C language with 2 if-statements
TestResult Test5() {
  const size_t kExpectedCodeBlocks = 2;
  return ParseStringWithTSParser<LANGUAGE_C>(
    "#include <stdio.h>\n"\
    "\n"\
    "int foo() {\n"\
    " int x;\n" \
    " if (x > 0) x++;\n" \
    " if (x >= 100) x*=x;\n" \
    " return x;\n"\
    "}", kExpectedCodeBlocks);
}

// Negative test for C language with 0 if-statements
TestResult Test6() {
  const size_t kExpectedCodeBlocks = 0;
  return ParseStringWithTSParser<LANGUAGE_C>(
    "#include <stdio.h>\n"\
    "\n"\
    "int foo() {\n"\
    " int x;\n" \
    " while (x > 0) x++;\n" \
    " while (x >= 100) x*=x;\n" \
    " return x;\n"\
    "}", kExpectedCodeBlocks);
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
