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
    if (std::holds_alternative<uint64_t>(data)) {
        return std::to_string(std::get<uint64_t>(data));
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
        return static_cast<uint64_t>(stoull(val));
    } catch (...) {
        LOG(ERROR) << val << " is not a int64";
        return 0ull;
    }
}

size_t ColumnTypeInt64::WriteType(std::vector<ColumnValue> data, FileWriter& writer) {
    size_t result = 0;
    for (auto val : data) {
        writer.Write(std::get<uint64_t>(val));
        result += sizeof(uint64_t);
    }
    return result;
}

std::vector<ColumnValue> ColumnTypeInt64::GetBatch(size_t size, FileReader& reader) {
    std::vector<ColumnValue> result;
    while (size) {
        result.push_back(reader.Read<uint64_t>());
        size -= sizeof(uint64_t);
    }
    return result;
}

};  // namespace column_engine