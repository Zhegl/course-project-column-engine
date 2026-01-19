#include "convert.h"
#include <interface/metadata.h>
#include <format/scheme_reader.h>
#include <format/meta_reader.h>
#include <types/types.h>
#include <io/file_reader.h>
#include <glog/logging.h>
#include <sys/types.h>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

void ConvertToColumnar(const std::string& input_path, const std::string& scheme_path,
                       const std::string& output_path, const size_t batch_size) {
    Scheme scheme = ReadScheme(scheme_path);

    std::vector<std::vector<ColumnType>> batch(scheme.columns.size());
    std::vector<BatchMetaData> batch_meta;
    size_t offset = 0;

    FileReader reader(input_path);
    FileWriter writer(output_path);

    while (!reader.Eof()) {
        for (size_t i = 0; i < batch_size; ++i) {
            char symbol;
            std::string current_str;
            size_t status = 0;
            while (reader.Read(&symbol, 1)) {
                if (symbol == ',' || symbol == '\n') {
                    batch[status].push_back(ConvertType(current_str, scheme.columns[status].type));
                    current_str.clear();
                    ++status;
                    if (symbol == '\n') {
                        if (status != scheme.columns.size()) {
                            throw std::runtime_error("syntax error in " + input_path);
                        }
                        status = 0;
                        break;
                    }
                } else if (std::isgraph(symbol)) {
                    current_str.push_back(symbol);
                }
            }
            if (reader.Eof()) {
                if (status == scheme.columns.size() - 1) {
                    batch[status].push_back(ConvertType(current_str, scheme.columns[status].type));
                } else if (status != 0) {
                    throw std::runtime_error("syntax error in " + input_path);
                }
                break;
            }
        }

        for (size_t i = 0; i < scheme.columns.size(); ++i) {
            size_t add = WriteType(batch[i], scheme.columns[i].type, writer);
            batch[i].clear();
            batch_meta.push_back({add, offset});
            offset += add;
        }
    }

    for (auto val : batch_meta) {
        writer.Write(val.size);
        writer.Write(val.offset);
    }

    for (auto col : scheme.columns) {
        writer.Write(col.type);
        writer.Write(col.name.data(), col.name.size() + 1);
    }

    writer.Write(offset);
    writer.Write(static_cast<uint64_t>(batch_meta.size()));
    writer.Write(static_cast<uint64_t>(scheme.columns.size()));
}


void PrintScheme(Scheme scheme, const std::string& path) {
    FileWriter writer(path);
    for (const auto& column : scheme.columns) {
        writer.Write(column.name.data(), column.name.size());
        writer.Write(',');
        auto type_name = GetTypeName(column.type);
        writer.Write(type_name.data(), type_name.size());
        writer.Write('\n');
    }
}

void PrintTable(Scheme scheme, std::vector<BatchMetaData> batch_meta, const std::string& input_path, const std::string& output_path) {
    FileReader reader(input_path);
    FileWriter writer(output_path);
    size_t batch = 0;
    while (batch < batch_meta.size()) {
        std::vector<std::vector<ColumnType>> columns(scheme.columns.size());
        for (size_t col = 0; col < scheme.columns.size(); ++col) {
            if (batch >= batch_meta.size()) {
                throw std::runtime_error("Batch metadata error");
            }
            columns[col] = GetBatch(batch_meta[batch].size, scheme.columns[col].type, reader);
            ++batch;
        }
        for (size_t i = 0; i < columns[0].size(); ++i) {
            std::string data = ColumnTypeToString(columns[0][i]);
            writer.Write(data.data(), data.size());
            for (size_t j = 1; j < scheme.columns.size(); ++j) {
                writer.Write(',');
                data = ColumnTypeToString(columns[j][i]);
                writer.Write(data.data(), data.size());
            }
            writer.Write('\n');
        }
    }
}

void ConvertToCsv(const std::string& input_path, const std::string& scheme_path,
                  const std::string& output_path) {
    auto [batch_meta, scheme] = GetMeta(input_path); 
    PrintScheme(scheme, scheme_path);
    PrintTable(scheme, batch_meta, input_path, output_path);
}