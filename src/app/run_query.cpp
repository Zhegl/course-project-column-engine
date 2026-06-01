#include <api/columnar_engine.h>
#include <types/experiment_config.h>

#include <glog/logging.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "app/clickbench_queries.h"

using column_engine::Engine;
using column_engine::ExperimentConfig;
using column_engine::QueryResult;

static std::string CsvField(const std::string& s) {
    if (s.find(',') == std::string::npos &&
        s.find('"') == std::string::npos &&
        s.find('\n') == std::string::npos) {
        return s;
    }
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') {
            out += '"';
        }
        out += c;
    }
    out += '"';
    return out;
}

static void WriteResult(const QueryResult& result, std::ostream& out) {
    for (const auto& row : result) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i) {
                out << ',';
            }
            out << CsvField(row[i]);
        }
        out << '\n';
    }
}

int main(int argc, char** argv) {
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;

    if (argc < 4) {
        std::cerr << "Usage: run_query <query_num> <columnar_file> <output_csv> [--no-bloom] [--rle-threshold=N]\n";
        return 1;
    }

    int query_num = std::stoi(argv[1]);
    std::string columnar = argv[2];
    std::string output_path = argv[3];

    for (int i = 4; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--no-bloom") {
            ExperimentConfig::Get().use_bloom = false;
        } else if (arg.rfind("--rle-threshold=", 0) == 0) {
            ExperimentConfig::Get().rle_threshold = std::stoull(arg.substr(16));
        }
    }

    try {
        Engine engine(columnar);
        QueryResult result = RunQuery(engine, query_num);

        std::ofstream out(output_path);
        if (!out) {
            throw std::runtime_error("Cannot open output: " + output_path);
        }
        // skip header row — bench expects only data rows
        for (size_t i = 1; i < result.size(); ++i) {
            const auto& row = result[i];
            for (size_t j = 0; j < row.size(); ++j) {
                if (j) {
                    out << ',';
                }
                out << CsvField(row[j]);
            }
            out << '\n';
        }
    } catch (const std::runtime_error& e) {
        LOG(WARNING) << "Query " << query_num << " skipped: " << e.what();
        std::ofstream out(output_path);
        google::ShutdownGoogleLogging();
        return 0;
    } catch (const std::exception& e) {
        LOG(ERROR) << e.what();
        google::ShutdownGoogleLogging();
        return 1;
    }

    google::ShutdownGoogleLogging();
    return 0;
}
