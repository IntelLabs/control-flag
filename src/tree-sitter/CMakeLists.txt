add_library(tree-sitter STATIC
  tree-sitter/lib/src/lib.c
)

target_include_directories(tree-sitter
  PRIVATE
  tree-sitter/lib/src
  tree-sitter/lib/include
)

add_library(tree-sitter-c STATIC
  tree-sitter-c/src/parser.c
)

target_include_directories(tree-sitter-c
  PRIVATE
  tree-sitter-c/src
)

add_library(tree-sitter-verilog STATIC
  tree-sitter-verilog/src/parser.c
)

target_include_directories(tree-sitter-verilog
  PRIVATE
  tree-sitter-verilog/src
)

add_library(tree-sitter-php STATIC
  tree-sitter-php/php/src/scanner.c
  tree-sitter-php/php/src/parser.c
)

target_include_directories(tree-sitter-php
  PRIVATE
  tree-sitter-php/php/src
)

add_library(tree-sitter-solidity STATIC
  tree-sitter-solidity/src/parser.c
)

target_include_directories(tree-sitter-solidity
  PRIVATE
  tree-sitter-solidity/src
)

add_library(tree-sitter-cpp STATIC
  tree-sitter-cpp/src/parser.c
  tree-sitter-cpp/src/scanner.c
)

target_include_directories(tree-sitter-cpp
  PRIVATE
  tree-sitter-cpp/src
)