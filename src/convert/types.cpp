#include "types.h"
#include <sys/types.h>
#include <string>
#include "metadata.h"

ColumnType ConvertType(std::string val, ColumnTypeName type) {
    if (type == ColumnTypeName::Int64) {
        return static_cast<uint64_t>(stoull(val));
    } else {
        return val;
    }
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