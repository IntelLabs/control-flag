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

#include "tree_abstraction.h"

// Convert binary_expression into be. Convert identifier into id.
std::string ExpressionCompacter::GetID(
    const ExpressionCompacter::Token& token) {
  std::unique_lock lock(mutex_);

  const auto it = token_id_map_.find(token);
  std::string id = "";
  if (it == token_id_map_.end()) {
    token_id_map_[token] = current_id_.load();
    id_token_map_[current_id_.load()] = token;
    id = IDToString(current_id_);
    ++current_id_;
  } else {
    id = IDToString(it->second);
  }

  return id;
}

std::string ExpressionCompacter::GetToken(const std::string& id_string) {
  std::unique_lock lock(mutex_);

  ID id = StringToID(id_string);
  auto it = id_token_map_.find(id);
  cf_assert(it != id_token_map_.end(),
            "ExpressionCompactor:Missing ID" + id_string);
  return it->second;
}

// Convert an expression such as
// "(parenthesized_expression (binary_expression ("%") (non_terminal_expression)
// (number_literal)))" into "(ID (ID ("%") (ID) (ID))): by shortening words
// and multi-words.
std::string ExpressionCompacter::Compact(const std::string& source) {
  std::string result;

  Token token = "";
  for (size_t i = 0; i < source.length(); i++) {
    char c = source[i];
    if (std::isalnum(c) || c == '_') {
      token += c;
    } else if (token.length() > 0) {
      // If c marks the end of a token, then process the token.
      result += GetID(token);
      token = "";  // reset token
      result += c;
    } else {
      result += c;
    }
  }

  if (token.length() > 0)
    result += GetID(token);

  return result;
}

// Opposite of Compact - convert IDs back into original strings
// Input would be a shortened expression like: (1 (0) (0))
std::string ExpressionCompacter::Expand(const std::string& source) {
  std::string result;

  std::string id = "";
  for (size_t i = 0; i < source.length(); i++) {
    char c = source[i];
    if (std::isdigit(c)) {
      id += c;
    } else if (id.length() > 0) {
      // if c marks an end of an ID, then map ID back into token.
      result += GetToken(id);
      id = "";  // reset ID
      result += c;  // copy c to output.
    } else {
      result += c;  // otherwise, just copy c to output.
    }
  }

  if (id.length() > 0)
    result += GetToken(id);
  return result;
}
