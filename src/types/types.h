#pragma once
#include <io/file_reader.h>
#include <io/file_writer.h>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace column_engine {

struct ScanOptions;

using ColumnValue = std::variant<int64_t, std::string>;
using ColumnData = std::variant<std::vector<int64_t>, std::vector<std::string_view>, std::vector<std::string>>;

class ColumnTypeName {
public:
    virtual ColumnValue ConvertType(std::string val) = 0;
    virtual std::string GetTypeName() const = 0;
    virtual ColumnData GetBatch(size_t size, FileReader& reader, const ScanOptions* options = nullptr) = 0;
    virtual size_t WriteType(const std::vector<ColumnValue>& data, FileWriter& writer) = 0;
    virtual ~ColumnTypeName() = default;
};

class ColumnTypeString : public ColumnTypeName {
public:
    ColumnValue ConvertType(std::string val) override;
    std::string GetTypeName() const override;
    ColumnData GetBatch(size_t size, FileReader& reader, const ScanOptions* options = nullptr) override;
    size_t WriteType(const std::vector<ColumnValue>& data, FileWriter& writer) override;
    ~ColumnTypeString() override = default;
};

class ColumnTypeInt64 : public ColumnTypeName {
public:
    std::string GetTypeName() const override;
    ColumnValue ConvertType(std::string val) override;
    [[gnu::always_inline]] inline ColumnData GetBatch(size_t size, FileReader& reader, const ScanOptions* options = nullptr) override;
    size_t WriteType(const std::vector<ColumnValue>& data, FileWriter& writer) override;
    ~ColumnTypeInt64() override = default;
};

struct ColumnMetaData {
    std::string name;
    std::shared_ptr<ColumnTypeName> type;
};

struct BatchMetaData {
    size_t size;
    size_t offset;
};

struct Schema {
    std::vector<ColumnMetaData> columns;
};

std::string ColumnTypeToString(ColumnValue data);

std::shared_ptr<ColumnTypeName> GetType(const std::string& name);

};  // namespace column_engine
