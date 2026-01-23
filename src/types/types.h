#pragma once
#include <io/file_reader.h>
#include <io/file_writer.h>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace column_engine {

enum class ColumnTypeName { String = 0, Int64 = 1 };

struct ColumnMetaData {
    std::string name;
    ColumnTypeName type;
};

struct BatchMetaData {
    size_t size;
    size_t offset;
};

struct Scheme {
    std::vector<ColumnMetaData> columns;
};

using ColumnValue = std::variant<uint64_t, std::string>;

ColumnValue ConvertType(std::string val, ColumnTypeName type);

std::string ColumnTypeToString(ColumnValue data);

std::string GetTypeName(ColumnTypeName type);

ColumnTypeName GetType(const std::string& name);

size_t WriteType(std::vector<ColumnValue> data, ColumnTypeName type, FileWriter& writer);

std::vector<ColumnValue> GetBatch(size_t size, ColumnTypeName type, FileReader& reader);

};  // namespace column_engine