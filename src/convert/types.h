#pragma once
#include "metadata.h"
#include "file_writer.h"
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

using ColumnType = std::variant<uint64_t, std::string>;

ColumnType ConvertType(std::string val, ColumnTypeName type);

size_t WriteType(std::vector<ColumnType> data, ColumnTypeName type, FileWriter& writer);