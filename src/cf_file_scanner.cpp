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

#include <math.h>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>  // NOLINT [build/c++11]

#include "train_and_scan_util.h"
#include "trie.h"

struct FileScannerArgs {
  std::string train_dataset_ = "";
  std::string eval_source_file_ = "";
  std::string eval_source_file_list_ = "";
  Language eval_file_language_ = LANGUAGE_C;
  std::string log_dir_ = "/tmp/";
  TrainAndScanUtil::ScanConfig scan_config_;
};

static int handle_command_args(int argc, char* argv[], FileScannerArgs& args) {
  auto print_usage = [&]() {
    std::cerr << "Usage: " << argv[0] << std::endl
           << "  -t if_statements_to_train_on " << std::endl
           << "  {-e source_file_to_scan |"
           << "   -s file_containing_list_of_source_files_to_scan}"
           << std::endl
           << "  [-c max_cost_for_autocorrect]              (default: 2)"
           << std::endl
           << "  [-n max_number_of_results_for_autocorrect] (default: 5)"
           << std::endl
           << "  [-j number_of_scanning_threads]            (default: 1)"
           << std::endl
           << "  [-o output_log_dir]                        (default: /tmp)"
           << std::endl
           << "  [-a anomaly_threshold]                     (default: 3.0)"
           << std::endl
           << "  [-l source_language_number]                (default: 1 (C), "
           << "supported: 1 (C), 2 (Verilog), 3(PHP))"
           << std::endl
           << "  [-v log_level ]                            (default: 0, "
           << "{ERROR, 0}, {INFO, 1}, {DEBUG, 2})"
           << std::endl;
  };

  int opt;
  while ((opt = getopt(argc, argv, "v:t:e:c:n:s:j:o:a:l:")) != -1) {
    switch (opt) {
      case 't': args.train_dataset_ = optarg; break;
      case 'e': args.eval_source_file_ = optarg; break;
      case 's': args.eval_source_file_list_ = optarg; break;
      case 'o': args.log_dir_ = optarg; break;
      // Fixing the max cost to 2 to keep autocorrection time reasonable.
      case 'c': args.scan_config_.max_cost_ = std::max(0, atoi(optarg)); break;
      case 'n': args.scan_config_.max_autocorrections_ =
                  std::max(0, atoi(optarg)); break;
      case 'j': args.scan_config_.num_threads_ = std::max(1, atoi(optarg));
                break;
      case 'a': args.scan_config_.anomaly_threshold_ = atof(optarg); break;
      case 'v': if (atoi(optarg) >= TrainAndScanUtil::LogLevel::MIN &&
                    atoi(optarg) <= TrainAndScanUtil::LogLevel::MAX) {
                  args.scan_config_.log_level_ =
                    static_cast<TrainAndScanUtil::LogLevel>(atoi(optarg));
                }
                break;
      case 'l': args.eval_file_language_ = VerifyLanguage(atoi(optarg)); break;
      default: /* '?' */
          print_usage();
          return EXIT_FAILURE;
    }
  }
  if (args.train_dataset_ == "" ||
      (args.eval_source_file_ == "" && args.eval_source_file_list_ == "")) {
    print_usage();
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

static int AddEvalFileNamesIntoList(const FileScannerArgs& file_scanner_args,
    std::vector<std::string>& eval_file_names) {
  if (file_scanner_args.eval_source_file_ != "") {
    eval_file_names.push_back(file_scanner_args.eval_source_file_);
  } else {
    std::string line;
    std::ifstream stream(file_scanner_args.eval_source_file_list_.c_str());
    if (!stream.is_open()) {
      throw cf_file_access_exception("Open failed:" +
                                      file_scanner_args.eval_source_file_list_);
    }
    while (std::getline(stream, line))
      eval_file_names.push_back(line);
  }
  return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
  FileScannerArgs file_scanner_args;

  int status = 0;
  status = handle_command_args(argc, argv, file_scanner_args);
  if (status != EXIT_SUCCESS) return status;

  std::vector<std::string> eval_file_names;
  status = AddEvalFileNamesIntoList(file_scanner_args, eval_file_names);
  if (status != EXIT_SUCCESS) return status;

  try {
    TrainAndScanUtil train_and_scan_util(file_scanner_args.scan_config_);
    status = train_and_scan_util.ReadTrainingDatasetFromFile(
                file_scanner_args.train_dataset_, std::cout);

    // Perform multi-threaded inference / scan for bugs.
    std::atomic<size_t> file_index(0);
    std::atomic<size_t> printed_file_index(-1);
    size_t tenth_eval_file_names = eval_file_names.size() < 10 ?
                                   eval_file_names.size() :
                                   eval_file_names.size() / 10;

    auto thread_scan_fn = [&](const size_t thread_num,
                              const std::string& log_file_name) {
      std::ofstream log_file(log_file_name.c_str());

      // Greedy multi-threading: scan next file if current is complete.
      while (file_index.load() < eval_file_names.size()) {
        // Grab file to scan for bugs.
        std::string& eval_file = eval_file_names[file_index.load()];
        file_index++;
        log_file << "[TID=" << std::this_thread::get_id() << "] "
                 << "Scanning File: " << eval_file << std::endl;

        // Scan.
        switch (file_scanner_args.eval_file_language_) {
          case LANGUAGE_C:
            status = train_and_scan_util.ScanFile<LANGUAGE_C>(eval_file,
                                                              log_file);
            break;
          case LANGUAGE_VERILOG:
            status = train_and_scan_util.ScanFile<LANGUAGE_VERILOG>(eval_file,
                                                                    log_file);
            break;
          case LANGUAGE_PHP:
            status = train_and_scan_util.ScanFile<LANGUAGE_PHP>(eval_file,
                                                                    log_file);
            break;
          default:
            throw cf_unexpected_situation("Unsupported language:" +
                                          std::to_string(LanguageToInt(
                                      file_scanner_args.eval_file_language_)));
        }

        // Report progress at every 10th % point.
        if (file_index.load() % tenth_eval_file_names == 0 &&
            printed_file_index.load() < file_index.load())
        std::cout << "Scan progress:" << file_index.load() << "/"
                  << eval_file_names.size() << " ... in progress" << std::endl;
        std::cout.flush();
        printed_file_index = file_index.load();
      }

      log_file.close();
    };

    // Start scanner threads and wait for join.
    std::vector<std::thread> scanner_threads;
    std::cout << "Storing logs in " << file_scanner_args.log_dir_ << std::endl;
    // Restricting the number of threads that we use for parallel scan, because
    // every scan invokes parallel autocorrect also.
    size_t sqrt_max_threads = static_cast<size_t>(
        sqrtf(static_cast<float>(file_scanner_args.scan_config_.num_threads_)));
    for (size_t i = 0; i < sqrt_max_threads; i++) {
      std::string log_file_name = file_scanner_args.log_dir_ + "/thread_" +
                                  std::to_string(i) + ".log";
      scanner_threads.push_back(std::thread(thread_scan_fn, i, log_file_name));
    }
    for (auto& scanner_thread : scanner_threads) {
      scanner_thread.join();
    }
  } catch (std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
  return 0;
}
