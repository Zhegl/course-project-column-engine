#include "convert.h"
#include "scheme_reader.h"
#include "types.h"
#include "file_reader.h"
#include <glog/logging.h>
#include <sys/types.h>
#include <stdexcept>
#include <string>
#include <vector>

void Convert(const std::string input_path, const std::string scheme_path,
             const std::string output_path, const size_t batch_size) {
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
}