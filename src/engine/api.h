#pragma once
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "types/types.h"
#include <engine/queries.h>
#include <engine/batch.h>
#include <engine/parser.h>

namespace column_engine {

using QueryResult = std::vector<std::vector<std::string>>;

class Operator;

class Engine;

class Scan;

class ApiPipeline {
public:
    explicit ApiPipeline(Engine& engine, const Schema& schema);
    ApiPipeline Count(std::string arg);
    ApiPipeline Where(std::string arg);
    ApiPipeline Aggregate(std::string arg);
    ApiPipeline Limit(size_t arg);
    ApiPipeline Offset(size_t arg);
    ApiPipeline OrderBy(std::string arg);
    ApiPipeline Rename(std::string from, std::string to);
    template <typename... Args>
    ApiPipeline GroupByAggregate(Args... args) {
        std::vector<std::string> all_args{std::string(args)...};
        std::string aggregates = all_args.back();
        all_args.pop_back();
        return GroupByAggregateImpl(std::move(all_args), std::move(aggregates));
    }
    template <typename... Args>
    ApiPipeline Select(Args... args) {
        for (const auto& name : {std::string(args)...}) {
            selected_columns_.push_back(parser_.GetColumnId(name));
        }
        return *this;
    }
    QueryResult Run();

private:
    ApiPipeline GroupByAggregateImpl(std::vector<std::string> group_columns, std::string aggregates);
    void MaterializePendingOrder();
    Engine& engine_;
    QueryParser parser_;
    std::vector<size_t> selected_columns_;
    std::shared_ptr<Operator> root_;
    std::shared_ptr<Scan> scanner_;
    std::optional<std::string> pending_order_col_;
    bool pending_order_reversed_ = false;
    size_t pending_offset_ = 0;
};
}  // namespace column_engine
