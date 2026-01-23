#pragma once
#include <io/file_reader.h>
#include <io/file_writer.h>
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace column_engine {

using ColumnValue = std::variant<uint64_t, std::string>;

class ColumnTypeName {
public:
    virtual ColumnValue ConvertType(std::string val) = 0;
    virtual std::string GetTypeName() = 0;
    virtual std::vector<ColumnValue> GetBatch(size_t size, FileReader& reader) = 0;
    virtual size_t WriteType(std::vector<ColumnValue> data, FileWriter& writer) = 0;
    virtual ~ColumnTypeName() = default;
};

class ColumnTypeString : public ColumnTypeName {
public:
    ColumnValue ConvertType(std::string val) override;
    std::string GetTypeName() override;
    std::vector<ColumnValue> GetBatch(size_t size, FileReader& reader) override;
    size_t WriteType(std::vector<ColumnValue> data, FileWriter& writer) override;
    ~ColumnTypeString() override = default;
};

class ColumnTypeInt64 : public ColumnTypeName {
public:
    std::string GetTypeName() override;
    ColumnValue ConvertType(std::string val) override;
    std::vector<ColumnValue> GetBatch(size_t size, FileReader& reader) override;
    size_t WriteType(std::vector<ColumnValue> data, FileWriter& writer) override;
    ~ColumnTypeInt64() override = default;
};


//enum class ColumnTypeName { String = 0, Int64 = 1 };

struct ColumnMetaData {
    std::string name;
    std::shared_ptr<ColumnTypeName> type;
};

struct BatchMetaData {
    size_t size;
    size_t offset;
};

struct Scheme {
    std::vector<ColumnMetaData> columns;
};

std::string ColumnTypeToString(ColumnValue data);

std::shared_ptr<ColumnTypeName> GetType(const std::string& name);

};  // namespace column_engine