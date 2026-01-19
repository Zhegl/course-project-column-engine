#include "types.h"
#include <sys/types.h>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include <interface/metadata.h>

ColumnType ConvertType(std::string val, ColumnTypeName type) {
    if (type == ColumnTypeName::Int64) {
        return static_cast<uint64_t>(stoull(val));
    } else {
        return val;
    }
}

ColumnTypeName GetType(const std::string& name) {
    if (name == "int64") {
        return ColumnTypeName::Int64;
    } else if (name == "string") {
        return ColumnTypeName::String;
    }
    throw std::runtime_error("Unknown type: " + name);
}

std::string GetTypeName(ColumnTypeName type) {
    if (type == ColumnTypeName::Int64) {
        return "int64";
    } else if (type == ColumnTypeName::String) {
        return "string";
    }
    throw std::runtime_error("Unknown type in GetTypeName");
}

size_t WriteType(std::vector<ColumnType> data, ColumnTypeName type, FileWriter& writer) {
    size_t result = 0;
    if (type == ColumnTypeName::Int64) {
        for (auto val : data) {
            writer.Write(std::get<uint64_t>(val));
            result += sizeof(uint64_t);
        }
    } else {
        for (auto val : data) {
            writer.Write(std::get<std::string>(val).data(), std::get<std::string>(val).size() + 1);
            result += std::get<std::string>(val).size() + 1;
        }
    }
    return result;
}


std::string ColumnTypeToString(ColumnType data) {
    if (std::holds_alternative<uint64_t>(data)) {
        return std::to_string(std::get<uint64_t>(data));
    } else if (std::holds_alternative<std::string>(data)) {
        return std::get<std::string>(data);
    }
    throw std::runtime_error("Unknown type in ColumnTypeToString");
}

std::vector<ColumnType> GetBatch(size_t size, ColumnTypeName type, FileReader& reader) {
    std::vector<ColumnType> result;
    if (type == ColumnTypeName::Int64) {
        while (size) {
            result.push_back(reader.Read<uint64_t>());
            size -= sizeof(uint64_t);
        }
    } else if (type == ColumnTypeName::String) {
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
    } else {
        throw std::runtime_error("Unknown type in GetBatch");
    }
    return result;
}