#pragma once
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "types/types.h"
#include <engine/batch.h>
#include <engine/parser.h>

namespace column_engine::internal {

using QueryResult = std::vector<std::vector<std::string>>;

class Operator;

class Engine;

class ScanOp;

class Pipeline {
public:
    explicit Pipeline(Engine& engine, const Schema& schema);
    Pipeline Count(std::string arg);
    Pipeline Where(std::string arg);
    Pipeline Aggregate(std::string arg);
    Pipeline Limit(size_t arg);
    Pipeline Offset(size_t arg);
    Pipeline OrderBy(std::string arg);
    Pipeline Rename(std::string from, std::string to);
    Pipeline Add(std::string arg);
    Pipeline Case(std::string name, std::string when_cond, std::string then_expr, std::string else_expr);
    template <typename... Args>
    Pipeline GroupByAggregate(Args... args) {
        std::vector<std::string> all_args{std::string(args)...};
        std::string aggregates = all_args.back();
        all_args.pop_back();
        return GroupByAggregateImpl(std::move(all_args), std::move(aggregates));
    }
    template <typename... Args>
    Pipeline Select(Args... args) {
        for (const auto& name : {std::string(args)...}) {
            if (name == "*") {
                selected_columns_.clear();
                for (auto col : parser_.GetRealSchema().columns) {
                    selected_columns_.push_back(parser_.GetColumnId(col.name));
                }
                break;
            } else {
                selected_columns_.push_back(parser_.GetColumnId(name));
            }
        }
        return *this;
    }
    Pipeline SelectVec(const std::vector<std::string>& columns) {
        for (const auto& name : columns) {
            Select(name);
        }
        return *this;
    }
    Pipeline GroupByAggregateVec(std::vector<std::string> all_args) {
        std::string aggregates = all_args.back();
        all_args.pop_back();
        return GroupByAggregateImpl(std::move(all_args), std::move(aggregates));
    }
    QueryResult Run();

private:
    Pipeline GroupByAggregateImpl(std::vector<std::string> group_columns, std::string aggregates);
    void MaterializePendingOrder();
    Engine& engine_;
    QueryParser parser_;
    std::vector<size_t> selected_columns_;
    std::shared_ptr<Operator> root_;
    std::shared_ptr<ScanOp> scanner_;
    std::optional<std::string> pending_order_col_;
    std::optional<std::string> pending_order_col2_;
    bool pending_order_reversed_ = false;
    size_t pending_offset_ = 0;
};

}  // namespace column_engine::internal
