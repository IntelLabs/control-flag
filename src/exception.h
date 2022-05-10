// Copyright (c) 2022 Niranjan Hasabnis
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

#ifndef SRC_EXCEPTION_H_
#define SRC_EXCEPTION_H_

#include <tree_sitter/api.h>
#include <exception>
#include <string>

//----------------------------------------------------------------------------
class cf_string_exception: public std::exception {
 public:
  explicit cf_string_exception(const std::string& message) :
    message_(message) {}
  virtual const char* what() const noexcept { return message_.c_str(); }
 private:
  std::string message_;
};

class cf_file_access_exception: public cf_string_exception {
 public:
  explicit cf_file_access_exception(const std::string& error) :
    cf_string_exception("File access failed: " + error) {}
};

class cf_parse_error: public cf_string_exception {
 public:
  explicit cf_parse_error(const std::string& expression) :
    cf_string_exception("Parse error in expression:" + expression) {}
};

class cf_unexpected_situation: public cf_string_exception {
 public:
  explicit cf_unexpected_situation(const std::string& error) :
    cf_string_exception("Assert failed: " + error) {}
};

inline void cf_assert(bool value, const std::string& message) {
  if (value == false) throw cf_unexpected_situation(message);
}

inline void cf_assert(bool value, const std::string& message,
                     const TSNode& node) {
  cf_assert(value, message + ts_node_string(node));
}

#endif  // SRC_EXCEPTION_H_
