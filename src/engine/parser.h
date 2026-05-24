#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "engine/aggregators.h"
#include "engine/bloom.h"
#include "engine/expressions.h"
#include "engine/predicates.h"
#include "types/types.h"
#include "types/scan_options.h"

namespace column_engine::internal {

struct AggFactory {
    std::function<std::unique_ptr<Aggregator>()> make;
    bool is_count_all{false};
};

class QueryParser {
public:
    explicit QueryParser(const Schema& schema);
    void SetSchema(Schema schema);
    Schema GetSchema();
    Schema GetRealSchema();
    size_t GetColumnId(const std::string& name);
    std::vector<size_t> GetColumnsForScan();
    std::vector<std::shared_ptr<ScanOptions>> GetScanOptions();
    std::shared_ptr<FilterPredicate> ParseWhere(const std::string& arg);
    std::pair<std::vector<AggFactory>, std::vector<ColumnMetaData>> ParseAggregate(const std::string& arg);
    // returns {function, is_int_result}
    std::pair<std::shared_ptr<AddColFun>, bool> ParseAdd(const std::string& arg);

private:
    const Schema& schema_;
    Schema cur_schema_;
    std::vector<size_t> columns_for_scan_;
    std::unordered_map<size_t, std::shared_ptr<StringScanOptions>> str_scan_options_;
};

}  // namespace column_engine::internal
