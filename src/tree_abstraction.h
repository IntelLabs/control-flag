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

#ifndef SRC_TREE_ABSTRACTION_H_
#define SRC_TREE_ABSTRACTION_H_

#include <tree_sitter/api.h>

#include <atomic>
#include <mutex>  // NOLINT [build/c++11]
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "common_util.h"

// Forward decl
bool IsCommentNode(const TSNode& node);

enum TreeLevel {
  LEVEL_MIN = 0,   // Basic Tree-sitter print
  LEVEL_ONE = 1,   // Same as MAX level. Diff is printing operators also.
                   // Such as == or != in addition to binary_expression.
  LEVEL_TWO = 2,   // EXPR := TERM BINARY_OP TERM | UNARY_OP TERM | *TERM |
                   //        TERM | FCALL | VAR.VAR | VAR[TERM] | anything_else
                   // TERM := VAR | CONST | anything_else
  LEVEL_MAX = 3,  // (VAR), (CONST), (EXPR)
};

inline TreeLevel VerifyTreeLevel(int tree_level) {
  if (tree_level < LEVEL_MIN)
    tree_level = LEVEL_MIN;
  if (tree_level > LEVEL_MAX)
    tree_level = LEVEL_MAX;
  return (TreeLevel) tree_level;
}

template <TreeLevel L>
inline std::string LevelToString();
template <> inline std::string LevelToString<LEVEL_MIN>() { return "MIN"; }
template <> inline std::string LevelToString<LEVEL_ONE>() { return "ONE"; }
template <> inline std::string LevelToString<LEVEL_TWO>() { return "TWO"; }
template <> inline std::string LevelToString<LEVEL_MAX>() { return "MAX"; }

//----------------------------------------------------------------------------
// A simple expression condenser

// Convert an expression like
// "(parenthesized_expression (binary_expression ("%") (non_terminal_expression)
// (number_literal)))" into
// "(ID (ID ("%") (ID) (ID))): by shortening words and multi-words by mapping
// them into unique IDs.
//
// This is useful in reducing the length of an expression for training and
// inference, and especially helps in auto-correction.
//
// We want to have single expression shortening scheme for training and
// inference.
class ExpressionCompacter {
 public:
  // Compact a full expression by mapping words into IDs
  std::string Compact(const std::string& source);

  // Opposite of Compact - convert IDs back into original strings
  // Input would be a shortened expression like: (1 (0) (0))
  std::string Expand(const std::string& source);

  // Singleton - we want to have a common shortening scheme across training and
  // multi-threaded inference.
  static ExpressionCompacter& Get() {
    static ExpressionCompacter shortner;
    return shortner;
  }

 private:
  ExpressionCompacter() : current_id_(0) {}

  using Token = std::string;
  using ID = size_t;

  inline std::string IDToString(ID id) const { return std::to_string(id); }
  inline ID StringToID(const std::string& id) const { return std::stoul(id); }

  // Get ID in string format corresponding to token.
  std::string GetID(const Token& token);
  // Get token in string format corresponding to ID given in string format.
  std::string GetToken(const std::string& id_string);

  std::shared_mutex mutex_;
  std::atomic<ID> current_id_;
  std::unordered_map<Token, ID> token_id_map_;
  std::unordered_map<ID, Token> id_token_map_;
};

// Return full string corresponding to TSNode
template <TreeLevel L, Language G>
inline std::string NodeToString(const TSNode& conditional_expression);

// Return shortened string corresponding to TSNode
template <TreeLevel L, Language G>
inline std::string NodeToShortString(const TSNode& conditional_expression) {
  std::string expression_string = NodeToString<L, G>(conditional_expression);
  return ExpressionCompacter::Get().Compact(expression_string);
}
// ---------------------------------------------------------------------------
// Basic tree-sitter print with no modification.
template <>
inline std::string NodeToString<LEVEL_MIN, LANGUAGE_C>(
    const TSNode& conditional_expression) {
  std::string condition_string;
  char* node_string = ts_node_string(conditional_expression);
  condition_string = node_string;

  // ts_node_string API mallocs memory for string that we need to free.
  free(node_string);
  return condition_string;
}

template <>
inline std::string NodeToString<LEVEL_MIN, LANGUAGE_VERILOG>(
    const TSNode& conditional_expression) {
  return NodeToString<LEVEL_MIN, LANGUAGE_C>(conditional_expression);
}

// ---------------------------------------------------------------------------

inline std::string AbstractTerminalString(const TSNode& node) {
  std::string ret;
  if (ts_node_named_child_count(node) == 0)
    ret = ts_node_type(node);
  else
    ret = "non_terminal_expression";

  return ret;
}

inline std::string AbstractBinaryExpressionString(const TSNode&
    binary_expression) {
  std::string ret;
  // Interesting enough, TreeSitter allows binary_expression to have
  // more than 2 children, by allowing comment node to be part of
  // the binary_expression children.
  cf_assert(ts_node_named_child_count(binary_expression) >= 2,
            "Binary expression has less than 2 children:",
            binary_expression);

  TSNode lhs, rhs;
  bool is_lhs_set = false, is_rhs_set = false;
  size_t num_comment_nodes = 0;
  for (uint32_t i = 0; i < ts_node_named_child_count(binary_expression); i++) {
    TSNode node = ts_node_named_child(binary_expression, i);
    if (IsCommentNode(node)) {
      num_comment_nodes++;
      continue;
    } else if (!is_lhs_set) {
      // We set lhs first because it comes first.
      lhs = node;
      is_lhs_set = true;
    } else if (!is_rhs_set) {
      rhs = node;
      is_rhs_set = true;
    }
  }
  cf_assert(is_lhs_set && is_rhs_set, "Binary expression has LHS/RHS missing:",
             binary_expression);
  cf_assert(ts_node_named_child_count(binary_expression) ==
         num_comment_nodes + 2, "Binary expression has less than 2 children:",
         binary_expression);

  ret += "(" + AbstractTerminalString(lhs) + ") ";
  ret += "(" + AbstractTerminalString(rhs) + ")";

  return ret;
}

inline std::string AbstractUnaryExpressionString(
    const TSNode& unary_expression) {
  std::string ret;
  cf_assert(ts_node_named_child_count(unary_expression) >= 1,
            "Unary expression has less than 1 children: ",
            unary_expression);

  TSNode arg;
  bool is_arg_set = false;
  size_t num_comment_nodes = 0;
  for (uint32_t i = 0; i < ts_node_named_child_count(unary_expression); i++) {
    TSNode node = ts_node_named_child(unary_expression, i);
    if (IsCommentNode(node)) {
      num_comment_nodes++;
      continue;
    } else if (!is_arg_set) {
      arg = node;
      is_arg_set = true;
    }
  }
  cf_assert(is_arg_set, "Unexpected unary expression:", unary_expression);
  cf_assert(ts_node_named_child_count(unary_expression) ==
         num_comment_nodes + 1, "Unexpected unary expression:",
         unary_expression);

  // argument
  ret += "(" + AbstractTerminalString(arg) + ")";
  return ret;
}

inline std::string AbstractSubscriptExpressionString(
    const TSNode& subscript_expression) {
  std::string ret;
  cf_assert(ts_node_named_child_count(subscript_expression) >= 2,
            "Subscript expression has less than 2 children:",
            subscript_expression);

  TSNode arg1, arg2;
  bool is_arg1_set = false, is_arg2_set = false;
  size_t num_comment_nodes = 0;
  for (uint32_t i = 0;
       i < ts_node_named_child_count(subscript_expression); i++) {
    TSNode node = ts_node_named_child(subscript_expression, i);
    if (IsCommentNode(node)) {
      num_comment_nodes++;
      continue;
    } else if (!is_arg1_set) {
      // We set arg1 first because it comes first.
      arg1 = node;
      is_arg1_set = true;
    } else if (!is_arg2_set) {
      arg2 = node;
      is_arg2_set = true;
    }
  }
  cf_assert(is_arg1_set && is_arg2_set,
            "One of the args of Subscript expression not found:",
            subscript_expression);
  cf_assert(ts_node_named_child_count(subscript_expression) ==
            num_comment_nodes + 2, "Args of Subscript expression missing:",
            subscript_expression);

  // argument
  ret += "(" + AbstractTerminalString(arg1) + ") ";
  // index
  ret += "(" + AbstractTerminalString(arg2) + ")";
  return ret;
}

inline std::string OriginalSourceExpression(
    const TSNode& node,
    const std::string& source_file_contents) {
  size_t start_byte = ts_node_start_byte(node);
  size_t end_byte =  ts_node_end_byte(node);
  return source_file_contents.substr(start_byte, end_byte - start_byte);
}

inline std::string OpToString(const TSNode& node) {
  const std::string& operator_str = "operator";
  TSNode op = ts_node_child_by_field_name(node, operator_str.c_str(),
                                          operator_str.length());

  char* node_string = ts_node_string(op);
  std::string orig_op_string = node_string;
  free(node_string);
  return orig_op_string + " ";
}

inline std::string AbstractParenthesizedExpressionString(const TSNode&
    parenthesized_expression_node) {
  const std::string parenthesized_expression = "parenthesized_expression";
  const std::string binary_expression = "binary_expression";
  const std::string assignment_expression = "assignment_expression";
  const std::string unary_expression = "unary_expression";
  const std::string pointer_expression = "pointer_expression";
  const std::string call_expression = "call_expression";
  const std::string field_expression = "field_expression";
  const std::string subscript_expression = "subscript_expression";

  std::string ret;
  if (ts_node_named_child_count(parenthesized_expression_node) != 1) return "";
  TSNode node = ts_node_named_child(parenthesized_expression_node, 0);
  if (parenthesized_expression.compare(ts_node_type(node)) == 0) {
    ret += "(parenthesized_expression ";
    ret += AbstractParenthesizedExpressionString(node);
    ret += ")";
  } else if (binary_expression.compare(ts_node_type(node)) == 0) {
    ret += "(binary_expression ";
    ret += OpToString(node);
    ret += AbstractBinaryExpressionString(node);
    ret += ")";
  } else if (assignment_expression.compare(ts_node_type(node)) == 0) {
    ret += "(binary_expression ";
    ret += "(\"=\") ";
    ret += AbstractBinaryExpressionString(node);
    ret += ")";
  } else if (unary_expression.compare(ts_node_type(node)) == 0) {
    ret += "(unary_expression ";
    ret += OpToString(node);
    ret += AbstractUnaryExpressionString(node);
    ret += ")";
  } else if (pointer_expression.compare(ts_node_type(node)) == 0) {
    ret += "(pointer_expression ";
    ret += AbstractUnaryExpressionString(node);
    ret += ")";
  } else if (call_expression.compare(ts_node_type(node)) == 0) {
    ret += "(call_expression)";
  } else if (field_expression.compare(ts_node_type(node)) == 0) {
    ret +=
      "(field_expression argument: (identifier) field: (field_identifier))";
  } else if (subscript_expression.compare(ts_node_type(node)) == 0) {
    ret += "(subscript_expression ";
    ret += AbstractSubscriptExpressionString(node);
    ret += ")";
  } else {
    ret += "(";
    ret += AbstractTerminalString(node);
    ret += ")";
  }

  return ret;
}

template <>
inline std::string NodeToString<LEVEL_TWO, LANGUAGE_C>(
    const TSNode& conditional_expression) {
  std::string ret;

  const std::string parenthesized_expression = "parenthesized_expression";
  if (parenthesized_expression.compare(
      ts_node_type(conditional_expression)) == 0) {
    ret += "(parenthesized_expression ";
    ret += AbstractParenthesizedExpressionString(conditional_expression);
    ret += ")";
  } else {
    throw cf_unexpected_situation(
      "Expecting paranthesized_expression at top-level, found:" +
      std::string(ts_node_string(conditional_expression)));
  }

  return ret;
}

// Currently for level 2 representation for Verilog, we use full details.
// May be we will need to implement a specialized version for Verilog
// for Level 2.
template <>
inline std::string NodeToString<LEVEL_TWO, LANGUAGE_VERILOG>(
  const TSNode& conditional_expression) {
    return NodeToString<LEVEL_MIN, LANGUAGE_VERILOG>(conditional_expression);
}
// -----------------------------------------------------------------------
// Close to full-detailed level with using Tree-sitter print. Only
// difference is in printing operators for binary and unary ops.
//
// For Level 1, both C and Verilog have same implementation.
template <>
inline std::string NodeToString<LEVEL_ONE, LANGUAGE_C>(const TSNode& node) {
  std::string ret = "";

  ret += "(";

  const std::string& binary_expression = "binary_expression";
  const std::string& unary_expression = "unary_expression";
  const std::string& assignment_expression = "assignment_expression";
  if (0 == binary_expression.compare(ts_node_type(node)) ||
      0 == unary_expression.compare(ts_node_type(node))) {
    ret += ts_node_type(node);
    ret += " ";
    ret += OpToString(node);
  } else if (0 == assignment_expression.compare(ts_node_type(node))) {
    ret += "binary_expression";
    ret += " ";
    ret += "(\"=\") ";
  } else {
    ret += ts_node_type(node);
    ret += ts_node_named_child_count(node) > 0 ? " " : "";
  }

  uint32_t children = ts_node_named_child_count(node);
  for (uint32_t i = 0; i < children; i++) {
    const TSNode child = ts_node_named_child(node, i);
    ret += NodeToString<LEVEL_ONE, LANGUAGE_C>(child);
  }

  ret += ")";

  return ret;
}

template <>
inline std::string NodeToString<LEVEL_ONE, LANGUAGE_VERILOG>(
  const TSNode& conditional_expression) {
    return NodeToString<LEVEL_ONE, LANGUAGE_C>(conditional_expression);
}

#endif  // SRC_TREE_ABSTRACTION_H_
