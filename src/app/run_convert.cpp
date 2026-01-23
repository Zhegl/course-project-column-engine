#include <convert/convert.h>
#include <glog/logging.h>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace column_engine {

void Usage() {
    std::cout << "Usage: convert --input <file> --output <file> --scheme <file> [--batch <size>] "
                 "[--reversed]"
              << std::endl;
    exit(1);
}

void Run(int argc, char** argv) {
    size_t batch_size = 1000000;
    std::string input_path;
    std::string output_path;
    std::string scheme_path;
    bool reverse = false;
    size_t i = 0;

    while (++i < argc) {
        std::string arg_name = argv[i];

        if (arg_name == "--reversed") {
            reverse = true;
            continue;
        }

        ++i;
        if (i >= argc) {
            Usage();
        }
        std::string arg = argv[i];

        if (arg_name == "--input") {
            input_path = arg;
        } else if (arg_name == "--output") {
            output_path = arg;
        } else if (arg_name == "--scheme") {
            scheme_path = arg;
        } else if (arg_name == "--batch") {
            batch_size = std::stoll(arg);
        } else {
            Usage();
        }
    }

    if (!reverse) {
        ConvertToColumnar(input_path, scheme_path, output_path, batch_size);
    } else {
        ConvertToCsv(input_path, scheme_path, output_path);
    }
}

};  // namespace column_engine

int main(int argc, char** argv) {
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;

    try {
        column_engine::Run(argc, argv);
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        LOG(ERROR) << e.what();
        google::ShutdownGoogleLogging();
        return 1;
    }

    google::ShutdownGoogleLogging();
    return 0;
}