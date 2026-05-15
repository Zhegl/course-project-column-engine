#include "engine.h"
#include "api.h"
#include "column_utils.h"
#include <format/meta_reader.h>
#include <io/file_reader.h>
#include <glog/logging.h>
#include <algorithm>
#include <memory>
#include <optional>
#include <vector>
#include "batch.h"

namespace column_engine::internal {

namespace {

std::optional<EngineBatch> GetAllBatches(std::shared_ptr<Operator> op, size_t limit = 65535) {
    EngineBatch result;
    while (auto batch = op->GetNext()) {
        if (result.names.empty()) {
            result.names = batch->names;
            for (const auto& col : batch->columns) {
                result.columns.push_back(MakeEmptyColumnLike(col));
            }
        }
        for (size_t col = 0; col < batch->columns.size(); ++col) {
            AppendSelectedValues(result.columns[col], batch->columns[col], batch->selection, limit);
        }
        for (size_t k = 0; k < batch->selection.size() && result.selection.size() < limit; ++k) {
            result.selection.push_back(result.selection.size());
        }
    }
    if (result.columns.empty()) {
        return std::nullopt;
    }
    return result;
}

}  // namespace

Scan::Scan(const std::string& path, Schema schema, std::vector<BatchMetaData> batch_meta)
    : reader_(FileReader(path)),
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

    EngineBatch result;
    for (size_t col : columns_) {
        const auto& meta = batch_meta_[rg * cols + col];
        reader_.Jump(meta.offset - reader_.GetPos());
        result.names.push_back(schema_.columns[col].name);
        result.columns.emplace_back(schema_.columns[col].type->GetBatch(meta.size, reader_));
    }

    size_t num_rows = GetColumnSize(result.columns[0]);
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
void Aggregate::Run() {
    ready_ = true;
    while (auto batch = child_->GetNext()) {
        for (auto i : batch->selection) {
            GroupKey key;
            key.values.reserve(group_columns_.size());
            for (size_t col : group_columns_) {
                key.values.push_back(GetColumnValue(batch->columns[col], i));
            }

            auto& aggs = groups_[key];
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

}

std::optional<EngineBatch> Aggregate::GetNext() {
    constexpr size_t kBatchSize = 1024;
    if (!ready_) {
        Run();
        cur_ = groups_.begin();
    }
    if (cur_ == groups_.end()) {
        return std::nullopt;
    }

    EngineBatch result;
    result.names = group_names_;
    for (const auto& agg : cur_->second) {
        result.names.push_back(agg->GetName());
    }
    
    for (const auto& v : cur_->first.values) {
        result.columns.push_back(MakeColumnData(v));
    }
    for (const auto& agg : cur_->second) {
        result.columns.push_back(MakeEmptyColumnLike(agg->GetResult()));
    }

    size_t count = 0;
    while (cur_ != groups_.end() && count < kBatchSize) {
        const auto& [key, aggs] = *cur_;
        for (size_t i = 0; i < key.values.size(); ++i) {
            AppendColumnValue(result.columns[i], key.values[i]);
        }
        for (size_t i = 0; i < aggs.size(); ++i) {
            AppendColumnValue(result.columns[group_names_.size() + i],
                              GetColumnValue(aggs[i]->GetResult(), 0));
        }
        result.selection.push_back(count++);
        ++cur_;
    }

    return result;
}

std::optional<EngineBatch> LimitOp::GetNext() {
    return GetAllBatches(child_, limit_);
}

std::optional<EngineBatch> OffsetOp::GetNext() {
    auto result = GetAllBatches(child_);
    if (!result) {
        return std::nullopt;
    }
    if (skipped_ < offset_) {
        size_t to_skip = std::min(offset_ - skipped_, result->selection.size());
        result->selection.erase(result->selection.begin(), result->selection.begin() + to_skip);
        skipped_ += to_skip;
    }
    return result;
}

std::optional<EngineBatch> Sort::GetNext() {
    if (auto result = GetAllBatches(child_)) {
        const ColumnData& col = result->columns[col_idx_];
        std::sort(result->selection.begin(), result->selection.end(),
                  [&](RowIndex lhs, RowIndex rhs) { return LessAt(col, lhs, rhs); });
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

    struct Row {
        std::vector<ColumnValue> values;
        size_t order;
    };

    std::function<bool(const Row&, const Row&)> is_better;
    if (col_idx_2_ != -1) {
        is_better = [&](const Row& lhs, const Row& rhs) {
            const ColumnValue& l = lhs.values[col_idx_];
            const ColumnValue& r = rhs.values[col_idx_];
            if (LessColumnValue(l, r)) { return !reversed_; }
            if (LessColumnValue(r, l)) { return reversed_; }
            return LessColumnValue(lhs.values[col_idx_2_], rhs.values[col_idx_2_]);
        };
    } else {
        is_better = [&](const Row& lhs, const Row& rhs) {
            const ColumnValue& l = lhs.values[col_idx_];
            const ColumnValue& r = rhs.values[col_idx_];
            if (LessColumnValue(l, r)) { return !reversed_; }
            if (LessColumnValue(r, l)) { return reversed_; }
            return lhs.order < rhs.order;
        };
    }

    auto heap_cmp = [&](const Row& lhs, const Row& rhs) {
        return is_better(lhs, rhs);
    };

    std::vector<std::string> names;
    std::vector<Row> heap;
    size_t next_order = 0;

    while (auto batch = child_->GetNext()) {
        if (names.empty()) {
            names = batch->names;
        }
        for (auto i : batch->selection) {
            Row row;
            row.order = next_order++;
            for (const auto& col : batch->columns) {
                row.values.push_back(GetColumnValue(col, i));
            }
            if (heap.size() < limit_) {
                heap.push_back(std::move(row));
                if (heap.size() == limit_) {
                    std::make_heap(heap.begin(), heap.end(), heap_cmp);
                }
            } else if (is_better(row, heap.front())) {
                std::pop_heap(heap.begin(), heap.end(), heap_cmp);
                heap.back() = std::move(row);
                std::push_heap(heap.begin(), heap.end(), heap_cmp);
            }
        }
    }

    if (names.empty()) {
        return std::nullopt;
    }

    std::sort(heap.begin(), heap.end(), [&](const Row& lhs, const Row& rhs) {
        return is_better(lhs, rhs);
    });

    EngineBatch result;
    result.names = std::move(names);
    for (size_t col = 0; col < result.names.size(); ++col) {
        result.columns.push_back(MakeEmptyColumnLike(MakeColumnData(heap[0].values[col])));
    }
    for (const auto& row : heap) {
        for (size_t col = 0; col < row.values.size(); ++col) {
            AppendColumnValue(result.columns[col], row.values[col]);
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
    // LOG(INFO) << "Engine started, " << schema_.columns.size() << " columns, "
    //           << batch_meta_.size() / schema_.columns.size() << " row groups";
}

std::shared_ptr<Scan> Engine::MakeScan() {
    return std::make_shared<Scan>(path_, schema_, batch_meta_);
}

EngineBatch Engine::Run(std::shared_ptr<Operator> root,
                        const std::vector<size_t>& selected_columns) {

    if (auto probe = GetAllBatches(root)) {
        EngineBatch result = probe.value();
        if (selected_columns.empty()) {
            return result;
        }

        EngineBatch selected_result;
        selected_result.selection = result.selection;
        for (size_t col : selected_columns) {
            selected_result.names.push_back(result.names[col]);
            selected_result.columns.push_back(result.columns[col]);
        }
        return selected_result;
    }
    return EngineBatch{};
}

ApiPipeline Engine::Api() {
    return ApiPipeline(*this, schema_);
}

}  // namespace column_engine::internal
