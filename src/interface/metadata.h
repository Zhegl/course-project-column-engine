#pragma once
#include <string>
#include <vector>

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
