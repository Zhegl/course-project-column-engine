#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "queries.h"
#include "types/types.h"

namespace column_engine {

using AggFactory = std::function<std::shared_ptr<Aggregator>()>;

class QueryParser {
public:
    explicit QueryParser(const Schema& schema);
    void SetSchema(Schema schema);
    Schema GetSchema();
    size_t GetColumnId(const std::string& name);
    std::vector<size_t> GetColumnsForScan();
    std::shared_ptr<FilterPredicate> ParseWhere(const std::string& arg);
    std::pair<std::vector<AggFactory>, std::vector<ColumnMetaData>> ParseAggregate(const std::string& arg);

private:
    const Schema& schema_;
    Schema cur_schema_;
    std::vector<size_t> columns_for_scan_;
};

}  // namespace column_engine
