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

#ifndef TESTS_TEST_COMMON_H_
#define TESTS_TEST_COMMON_H_

#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>

enum TestResult {
  TEST_SUCCESS = 0,
  TEST_FAILURE = 1
};

const std::unordered_map<TestResult, std::string> kResultToString({
  {TEST_SUCCESS, "Test passed"},
  {TEST_FAILURE, "Test failed"}
});

inline void ReportTestResult(TestResult result) {
  auto it = kResultToString.find(result);
  assert(it != kResultToString.end());
  std::cout << it->second;
}

#endif  // TESTS_TEST_COMMON_H_
