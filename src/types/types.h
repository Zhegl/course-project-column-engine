#pragma once
#include <io/file_reader.h>
#include <interface/metadata.h>
#include <io/file_writer.h>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace column_engine {

using ColumnType = std::variant<uint64_t, std::string>;

ColumnType ConvertType(std::string val, ColumnTypeName type);

std::string ColumnTypeToString(ColumnType data);

std::string GetTypeName(ColumnTypeName type);

ColumnTypeName GetType(const std::string& name);

size_t WriteType(std::vector<ColumnType> data, ColumnTypeName type, FileWriter& writer);

std::vector<ColumnType> GetBatch(size_t size, ColumnTypeName type, FileReader& reader);

};