#include "api.h"
#include "column_utils.h"
#include <memory>
#include <stdexcept>
#include <string>
#include "engine.h"
#include "types/types.h"

namespace column_engine::internal {

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
            std::vector<AggFactory>{{[]() { return std::make_unique<CountAll>(); }, true}},
            std::vector<std::string>{"COUNT(*)"},
            GroupKeyType::Multi);
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
    size_t comma = std::string::npos;
    {
        int depth = 0;
        for (size_t i = 0; i + 1 < arg.size(); ++i) {
            if (arg[i] == '(') { ++depth; }
            else if (arg[i] == ')') { --depth; }
            else if (depth == 0 && arg[i] == ',' && arg[i+1] == ' ') { comma = i; break; }
        }
    }
    std::string first = (comma != std::string::npos) ? arg.substr(0, comma) : arg;
    std::string second = (comma != std::string::npos) ? arg.substr(comma + 2) : "";

    size_t start = 4;
    bool reversed = false;
    if (first[first.size() - 3] != 'A') {
        start = 5;
        reversed = true;
    }
    pending_order_col_ = first.substr(0, first.size() - start);
    pending_order_reversed_ = reversed;

    if (!second.empty()) {
        size_t start2 = (second.size() >= 5 && second[second.size() - 3] != 'A') ? 5 : 4;
        pending_order_col2_ = second.substr(0, second.size() - start2);
    } else {
        pending_order_col2_.reset();
    }

    return *this;
}

ApiPipeline ApiPipeline::Offset(size_t arg) {
    if (pending_order_col_) {
        pending_offset_ = arg;
    } else {
        root_ = std::make_shared<OffsetOp>(root_, arg);
    }
    return *this;
}

ApiPipeline ApiPipeline::Limit(size_t arg) {
    if (pending_order_col_) {
        size_t col_idx = parser_.GetColumnId(*pending_order_col_);
        if (pending_order_col2_) {
            size_t col_idx2 = parser_.GetColumnId(*pending_order_col2_);
            root_ = std::make_shared<TopK>(root_, col_idx, col_idx2, pending_order_reversed_, arg + pending_offset_);
            pending_order_col2_.reset();
        } else {
            root_ = std::make_shared<TopK>(root_, col_idx, pending_order_reversed_, arg + pending_offset_);
        }
        if (pending_offset_ > 0) {
            root_ = std::make_shared<OffsetOp>(root_, pending_offset_);
            pending_offset_ = 0;
        }
        pending_order_col_.reset();
    } else {
        root_ = std::make_shared<LimitOp>(root_, arg);
    }
    return *this;
}

ApiPipeline ApiPipeline::Rename(std::string from, std::string to) {
    size_t id = parser_.GetColumnId(from);
    Schema new_schema = parser_.GetSchema();
    new_schema.columns[id].name = to;
    parser_.SetSchema(new_schema);
    root_ = std::make_shared<As>(root_, from, to);
    return *this;
}

ApiPipeline ApiPipeline::Add(std::string arg) {
    auto [func, is_int] = parser_.ParseAdd(arg);

    Schema new_schema = parser_.GetSchema();
    size_t insert_idx = new_schema.columns.size();
    ColumnMetaData meta;
    meta.name = func->GetName();

    if (is_int) {
        meta.type = std::make_shared<ColumnTypeInt64>();
        new_schema.columns.push_back(std::move(meta));
        root_ = std::make_shared<AddCol<int64_t>>(root_, func, insert_idx);
    } else {
        meta.type = std::make_shared<ColumnTypeString>();
        new_schema.columns.push_back(std::move(meta));
        root_ = std::make_shared<AddCol<std::string>>(root_, func, insert_idx);
    }
    parser_.SetSchema(new_schema);
    return *this;
}

ApiPipeline ApiPipeline::Case(std::string name, std::string when_cond, std::string then_expr, std::string else_expr) {
    auto pred = parser_.ParseWhere(when_cond);
    auto [then_fun, then_is_int] = parser_.ParseAdd(then_expr);
    auto [else_fun, else_is_int] = parser_.ParseAdd(else_expr);
    bool is_int = then_is_int || else_is_int;

    Schema new_schema = parser_.GetSchema();
    size_t insert_idx = new_schema.columns.size();
    ColumnMetaData meta;
    meta.name = name;
    if (is_int) {
        meta.type = std::make_shared<ColumnTypeInt64>();
        new_schema.columns.push_back(std::move(meta));
        root_ = std::make_shared<AddCase<int64_t>>(root_, then_fun, else_fun, insert_idx, pred, name);
    } else {
        meta.type = std::make_shared<ColumnTypeString>();
        new_schema.columns.push_back(std::move(meta));
        root_ = std::make_shared<AddCase<std::string>>(root_, then_fun, else_fun, insert_idx, pred, name);
    }
    parser_.SetSchema(new_schema);
    return *this;
}

ApiPipeline ApiPipeline::GroupByAggregateImpl(std::vector<std::string> group_columns, std::string aggregates) {
    std::vector<size_t> group_column_ids;
    group_column_ids.reserve(group_columns.size());
    for (const auto& name : group_columns) {
        group_column_ids.push_back(parser_.GetColumnId(name));
    }
    Schema cur_schema = parser_.GetSchema();

    auto [agg_factories, agg_columns] = parser_.ParseAggregate(aggregates);

    std::vector<std::string> agg_names;
    agg_names.reserve(agg_columns.size());
    for (const auto& col : agg_columns) {
        agg_names.push_back(col.name);
    }

    Schema new_schema;
    for (size_t id : group_column_ids) {
        new_schema.columns.push_back(cur_schema.columns[id]);
    }
    for (auto& col : agg_columns) {
        new_schema.columns.push_back(std::move(col));
    }
    parser_.SetSchema(new_schema);

    GroupKeyType key_type = GroupKeyType::Multi;
    if (group_columns.size() == 1) {
        const auto& type_name = cur_schema.columns[group_column_ids[0]].type->GetTypeName();
        key_type = (type_name == "int64") ? GroupKeyType::Int : GroupKeyType::Str;
    }

    root_ = std::make_shared<class Aggregate>(
        root_,
        std::move(group_column_ids),
        std::move(group_columns),
        std::move(agg_factories),
        std::move(agg_names),
        key_type);




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
    if (batch.names.empty() && !selected_columns_.empty()) {
        Schema cur = parser_.GetSchema();
        std::vector<std::string> names;
        for (size_t idx : selected_columns_) {
            names.push_back(cur.columns[idx].name);
        }
        result.push_back(std::move(names));
        return result;
    }
    result.push_back(batch.names);
    for (auto i : batch.selection) {
        result.emplace_back();
        for (size_t col = 0; col < result[0].size(); ++col) {
            result.back().emplace_back(ColumnValueToStringAt(batch.columns[col], i));
        }
    }
    return result;
}

}  // namespace column_engine::internal
