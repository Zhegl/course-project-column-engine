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

    size_t GetColumnId(const std::string& name);
    std::vector<size_t> GetColumnsForScan();
    std::shared_ptr<FilterPredicate> ParseWhere(const std::string& arg);
    std::vector<AggFactory> ParseAggregate(const std::string& arg);

private:
    const Schema& schema_;
    std::map<std::string, std::pair<size_t, size_t>> column_idx_; // (real idx, out idx)
};

}  // namespace column_engine
