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

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/tests/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/bin)
set(CTEST_BINARY_DIRECTORY ${CMAKE_BINARY_DIR}/tests/bin)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${TREE_SITTER_INCLUDE})

set (test_c_parser_parts 1 2 3 4)
set (test_verilog_parser_parts 1 2 3 4)
set (test_expression_compactor_parts 1 2 3 4 5 6 7)
set (test_dump_conditional_exprs_parts 1 2 3 4 5 6)
set (test_trie_parts 1 2 3 4 5 6)

file(GLOB files "test_*.cpp")

foreach(file ${files})
    string(REGEX REPLACE "(^.*/|\\.[^.]*$)" "" file_without_ext ${file})
    add_executable(${file_without_ext} ${file})
    target_link_libraries(${file_without_ext}
      cf_base
      tree-sitter
      tree-sitter-c
      tree-sitter-verilog
      pthread)
    foreach(part ${${file_without_ext}_parts})
      add_test(NAME ${file_without_ext}_${part} COMMAND ${file_without_ext} ${part} WORKING_DIRECTORY ${CTEST_BINARY_DIRECTORY})
      set_tests_properties(${file_without_ext}_${part}
        PROPERTIES
        PASS_REGULAR_EXPRESSION "Test passed")
      set_tests_properties(${file_without_ext}_${part}
        PROPERTIES
        FAIL_REGULAR_EXPRESSION "(Exception|Test failed)")
      set_tests_properties(${file_without_ext}_${part}
        PROPERTIES
        TIMEOUT 120)
    endforeach()
endforeach()
