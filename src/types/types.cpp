#include "types.h"
#include <sys/types.h>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include <glog/logging.h>

namespace column_engine {

std::shared_ptr<ColumnTypeName> GetType(const std::string& name) {
    if (name == "int64") {
        return std::make_shared<ColumnTypeInt64>();
    } else if (name == "string") {
        return std::make_shared<ColumnTypeString>();
    }
    LOG(INFO) << "Unknown type: " << name;
    return std::make_shared<ColumnTypeString>();
}

std::string ColumnTypeToString(ColumnValue data) {
    if (std::holds_alternative<int64_t>(data)) {
        return std::to_string(std::get<int64_t>(data));
    } else if (std::holds_alternative<std::string>(data)) {
        return std::get<std::string>(data);
    }
    throw std::runtime_error("Unknown type in ColumnTypeToString");
}

std::string ColumnTypeString::GetTypeName() {
    return "string";
}
ColumnValue ColumnTypeString::ConvertType(std::string val) {
    return val;
}

size_t ColumnTypeString::WriteType(std::vector<ColumnValue> data, FileWriter& writer) {
    size_t result = 0;
    for (auto val : data) {
        writer.Write(std::get<std::string>(val).data(), std::get<std::string>(val).size() + 1);
        result += std::get<std::string>(val).size() + 1;
    }
    return result;
}

std::vector<ColumnValue> ColumnTypeString::GetBatch(size_t size, FileReader& reader) {
    std::vector<ColumnValue> result;
    std::string add;
    char symbol;
    while (size) {
        while (reader.Read(&symbol, 1)) {
            --size;
            if (symbol == '\000') {
                break;
            }
            add.push_back(symbol);
        }
        result.emplace_back(add);
        add.clear();
    }
    return result;
}

std::string ColumnTypeInt64::GetTypeName() {
    return "int64";
}

ColumnValue ColumnTypeInt64::ConvertType(std::string val) {
    try {
        return static_cast<int64_t>(stoll(val));
    } catch (...) {
        LOG(ERROR) << val << " is not a int64";
        return 0ll;
    }
}

size_t ColumnTypeInt64::WriteType(std::vector<ColumnValue> data, FileWriter& writer) {
    const size_t block_size = 1000;
    size_t blocks = (data.size() + block_size - 1) / block_size;

    size_t result = 0;
    std::vector<int64_t> min_val(blocks);
    std::vector<int64_t> max_val(blocks);
    std::vector<size_t> sz(blocks, 1);

    for (size_t i = 0; i < data.size(); ++i) {
        if (i % block_size == 0 || min_val[i / block_size] > std::get<int64_t>(data[i])) {
            min_val[i / block_size] = std::get<int64_t>(data[i]);
        }
        if (i % block_size == 0 || max_val[i / block_size] < std::get<int64_t>(data[i])) {
            max_val[i / block_size] = std::get<int64_t>(data[i]);
        }
    }

    size_t tba = sizeof(size_t) + (sizeof(size_t) * 2 + sizeof(int64_t)) * blocks;
    writer.Write(blocks);
    result += sizeof(size_t);

    for (size_t i = 0; i < blocks; ++i) {
        uint64_t delta = static_cast<uint64_t>(max_val[i]) - static_cast<uint64_t>(min_val[i]);
        if (delta != 0) {
            sz[i] = (64 - __builtin_clzll(delta) + 7) / 8;
        }
        writer.Write(min_val[i]);
        result += sizeof(int64_t);
        writer.Write(sz[i]);
        result += sizeof(size_t);
        writer.Write(tba);
        result += sizeof(size_t);
        tba += sz[i] * block_size;
    }
    for (size_t i = 0; i < data.size(); ++i) {
        uint64_t val_norm = static_cast<uint64_t>(std::get<int64_t>(data[i])) -
                            static_cast<uint64_t>(min_val[i / block_size]);
        writer.Write(reinterpret_cast<const char*>(&val_norm), sz[i / block_size]);
        result += sz[i / block_size];
    }
    return result;
}

std::vector<ColumnValue> ColumnTypeInt64::GetBatch(size_t size, FileReader& reader) {
    std::vector<ColumnValue> result;
    std::vector<int64_t> min_val;
    std::vector<size_t> read_sz;
    std::vector<size_t> block_offset;
    size_t blocks = reader.Read<size_t>();

    for (size_t i = 0; i < blocks; ++i) {
        min_val.push_back(reader.Read<int64_t>());
        read_sz.push_back(reader.Read<size_t>());
        block_offset.push_back(reader.Read<size_t>());
    }
    // LOG(INFO) << "size=" << size << " block_offset[0]=" << block_offset[0]
    //      << " blocks=" << blocks << " read_sz=" << read_sz[0] << "\n";
    size_t block = 0;
    size_t i = block_offset[0];
    uint64_t val = 0;
    while (i < size) {
        if (block + 1 != blocks && i >= block_offset[block + 1]) {
            ++block;
            val = 0;
        }
        reader.Read(reinterpret_cast<char*>(&val), read_sz[block]);
        result.push_back(static_cast<int64_t>(val + static_cast<uint64_t>(min_val[block])));
        i += read_sz[block];
        // LOG(INFO) << "result.size()=" << result.size() << " i=" << i << "\n";
    }

    return result;
}

};  // namespace column_engine