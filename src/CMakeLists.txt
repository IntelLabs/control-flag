# Copyright (c) 2021 Niranjan Hasabnis and Justin Gottschlich
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

add_subdirectory(tree-sitter)
set(COMMON_INCLUDES
  PRIVATE
  ${TREE_SITTER_INCLUDE}
)
set(COMMON_LINK_LIBRARIES
  cf_base
  tree-sitter
  tree-sitter-c
  tree-sitter-php
  tree-sitter-solidity
  tree-sitter-cpp
  tree-sitter-verilog
  pthread
)

add_library(cf_base
  common_util.cpp
  train_and_scan_util.cpp
  tree_abstraction.cpp
  trie.cpp
  result_processing.cpp
  autocorrect.cpp
) 
target_include_directories(cf_base ${COMMON_INCLUDES})

add_executable(cf_file_scanner cf_file_scanner.cpp)
target_include_directories(cf_file_scanner ${COMMON_INCLUDES})
target_link_libraries(cf_file_scanner ${COMMON_LINK_LIBRARIES})
target_link_options(cf_file_scanner PRIVATE $<$<PLATFORM_ID:Windows>:-static-libgcc -static-libstdc++ -static>)

add_executable(cf_dump_code_blocks cf_dump_code_blocks.cpp)
target_include_directories(cf_dump_code_blocks ${COMMON_INCLUDES})
target_link_libraries(cf_dump_code_blocks ${COMMON_LINK_LIBRARIES})
target_link_options(cf_dump_code_blocks PRIVATE $<$<PLATFORM_ID:Windows>:-static-libgcc -static-libstdc++ -static>)

# To be able to use the default scripts even if we have chosen to build outside
#  the source directory
install(TARGETS
          cf_file_scanner
          cf_dump_code_blocks
        RUNTIME           # Following options apply to runtime artifacts.
          COMPONENT Runtime)