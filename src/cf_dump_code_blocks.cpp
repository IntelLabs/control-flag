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
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "common_util.h"
#include "tree_abstraction.h"

struct CFDumpArgs {
  std::string source_file_ = "";
  Language source_language_ = LANGUAGE_C;
  TreeLevel level_ = LEVEL_MAX;
  size_t github_contributor_id_ = 0;
};

template <Language G>
void DumpCodeBlocks(const code_blocks_t&
    code_blocks, const std::string& source_file_contents,
    TreeLevel level, size_t contributor_id) {
  for (auto code_block : code_blocks) {
    std::string code_block_l1, code_block_l2;
    try {
      code_block_l1 = NodeToString<LEVEL_ONE, G>(code_block);
      code_block_l2 = NodeToString<LEVEL_TWO, G>(code_block);
    } catch (std::exception& e) {
      // We just skip this expression.
      continue;
    }

    std::cout << "//" << OriginalSourceExpression(code_block,
                         source_file_contents) << std::endl;
    std::cout << contributor_id << ",AST_expression_"
              << LevelToString<LEVEL_ONE>() << ":"
              << code_block_l1 << std::endl;
    std::cout << contributor_id << ",AST_expression_"
              << LevelToString<LEVEL_TWO>() << ":"
              << code_block_l2 << std::endl;
  }
}

template <Language G>
void DumpCodeBlocksFromSourceFile(const CFDumpArgs& command_args) {
  ManagedTSTree ts_tree;
  std::string source_file_contents;
  try {
    ts_tree = GetTSTree<G>(command_args.source_file_, source_file_contents);
  } catch(std::string& error) {
    std::cerr << error << " in " << command_args.source_file_
              << "... skipping" << std::endl;
    return;
  }

  code_blocks_t code_blocks;
  code_blocks.clear();
  CollectCodeBlocksOfInterest<G>(ts_tree, code_blocks);

  DumpCodeBlocks<G>(code_blocks, source_file_contents,
                                command_args.level_,
                                command_args.github_contributor_id_);
}

/////////////////////////////////////////////////////////////////////

int handle_command_args(int argc, char* argv[], CFDumpArgs& command_args) {
  auto print_usage = [&]() {
      std::cerr << "Usage: " << argv[0] << std::endl
                << "  -f source_file " << std::endl
                << "  [-t tree_depth]   (default:  " << LEVEL_MAX << ")"
                << std::endl
                << "  [-g github_id]    (default: 0)"
                << std::endl
                << "  [-l source_language_number]   (default: "
                << LANGUAGE_C << ")"
                << ", supported: 1 (C), 2 (Verilog), 3 (PHP), 4 (C++)"
                << std::endl;
  };

  int opt;
  while ((opt = getopt(argc, argv, "f:l:g:t:")) != -1) {
    switch (opt) {
      case 'f': command_args.source_file_ = FormatPath(optarg); break;
      case 'l':
        command_args.source_language_ = VerifyLanguage(atoi(optarg)); break;
      case 'g':
        command_args.github_contributor_id_ = std::strtoul(optarg, NULL, 10);
        break;
      case 't': command_args.level_ = VerifyTreeLevel(atoi(optarg)); break;
      default: print_usage(); return EXIT_FAILURE;
    }
  }

  if (command_args.source_file_ == "") {
    print_usage();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
  CFDumpArgs command_args;

  int status = 0;
  status = handle_command_args(argc, argv, command_args);
  if (status != EXIT_SUCCESS) return status;

  try {
    switch (command_args.source_language_) {
      case LANGUAGE_C:
        DumpCodeBlocksFromSourceFile<LANGUAGE_C>(command_args);
        break;
      case LANGUAGE_VERILOG:
        DumpCodeBlocksFromSourceFile<LANGUAGE_VERILOG>(command_args);
        break;
      case LANGUAGE_PHP:
        DumpCodeBlocksFromSourceFile<LANGUAGE_PHP>(command_args);
        break;
      case LANGUAGE_CPP:
        DumpCodeBlocksFromSourceFile<LANGUAGE_CPP>(command_args);
        break;
      default:
        throw cf_unexpected_situation("Unsupported language:" +
                                      std::to_string(LanguageToInt(
                                        command_args.source_language_)));
    }
  } catch (std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }

  exit(EXIT_SUCCESS);
}
