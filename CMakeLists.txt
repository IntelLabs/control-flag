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

cmake_minimum_required(VERSION 3.4.3)
project(controlflag)
include(ExternalProject)
include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT EXISTS ${PROJECT_SOURCE_DIR}/src/tree-sitter/tree-sitter)
    execute_process(
        COMMAND git clone https://github.com/tree-sitter/tree-sitter.git ${PROJECT_SOURCE_DIR}/src/tree-sitter/tree-sitter
    )
endif()

if(NOT EXISTS ${PROJECT_SOURCE_DIR}/src/tree-sitter/tree-sitter-c)
    execute_process(
        COMMAND git clone https://github.com/tree-sitter/tree-sitter-c.git ${PROJECT_SOURCE_DIR}/src/tree-sitter/tree-sitter-c
    )
endif()

if(NOT EXISTS ${PROJECT_SOURCE_DIR}/src/tree-sitter/tree-sitter-verilog)
    execute_process(
        COMMAND git clone https://github.com/tree-sitter/tree-sitter-verilog.git ${PROJECT_SOURCE_DIR}/src/tree-sitter/tree-sitter-verilog
    )
endif()

if(NOT EXISTS ${PROJECT_SOURCE_DIR}/src/tree-sitter/tree-sitter-php)
    execute_process(
        COMMAND git clone https://github.com/tree-sitter/tree-sitter-php.git ${PROJECT_SOURCE_DIR}/src/tree-sitter/tree-sitter-php
    )
endif()

if(NOT EXISTS ${PROJECT_SOURCE_DIR}/src/tree-sitter/tree-sitter-solidity)
    execute_process(
        COMMAND git clone https://github.com/JoranHonig/tree-sitter-solidity.git ${PROJECT_SOURCE_DIR}/src/tree-sitter/tree-sitter-solidity
    )
endif()

if(NOT EXISTS ${PROJECT_SOURCE_DIR}/src/tree-sitter/tree-sitter-cpp)
    execute_process(
        COMMAND git clone https://github.com/tree-sitter/tree-sitter-cpp.git ${PROJECT_SOURCE_DIR}/src/tree-sitter/tree-sitter-cpp
    )
endif()

get_filename_component(TREE_SITTER_INCLUDE src/tree-sitter/tree-sitter/lib/include ABSOLUTE)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
