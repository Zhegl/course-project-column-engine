#include "engine.h"
#include "api.h"
#include <format/meta_reader.h>
#include <io/file_reader.h>
#include <glog/logging.h>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "batch.h"

namespace column_engine {

namespace {

bool LessColumnValue(const ColumnValue& lhs, const ColumnValue& rhs) {
    return std::visit(
        [](const auto& left, const auto& right) -> bool {
            using L = std::decay_t<decltype(left)>;
            using R = std::decay_t<decltype(right)>;
            if constexpr (std::is_same_v<L, R>) {
                return left < right;
            }
            throw std::runtime_error("Column type mismatch in comparison");
        },
        lhs, rhs);
}

ColumnValue GetColumnValue(const ColumnData& column, RowIndex i) {
    return std::visit([&](const auto& data) -> ColumnValue { return data[i]; }, column);
}

ColumnData MakeColumnData(const ColumnValue& value) {
    return std::visit(
        [](const auto& v) -> ColumnData {
            using T = std::decay_t<decltype(v)>;
            return std::vector<T>{};
        },
        value);
}

void AppendColumnValue(ColumnData& column, const ColumnValue& value) {
    std::visit(
        [&](auto& data) {
            using T = typename std::decay_t<decltype(data)>::value_type;
            data.push_back(std::get<T>(value));
        },
        column);
}

std::optional<EngineBatch> GetAllBatches(std::shared_ptr<Operator> op, size_t limit = 0) {
    EngineBatch result;
    while (auto batch = op->GetNext()) {
        if (result.names.empty()) {
            result.names = batch->names;
            for (const auto& col : batch->columns) {
                result.columns.push_back(std::visit(
                    [](const auto& v) -> ColumnData { return std::decay_t<decltype(v)>{}; }, col));
            }
        }
        for (size_t col = 0; col < batch->columns.size(); ++col) {
            std::visit(
                [&](auto& dst, const auto& src) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(dst)>,
                                                 std::decay_t<decltype(src)>>) {
                        for (auto i : batch->selection) {
                            if (limit != 0 && dst.size() >= limit) {
                                break;
                            }
                            dst.push_back(src[i]);
                        }
                    }
                },
                result.columns[col], batch->columns[col]);
        }
        for (size_t k = 0; k < batch->selection.size() && (!limit || result.selection.size() < limit); ++k) {
            result.selection.push_back(result.selection.size());
        }
    }
    if (result.columns.empty()) {
        return std::nullopt;
    }
    return result;
}

}  // namespace

size_t GetColumnIndex(const EngineBatch& batch, const std::string& name) {
    for (size_t i = 0; i < batch.names.size(); ++i) {
        if (batch.names[i] == name) {
            return i;
        }
    }
    throw std::runtime_error("Key error: " + name);
}

void Operator::SetChild(std::shared_ptr<Operator> child) {
    child_ = child;
}

Scan::Scan(const std::string& path, Schema schema, std::vector<BatchMetaData> batch_meta)
    : path_(path),
      schema_(std::move(schema)),
      batch_meta_(std::move(batch_meta)),
      num_row_groups_(batch_meta_.size() / schema_.columns.size()) {
}

void Scan::SetColumns(std::vector<size_t> columns) {
    columns_ = columns;
}

std::optional<EngineBatch> Scan::GetNext() {
    if (current_row_group_ >= num_row_groups_) {
        return std::nullopt;
    }

    size_t cols = schema_.columns.size();
    size_t rg = current_row_group_++;

    FileReader reader(path_);
    EngineBatch result;
    for (size_t col : columns_) {
        const auto& meta = batch_meta_[rg * cols + col];
        reader.Jump(meta.offset - reader.GetPos());
        result.names.push_back(schema_.columns[col].name);
        result.columns.emplace_back(schema_.columns[col].type->GetBatch(meta.size, reader));
    }

    size_t num_rows = std::visit([](const auto& c) { return c.size(); }, result.columns[0]);
    result.selection.resize(num_rows);
    for (RowIndex i = 0; i < num_rows; ++i) {
        result.selection[i] = i;
    }
    return result;
}

// Filter

std::optional<EngineBatch> Filter::GetNext() {
    while (auto batch = child_->GetNext()) {
        std::vector<RowIndex> new_selection;
        for (auto i : batch->selection) {
            if (pred_->Check(*batch, i)) {
                new_selection.emplace_back(i);
            }
        }
        batch->selection = std::move(new_selection);
        if (!batch->selection.empty()) {
            return batch;
        }
    }
    return std::nullopt;
}

// Aggregate
std::optional<EngineBatch> Aggregate::GetNext() {
    HashMap<std::vector<std::shared_ptr<Aggregator>>> groups;
    bool is_empty = true;
    while (auto batch = child_->GetNext()) {
        is_empty &= batch->selection.empty();
        for (auto i : batch->selection) {
            GroupKey key;
            key.values.reserve(group_columns_.size());
            for (size_t col : group_columns_) {
                key.values.push_back(GetColumnValue(batch->columns[col], i));
            }

            auto& aggs = groups[key];
            if (aggs.empty()) {
                for (auto& f : factories_) {
                    aggs.push_back(f());
                }
            }
            for (auto& agg : aggs) {
                agg->Next(*batch, i);
            }
        }
    }
    if (is_empty) {
        return std::nullopt;
    }

    if (groups.empty()) {
        groups[GroupKey{}];
    }

    EngineBatch result;
    for (const auto& name : group_names_) {
        result.names.push_back(name);
    }
    if (!groups.empty()) {
        const auto& first_key = groups.begin()->first;
        for (const auto& value : first_key.values) {
            result.columns.push_back(MakeColumnData(value));
        }
        for (const auto& agg : groups.begin()->second) {
            result.names.push_back(agg->GetName());
            result.columns.push_back(agg->GetResult());
            std::visit([](auto& data) { data.clear(); }, result.columns.back());
        }
    }

    for (const auto& [key, aggs] : groups) {
        for (size_t i = 0; i < key.values.size(); ++i) {
            AppendColumnValue(result.columns[i], key.values[i]);
        }
        for (size_t i = 0; i < aggs.size(); ++i) {
            AppendColumnValue(result.columns[group_names_.size() + i],
                              GetColumnValue(aggs[i]->GetResult(), 0));
        }
    }

    size_t n = 0;
    if (!result.columns.empty()) {
        n = std::visit([](const auto& vec) { return vec.size(); }, result.columns[0]);
    } else if (!groups.empty()) {
        n = groups.size();
    }
    result.selection.resize(n);
    std::iota(result.selection.begin(), result.selection.end(), 0);
    return result;
}

std::optional<EngineBatch> LimitOp::GetNext() {
    return GetAllBatches(child_, limit_);
}

std::optional<EngineBatch> Sort::GetNext() {
    if (auto result = GetAllBatches(child_)) {
        size_t col_id = GetColumnIndex(result.value(), col_);
        const ColumnData& col = result->columns[col_id];
        std::sort(result->selection.begin(), result->selection.end(),
                  [&](RowIndex lhs, RowIndex rhs) {
                      return std::visit(
                          [&](const auto& values) { return values[lhs] < values[rhs]; }, col);
                  });
        if (reversed_) {
            std::reverse(result->selection.begin(), result->selection.end());
        }
        return result;
    }
    return std::nullopt;
}

std::optional<EngineBatch> As::GetNext() {
    if (auto batch = child_->GetNext()) {
        for (auto& col : batch->names) {
            if (col == from_) {
                col = to_;
            }
        }
        return batch;
    }
    return std::nullopt;
}


std::optional<EngineBatch> TopK::GetNext() {
    if (limit_ == 0) {
        return GetAllBatches(child_, 0);
    }

    struct TopKRow {
        std::vector<ColumnValue> values;
        size_t order;
    };

    size_t sort_col_idx = 0;

    auto is_better = [&](const TopKRow& lhs, const TopKRow& rhs) {
        const ColumnValue& left = lhs.values[sort_col_idx];
        const ColumnValue& right = rhs.values[sort_col_idx];
        if (LessColumnValue(left, right)) {
            return !reversed_;
        }
        if (LessColumnValue(right, left)) {
            return reversed_;
        }
        return lhs.order < rhs.order;
    };


    std::vector<TopKRow> heap_storage;
    size_t next_order = 0;
    bool initialized = false;
    std::vector<std::string> names;
    std::vector<ColumnData> schema_columns;

    auto heap_cmp = [&](size_t lhs, size_t rhs) {
        return is_better(heap_storage[lhs], heap_storage[rhs]);
    };
    std::priority_queue<size_t, std::vector<size_t>, decltype(heap_cmp)> heap(heap_cmp);

    while (auto batch = child_->GetNext()) {
        if (!initialized) {
            names = batch->names;
            schema_columns = batch->columns;
            sort_col_idx = GetColumnIndex(*batch, col_);
            initialized = true;
        }
        for (auto i : batch->selection) {
            TopKRow row;
            row.order = next_order++;
            row.values.reserve(batch->columns.size());
            for (const auto& column : batch->columns) {
                row.values.push_back(GetColumnValue(column, i));
            }

            heap_storage.push_back(std::move(row));
            size_t row_idx = heap_storage.size() - 1;
            if (heap.size() < limit_) {
                heap.push(row_idx);
                continue;
            }
            if (is_better(heap_storage[row_idx], heap_storage[heap.top()])) {
                heap.pop();
                heap.push(row_idx);
            }
        }
    }

    if (!initialized) {
        return std::nullopt;
    }

    std::vector<size_t> best_rows;
    best_rows.reserve(heap.size());
    while (!heap.empty()) {
        best_rows.push_back(heap.top());
        heap.pop();
    }

    std::sort(best_rows.begin(), best_rows.end(), [&](size_t lhs, size_t rhs) {
        return is_better(heap_storage[lhs], heap_storage[rhs]);
    });

    EngineBatch result;
    result.names = std::move(names);
    for (const auto& col : schema_columns) {
        result.columns.push_back(std::visit(
            [](const auto& v) -> ColumnData { return std::decay_t<decltype(v)>{}; }, col));
    }
    for (size_t row_idx : best_rows) {
        for (size_t col = 0; col < heap_storage[row_idx].values.size(); ++col) {
            AppendColumnValue(result.columns[col], heap_storage[row_idx].values[col]);
        }
        result.selection.push_back(result.selection.size());
    }
    return result;
}

// Engine

Engine::Engine(const std::string& path) : path_(path) {
    auto [batch_meta, schema] = GetMeta(path_);
    batch_meta_ = std::move(batch_meta);
    schema_ = std::move(schema);
    LOG(INFO) << "Engine started, " << schema_.columns.size() << " columns, "
              << batch_meta_.size() / schema_.columns.size() << " row groups";
}

std::shared_ptr<Scan> Engine::MakeScan() {
    return std::make_shared<Scan>(path_, schema_, batch_meta_);
}

EngineBatch Engine::Run(std::shared_ptr<Operator> root,
                        const std::vector<std::string>& selected_columns) {

    if (auto probe = GetAllBatches(root)) {
        EngineBatch result = probe.value();
        if (selected_columns.empty()) {
            return result;
        }

        EngineBatch selected_result;
        if (result.names.empty()) {
            selected_result.names = selected_columns;
            return selected_result;
        }

        selected_result.selection = result.selection;
        for (const auto& name : selected_columns) {
            size_t col = GetColumnIndex(result, name);
            selected_result.names.push_back(result.names[col]);
            selected_result.columns.push_back(result.columns[col]);
        }
        return selected_result;
    }
    EngineBatch result;
    result.names = selected_columns;
    return result;
}

ApiPipeline Engine::Api() {
    return ApiPipeline(*this, schema_);
}

}  // namespace column_engine
