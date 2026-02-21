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
    size_t result = 0;
    int64_t min_val = std::get<int64_t>(data[0]);
    int64_t max_val = std::get<int64_t>(data[0]);
    for (auto val : data) {
        min_val = std::min(min_val, std::get<int64_t>(val));
        max_val = std::max(max_val, std::get<int64_t>(val));
    }

    uint64_t delta = (max_val - min_val);
    size_t sz = 1;

    if (delta != 0) {
        sz = (64 - __builtin_clzll(delta) + 7) / 8;
    }

    writer.Write(min_val);
    result += sizeof(int64_t);
    writer.Write(sz);
    result += sizeof(size_t);
    for (auto val : data) {
        uint64_t val_norm = std::get<int64_t>(val) - min_val;
        writer.Write(reinterpret_cast<const char*>(&val_norm), sz);
        result += sz;
    }
    return result;
}

std::vector<ColumnValue> ColumnTypeInt64::GetBatch(size_t size, FileReader& reader) {
    std::vector<ColumnValue> result;
    int64_t min_val = reader.Read<int64_t>();
    size -= sizeof(int64_t);
    size_t read_sz = reader.Read<size_t>();
    size -= sizeof(size_t);
    uint64_t val = 0;
    while (size) {
        reader.Read(reinterpret_cast<char*>(&val), read_sz);
        result.push_back(static_cast<int64_t>(val) + min_val);
        size -= read_sz;
    }
    return result;
}

};  // namespace column_engine