#include "api.h"
#include "column_utils.h"
#include <memory>
#include <stdexcept>
#include <string>
#include "engine.h"
#include "queries.h"
#include "types/types.h"

namespace column_engine {

ApiPipeline::ApiPipeline(Engine& engine, const Schema& schema)
    : engine_(engine), parser_(schema) {
    scanner_ = engine.MakeScan();
    root_ = scanner_;
}

ApiPipeline ApiPipeline::Count(std::string arg) {
    if (arg == "*") {
        root_ = std::make_shared<class Aggregate>(
            root_,
            std::vector<size_t>{},
            std::vector<std::string>{},
            std::vector<AggFactory>{[]() { return std::make_shared<CountAll>(); }});
        return *this;
    }
    throw std::runtime_error("Wrong arg for .Count: " + arg);
}

ApiPipeline ApiPipeline::Where(std::string arg) {
    root_ = std::make_shared<Filter>(root_, parser_.ParseWhere(arg));
    return *this;
}

ApiPipeline ApiPipeline::Aggregate(std::string arg) {
    return GroupByAggregateImpl({}, std::move(arg));
}

ApiPipeline ApiPipeline::OrderBy(std::string arg) {
    if (arg.size() < 4) {
        throw std::runtime_error("wrong args for OrderBy");
    }


    size_t start = 4;
    bool reversed = false;

    if (arg[arg.size() - 3] != 'A') { // TODO better parser
        start = 5;
        reversed = true;
    } 

    pending_order_col_ = arg.substr(0, arg.size() - start);
    pending_order_reversed_ = reversed;
    return *this;
}

ApiPipeline ApiPipeline::Limit(size_t arg) {
    if (pending_order_col_) {
        size_t col_idx = parser_.GetColumnId(*pending_order_col_);
        root_ = std::make_shared<TopK>(root_, col_idx, pending_order_reversed_, arg);
        pending_order_col_.reset();
    } else {
        root_ = std::make_shared<LimitOp>(root_, arg);
    }
    return *this;
}

ApiPipeline ApiPipeline::Rename(std::string from, std::string to) {
    Schema new_schema = parser_.GetSchema();
    new_schema.columns[parser_.GetColumnId(from)].name = to;
    parser_.SetSchema(new_schema);
    root_ = std::make_shared<As>(root_, from, to);
    return *this;
}

ApiPipeline ApiPipeline::GroupByAggregateImpl(std::vector<std::string> group_columns, std::string aggregates) {
    std::vector<size_t> group_column_ids;
    group_column_ids.reserve(group_columns.size());
    for (const auto& name : group_columns) {
        group_column_ids.push_back(parser_.GetColumnId(name));
    }
    Schema old_schema = parser_.GetSchema();
    Schema new_schema;
    for (size_t id : group_column_ids) {
        new_schema.columns.push_back(old_schema.columns[id]);
    }

    parser_.SetSchema(new_schema);

    size_t schema_before_agg = new_schema.columns.size();
    auto agg_factories = parser_.ParseAggregate(aggregates);

    Schema agg_schema;
    Schema full_schema = parser_.GetSchema();
    for (size_t i = 0; i < schema_before_agg; ++i) {
        agg_schema.columns.push_back(full_schema.columns[i]);
    }
    for (size_t i = full_schema.columns.size() - agg_factories.size(); i < full_schema.columns.size(); ++i) {
        agg_schema.columns.push_back(full_schema.columns[i]);
    }
    parser_.SetSchema(agg_schema);

    root_ = std::make_shared<class Aggregate>(
        root_,
        std::move(group_column_ids),
        std::move(group_columns),
        std::move(agg_factories));




    return *this;
}

void ApiPipeline::MaterializePendingOrder() {
    if (pending_order_col_) {
        size_t col_idx = parser_.GetColumnId(*pending_order_col_);
        root_ = std::make_shared<Sort>(root_, col_idx, pending_order_reversed_);
        pending_order_col_.reset();
    }
}

QueryResult ApiPipeline::Run() {
    MaterializePendingOrder();
    scanner_->SetColumns(parser_.GetColumnsForScan());
    EngineBatch batch = engine_.Run(root_, selected_columns_);
    QueryResult result;
    result.push_back(batch.names);
    for (auto i : batch.selection) {
        result.emplace_back();
        for (size_t col = 0; col < result[0].size(); ++col) {
            result.back().emplace_back(ColumnValueToStringAt(batch.columns[col], i));
        }
    }
    return result;
}

}  // namespace column_engine
