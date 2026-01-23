#include "meta_reader.h"
#include <glog/logging.h>

namespace column_engine {

std::vector<BatchMetaData> GetBatchMeta(FileReader& reader, size_t batches_amount) {
    std::vector<BatchMetaData> result;
    for (size_t i = 0; i < batches_amount; ++i) {
        auto size = reader.Read<uint64_t>();
        auto offset = reader.Read<uint64_t>();
        result.emplace_back(size, offset);
    }
    return result;
}

Scheme GetScheme(FileReader& reader, size_t columns_amount) {
    Scheme result;
    for (size_t i = 0; i < columns_amount; ++i) {
        std::string name;
        char symbol;

        while (reader.Read(&symbol, 1)) {
            if (symbol == '\000') {
                break;
            }
            name.push_back(symbol);
        }


        auto type = GetType(name);
        name.clear();

        while (reader.Read(&symbol, 1)) {
            if (symbol == '\000') {
                break;
            }
            name.push_back(symbol);
        }
        result.columns.emplace_back(name, type);
    }
    return result;
}

std::pair<std::vector<BatchMetaData>, Scheme> GetMeta(const std::string& path) {
    FileReader reader(path);
    auto input_size = reader.Size();
    const size_t base_meta_offset = sizeof(uint64_t) * 3;
    if (input_size < base_meta_offset) {
        throw std::runtime_error(path + " is too small");
    }
    reader.Jump(input_size - base_meta_offset);

    size_t meta_start = reader.Read<uint64_t>();
    size_t batches_amount = reader.Read<uint64_t>();
    size_t columns_amount = reader.Read<uint64_t>();

    LOG(INFO) << "meta_start: " << meta_start << " batches_amount: " << batches_amount
              << " columns_amount: " << columns_amount;

    if (meta_start > input_size) {
        throw std::runtime_error(path + " is broken");
    }
    reader.Jump(meta_start - input_size);

    auto batch_meta = GetBatchMeta(reader, batches_amount);
    auto scheme = GetScheme(reader, columns_amount);
    return std::make_pair(batch_meta, scheme);
}

};  // namespace column_engine