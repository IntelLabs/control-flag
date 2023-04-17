**A friendly request: Thanks for visiting control-flag GitHub repository! If you find control-flag useful, we would appreciate a note from you (to niranjan.hasabnis@intel.com). And, of course, we love testimonials!**

-- The ControlFlag Team 

[![linux_build_and_test](https://github.com/IntelLabs/control-flag/actions/workflows/linux_controlflag_cmake.yml/badge.svg)](https://github.com/IntelLabs/control-flag/actions/workflows/linux_controlflag_cmake.yml)
[![linux_style_check](https://github.com/IntelLabs/control-flag/actions/workflows/linux_controlflag_cpplint.yml/badge.svg)](https://github.com/IntelLabs/control-flag/actions/workflows/linux_controlflag_cpplint.yml)
[![macos_build_and_test](https://github.com/IntelLabs/control-flag/actions/workflows/macos_controlflag_cmake.yml/badge.svg)](https://github.com/IntelLabs/control-flag/actions/workflows/macos_controlflag_cmake.yml)
[![macos_style_check](https://github.com/IntelLabs/control-flag/actions/workflows/macos_controlflag_cpplint.yml/badge.svg)](https://github.com/IntelLabs/control-flag/actions/workflows/macos_controlflag_cpplint.yml)
[![GitHub license](https://img.shields.io/github/license/IntelLabs/control-flag)](https://github.com/IntelLabs/control-flag/blob/master/LICENSE)

# ControlFlag: A Self-supervised Idiosyncratic Pattern Detection System for Software Control Structures

ControlFlag is a self-supervised idiosyncratic pattern detection system that
learns typical patterns that occur in the control structures of high-level
programming languages, such as C/C++, by mining these patterns from open-source
repositories (on GitHub and other version control systems). It then applies
learned patterns to detect anomalous patterns in user's code.

## Brief technical description

ControlFlag's pattern anomaly detection system can be used for various problems
such as typographical error detection, flagging a missing NULL check to
name a few. *This PoC demonstrates ControlFlag's application in the typographical
error detection.*

Figure below shows ControlFlag's two main phases: (1) pattern
mining phase, and (2) scanning for anomalous patterns phase. The pattern mining
phase is a "training phase" that mines typical patterns in the user-provided GitHub
repositories and then builds a decision-tree from the mined patterns. The scanning
phase, on the other hand, applies the mined patterns to flag anomalous
expressions in the user-specified target repositories.

![ControlFlag design](/docs/controlflag_design.jpg)

More details can be found in our MAPS paper (https://arxiv.org/abs/2011.03616).

## Directory structure (evolving)
- `src`: Source code for ControlFlag for typographical error detection system
- `scripts`: Scripts for pattern mining and scanning for anomalies
- `quick_start`: Scripts to run quick start tests
- `github`: Scripts and data for downloading GitHub repos.
- `tests`: unit tests

## Install

ControlFlag can be built on Linux and MacOS.

#### Requirements

- CMake 3.4.3 or above
- C++17 compatible compiler
- [Tree-sitter](https://github.com/tree-sitter/tree-sitter.git) parser (downloaded automatically as part of cmake)
- [GNU parallel](https://www.gnu.org/software/parallel/) (optional, if you want
  to generate your own training data)

**Tested build configuration on Linux-based systems**
- CentOS-7.6/Ubuntu-20.04 with g++-v10.2.0 for x86\_64

**Tested build configuration on MacOS**
- MacOS Mojave v10.14.6 with clang-1001.0.46.4 (Apple LLVM version 10.0.1) for x86\_64 (obtained from The Command Line Tools Package)

#### Build

```
$ cd control-flag
$ cmake .
$ make -j
$ make test
```
All tests in `make test` should pass.

## Using ControlFlag

### Quick start

#### Using patterns obtained from several GitHub repos to scan repository of your choice

Download the training data for the language of interest depending on the memory constraints of your device. Note, however, that using smaller datasets may lead to reduced accuracy in the results ControlFlag produces and possibly an increase in the number of false positives it generates.

Language | Dataset name | Size on disk | Memory requirements | Direct link | MD5 checksum
---------|--------------|--------------|---------------------|-------------|-------------
C | Small        | ~100MB       | ~400MB              | [link](https://www.dropbox.com/s/88kb00r71t0lf94/c_lang_if_stmts_6000_gitrepos_small.ts.tgz?dl=0)| 2825f209aba0430993f7a21e74d99889
C | Medium       |   ~450MB     | ~1.3GB           | [link](https://www.dropbox.com/s/zjdwmqvhgbdnuns/c_lang_if_stmts_6000_gitrepos_medium.ts.tgz?dl=0) | aab2427edebe9ed4acab75c3c6227f24
C | Large        |   ~9GB       | ~13GB           | [link](https://www.dropbox.com/s/oledgd1jli55xps/c_lang_if_stmts_6000_gitrepos.large.ts.tgz?dl=0) | 1ba954d9716765d44917445d3abf8e85
C++ | Small | ~200MB | ~500MB | [link](https://www.dropbox.com/s/jtys6pihknl329b/cpp_controlflag_if_stmts_small.ts.tgz?dl=0) | f954486e20961f0838ac08e5d4dbf312
C++ | Medium | ~500MB | ~1.3GB | [link](https://www.dropbox.com/s/ea9nwa2ijv2zfxq/cpp_controlflag_if_stmts_medium.ts.tgz?dl=0) |  a5c18ea1cdbe354b93aabf9ecaa5b07a
C++ | Large | ~1.2GB | ~3GB | [link](https://www.dropbox.com/s/4du59qq28r4qnbw/cpp_controlflag_if_stmts_large.ts.tgz?dl=0) | 4f5ffc1ab942eaba399cafd5be8bb45f
PHP | Small      | ~120MB       |  ~1GB           | [link](https://www.dropbox.com/s/it0ql3d2e1viao8/php_controlflag_if_stmts.ts.tgz?dl=0) | 5a1cc4c24a20de7dad1b9f40661d517a

```
$ Download <tgz_file> from the link above.
$ (optional) md5sum <tgz_file>
$ tar -zxf <tgz_file>
```

To scan C code of your choice, use below command:

```
$ scripts/scan_for_anomalies.sh -d <directory_to_be_scanned_for_anomalies> -t <training_data>.ts -o <output_directory_to_store_log_files> -l 1
```

To scan C++ code of your choice, use below command:

```
$ scripts/scan_for_anomalies.sh -d <directory_to_be_scanned_for_anomalies> -t <training_data>.ts -o <output_directory_to_store_log_files> -l 4
```

Once the run is complete (which could take some time depending on your system and the
number of programs from your repository that can be scanned by ControlFlag,) refer to [the section below to
understand scan output](#understanding-scan-output).

#### Mining patterns from a small repo and applying them to another small repo

In this test for C language programs, we will mine patterns from
[Glb-director](https://github.com/github/glb-director.git) project of GitHub and
apply them to flag anomalies in GitHub's [brubeck](https://github.com/github/brubeck.git) project.

Simply run below command:
```
cd quick_start && ./test1_c.sh
```

If everything goes well, you can see output from the scanner in `test1_scan_output`
directory. Look for "Potential anomaly" label in it by `grep "Potential anomaly"
-C 5 \*.log`, and you should see output like below:

```
thread_6.log-Level:TWO Expression:(parenthesized_expression (binary_expression ("==") (identifier) (non_terminal_expression))) found in training dataset:
Source file: brubeck/src/server.c:266:5:(s == sizeof(fdsi))
thread_6.log-Autocorrect search took 0.000 secs
thread_6.log:Potential anomaly
thread_6.log-Did you mean:(parenthesized_expression (binary_expression ("==") (identifier) (non_terminal_expression))) with editing cost:0 and occurrences: 1
thread_6.log-Did you mean:(parenthesized_expression (binary_expression ("==") (identifier) (null))) with editing cost:1 and occurrences: 25
thread_6.log-Did you mean:(parenthesized_expression (binary_expression ("==") (identifier) (identifier))) with editing cost:1 and occurrences: 5
thread_6.log-Did you mean:(parenthesized_expression (binary_expression (">=") (identifier) (non_terminal_expression))) with editing cost:1 and occurrences: 3
thread_6.log-Did you mean:(parenthesized_expression (binary_expression ("==") (non_terminal_expression) (non_terminal_expression))) with editing cost:1 and occurrences: 2
```
The anomaly is flagged for `brubeck/src/server.c` at line number `266`.

### Detailed steps

1. __Pattern Mining phase__ (if you want to generate training data yourself)

If you do not want to generate training data yourself, go to [Evaluation step below](#evaluation-or-scanning-for-anomalies-in-c-code-from-test-repo).

In this phase, we mine the idiosyncratic patterns that appear in the control
structures of high-level language such as C. *This PoC mines patterns from `if`
statements that appear in C programs.*

If you want to use your own repository for mining patterns, jump to Step 1.2.

1.1 __Downloading GitHub repos for C language having more than 100 stars__

Steps below show how to download GitHub repos for C language that have more than 100 stars
(`c100.txt`) and generate training data. `training_repo_dir` is a directory
where the command below will clone all the repos.

```
$ cd github
$ python download_repos.py -f c100.txt -o <training_repo_dir> -m clone -p 5
```

1.2 __Mining patterns from downloaded repositories__

You can use your own repository to mine for expressions by passing it in
place of <training_repo_dir>.

`mine_patterns.sh` script helps for this. It's usage is as below:

```
Usage: ./mine_patterns.sh -d <directory_to_mine_patterns_from> -o <output_file_to_store_training_data>
Optional:
[-n number_of_processes_to_use_for_mining]  (default: num_cpus_on_system)
[-l source_language_number] (default: 1 (C), supported: 1 (C), 2 (Verilog), 3 (PHP), 4 (C++)
[-g github_repo_id] (default: 0) A unique identifier for GitHub repository, if any
```

We use it as:
```
$ scripts/mine_patterns.sh -d <training_repo_dir> -o <training_data_file> -l 1
```

`<training_dat_file>` contains conditional expressions in C language that are
found in the specified GitHub repos and their AST (abstract syntax tree) representations.
You can view this file as a text file, if
you want.

## Evaluation (or scanning for anomalies)

We can run `scan_for_anomalies.sh` script to scan target directory of interest.
Its usage is as below.
```
Usage: ./scan_for_anomalies.sh -t <training_data> -d <directory_to_scan_for_anomalous_patterns>
Optional:
 [-c max_cost_for_autocorrect]              (default: 2)
 [-n max_number_of_results_for_autocorrect] (default: 5)
 [-j number_of_scanning_threads]            (default: num_cpus_on_systems)
 [-o output_log_dir]                        (default: /tmp)
 [-l source_language_number]                (default: 1 (C), supported: 1 (C), 2 (Verilog), 3 (PHP), 4 (C++))
 [-a anomaly_threshold]                     (default: 3.0)
```

As a part of scanning for anomalies, ControlFlag also suggests possible
corrections in case a conditional expression is flagged as an anomaly. `25` is the
`max_cost` for the correction -- how close should the suggested correction be to
possibly mistyped expression. Increasing `max_cost` leads to suggesting more
corrections. ___If you feel that the number of reported anomalies is
high, consider reducing `anomaly_threshold` to `1.0` or less___.

### Understanding scan output

Under `output_log_dir` you will find multiple log files corresponding to
the scan output from different scanner threads. Potential anomalies are reported
with "Potential anomaly" as a label. Command below will report log files
containing at least one anomaly.

```
$ grep "Potential anomaly" <output_log_dir>/thread_*.log
```

A sample anomaly report looks like below:
```
Level:<ONE or TWO> Expression: <AST_for_anomalous_expression>
Source file and line number: <Source code expression with line number having the anomaly>
Potential anomaly
Did you mean ...
```
The text after "Did you mean" shows possible corrections to the anomalous expression.

## Success stories
In the spirit of community service, we routinely scan open-source packages using ControlFlag. We have found several programming errors in various open-source projects. We are mentioning some of the errors that are confirmed by the respective developers below.

Issue link | Language | Erroneous expression | Comment
-----------|----------------------|----------------------|---------
https://github.com/curl/curl/pull/6193 | C | `if (s->keepon > TRUE)` | Comparison between a variable and a boolean using `>`
https://github.com/vrpn/vrpn/issues/263 | C | `(l_inbuf[2] \| 1)`, `if (l_inbuf[3] \| 1)` | Incorrect use of `\|` instead of `&`
https://github.com/vlm/asn1c/issues/443 | C | `if(!saved_aid && 0)` | Dead code
https://github.com/shoes/shoes3/issues/468 | C | `if ((attr == 39) \|\| (attr = 49))` | Incorrect use of `=` instead of `==`
https://github.com/IoLanguage/io/issues/455 | C | `if (UArray_greaterThan_(self, other) \| UArray_equals_(self, other))` | Inefficient use of `\|` instead of `\|\|`
https://github.com/IoLanguage/io/issues/455 | C | `if( ln = (SFG_Node *)node->Next )`, `if( ln = (SFG_Node *)node->Prev )` | Missing parenthesis
https://github.com/elua/elua/issues/170 | C | `if (Protection_Level_1_Register &= FMI_Sector_Mask)` | Missing parenthesis
