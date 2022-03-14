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

#ifndef SRC_PARSER_H_
#define SRC_PARSER_H_

#include <cassert>
#include <functional>
#include <string>
#include <thread>  // NOLINT [build/c++11]

#include "tree_sitter/api.h"

extern "C" const TSLanguage *tree_sitter_c();
extern "C" const TSLanguage *tree_sitter_verilog();
extern "C" const TSLanguage *tree_sitter_php();

enum Language {
  LANGUAGE_C = 1,
  LANGUAGE_VERILOG = 2,
  LANGUAGE_PHP = 3
};

#define LANGUAGE_MIN LANGUAGE_C
#define LANGUAGE_MAX LANGUAGE_PHP

inline Language VerifyLanguage(int language) {
  if (language < LANGUAGE_MIN)
    language = LANGUAGE_MIN;
  if (language > LANGUAGE_MAX)
    language = LANGUAGE_MAX;
  return (Language) language;
}

inline int LanguageToInt(Language l) { return static_cast<int>(l); }

template <Language L> inline const TSLanguage* GetTSLanguage();
template <> inline const TSLanguage* GetTSLanguage<LANGUAGE_C> () {
  return tree_sitter_c();
}
template <> inline const TSLanguage* GetTSLanguage<LANGUAGE_VERILOG> () {
  return tree_sitter_verilog();
}
template <> inline const TSLanguage* GetTSLanguage<LANGUAGE_PHP> () {
  return tree_sitter_php();
}

template<Language L>
class ParserBase {
 public:
  ParserBase() {
    parser_ = ts_parser_new();
    // set_language API call can fail if tree-sitter library version is
    // different than its language support.
    bool ret = ts_parser_set_language(parser_, GetTSLanguage<L>());
    assert(ret == true);
  }

  ~ParserBase() {
    if (parser_ != NULL) {
      ts_parser_delete(parser_);
      parser_ = NULL;
    }
  }

  void ResetTSParser() {
    if (parser_) {
      ts_parser_reset(parser_);
    }
  }

  TSParser* GetTSParser() {
    return parser_;
  }

 private:
  ParserBase(const ParserBase& parser_base) = delete;
  ParserBase& operator=(const ParserBase& parser_base) = delete;

  TSParser* parser_ = NULL;
};

/////////////////////////////////////////////////////////////////////
//  Language-specific functions
inline bool IsTSNodeofType(const TSNode& node, const std::string type) {
  return (!ts_node_is_null(node) &&
          0 == type.compare(ts_node_type(node)));
}

template <Language L> inline bool IsIfStatement(const TSNode& node);
template <Language L> inline TSNode GetIfConditionNode(
                                                const TSNode& if_statement);

template <> inline bool IsIfStatement<LANGUAGE_C>(const TSNode& node) {
  return IsTSNodeofType(node, "if_statement");
}
template <> inline bool IsIfStatement<LANGUAGE_VERILOG>(const TSNode& node) {
  return IsTSNodeofType(node, "conditional_statement");
}
template <> inline bool IsIfStatement<LANGUAGE_PHP>(const TSNode& node) {
  return IsTSNodeofType(node, "if_statement");
}
inline bool IsCommentNode(const TSNode& node) {
  return IsTSNodeofType(node, "comment");
}
// This method differs from ts_node_string in that it prints some details such
// as variable names, identifiers and constants that are important for our
// purpose.
template <Language L> bool IsIdentifier(const TSNode& node);
template <Language L> bool IsLiteral(const TSNode& node);
template <Language L> bool IsPrimitiveType(const TSNode& node);

template <> inline bool IsIdentifier<LANGUAGE_C>(const TSNode& node) {
  return IsTSNodeofType(node, "identifier");
}
template <>
inline bool IsLiteral<LANGUAGE_C>(const TSNode& node) {
  return IsTSNodeofType(node, "number_literal");
}
template <>
inline bool IsPrimitiveType<LANGUAGE_C>(const TSNode& node) {
  return IsTSNodeofType(node, "primitive_type");
}
template <>
inline bool IsIdentifier<LANGUAGE_VERILOG>(const TSNode& node) {
  return IsTSNodeofType(node, "simple_identifier");
}
// TODO(nhasabni): complete this for Verilog if needed.
template <>
inline bool IsLiteral<LANGUAGE_VERILOG>(const TSNode& node) { return false; }
template <>
inline bool IsPrimitiveType<LANGUAGE_VERILOG>(const TSNode& node) {
  return false;
}
// Verilog specific
inline bool IsAlwaysBlock(const TSNode& node) {
  return (IsTSNodeofType(node, "always_construct"));
}

template <>
inline TSNode GetIfConditionNode<LANGUAGE_C>(const TSNode& if_statement) {
  const std::string& kIfCondition = "condition";
  return ts_node_child_by_field_name(if_statement,
                      kIfCondition.c_str(), kIfCondition.length());
}
template <>
inline TSNode GetIfConditionNode<LANGUAGE_VERILOG>(const TSNode& if_statement) {
  const std::string& kIfCondition = "cond_predicate";
  // TreeSitter verilog parser does not support gathering node child with field
  // name. Hence this logic.
  uint32_t count = ts_node_child_count(if_statement);
  for (uint32_t i = 0; i < count; i++) {
    auto child = ts_node_child(if_statement, i);
    if (0 == kIfCondition.compare(ts_node_type(child))) {
      return child;
    }
  }
  TSNode condition_node;
  return condition_node;
  // TODO(nhasabni): This requires common_util.h, which creates cyclic
  // dependency.
  // throw cf_parse_error("if statement without cond_predicate node");
}
template <>
inline TSNode GetIfConditionNode<LANGUAGE_PHP>(const TSNode& if_statement) {
  const std::string& kIfCondition = "condition";
  return ts_node_child_by_field_name(if_statement,
                      kIfCondition.c_str(), kIfCondition.length());
}

std::string OriginalSourceExpression(const TSNode&, const std::string&);

template <Language L>
inline std::string NodeToConcreteSyntaxTree(const TSNode& node,
    const std::string& source_code, bool pretty_print = false) {
  std::string node_string;
  uint32_t start_indent = 0;

  std::function<void(const TSNode&, std::string&, uint32_t)>  helper_fn =
        [&](const TSNode& node, std::string& node_string, uint32_t indent) {
    if (ts_node_is_null(node)) return;

    // If user wants to print the tree nicely, then add newlines and spaces.
    if (pretty_print) {
      node_string += "\n";
      for (uint32_t i = 0; i < indent; i++) {
        node_string += "  ";
      }
    }
    node_string += "(";
    node_string += ts_node_type(node);

    if (IsIdentifier<L>(node) || IsLiteral<L>(node) ||
        IsPrimitiveType<L>(node)) {
      node_string += " (";
      node_string += OriginalSourceExpression(node, source_code);
      node_string += ")";
    }

    uint32_t count = ts_node_named_child_count(node);
    for (uint32_t i = 0; i < count; i++) {
      auto child = ts_node_named_child(node, i);
      helper_fn(child, node_string, indent + 1);
    }

    node_string += ")";
  };

  helper_fn(node, node_string, start_indent);
  return node_string;
}

#endif  // SRC_PARSER_H_
