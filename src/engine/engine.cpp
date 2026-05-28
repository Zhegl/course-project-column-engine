#include "engine.h"
#include "engine/pipeline.h"
#include <types/scan_options.h>
#include "engine/column_utils.h"
#include <format/meta_reader.h>
#include <io/file_reader.h>
#include <glog/logging.h>
#include <algorithm>
#include <memory>
#include <optional>
#include <vector>
#include <fcntl.h>
#include "engine/batch.h"

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

ScanOp::ScanOp(const std::string& path, Schema schema, std::vector<BatchMetaData> batch_meta)
    : reader_(FileReader(path)),
      schema_(std::move(schema)),
      batch_meta_(std::move(batch_meta)),
      num_row_groups_(batch_meta_.size() / schema_.columns.size()) {
}

void ScanOp::SetColumns(std::vector<size_t> columns) {
    columns_ = columns;
}

void ScanOp::SetScanOptions(std::vector<std::shared_ptr<ScanOptions>> options) {
    scan_options_ = std::move(options);
}

std::optional<EngineBatch> ScanOp::GetNext() {
    if (current_row_group_ >= num_row_groups_) {
        return std::nullopt;
    }

    size_t cols = schema_.columns.size();
    size_t rg = current_row_group_++;

    constexpr size_t kReadaheadGroups = 4;
    size_t ra_rg = current_row_group_ + kReadaheadGroups - 1;
    if (ra_rg < num_row_groups_) {
        for (size_t col : columns_) {
            const auto& meta = batch_meta_[ra_rg * cols + col];
            readahead(reader_.Fd(), meta.offset, meta.size);
        }
    }

    EngineBatch result;
    for (size_t k = 0; k < columns_.size(); ++k) {
        size_t col = columns_[k];
        const auto& meta = batch_meta_[rg * cols + col];
        reader_.Jump(meta.offset - reader_.GetPos());
        result.names.push_back(schema_.columns[col].name);
        const ScanOptions* opts = (!scan_options_.empty() && scan_options_[k])
                                      ? scan_options_[k].get()
                                      : nullptr;
        result.columns.emplace_back(schema_.columns[col].type->GetBatch(meta.size, reader_, opts));
        if (GetColumnSize(result.columns.back()) == 0 && opts != nullptr) {
            return GetNext();
        }
    }

    size_t num_rows = GetColumnSize(result.columns[0]);
    result.selection.resize(num_rows);
    for (RowIndex i = 0; i < num_rows; ++i) {
        result.selection[i] = i;
    }
    return result;
}

// FilterOp

std::optional<EngineBatch> FilterOp::GetNext() {
    while (auto batch = child_->GetNext()) {
        batch->selection = pred_->CheckBatch(*batch, batch->selection);
        if (!batch->selection.empty()) {
            return batch;
        }
    }
    return std::nullopt;
}

// AggregateOp

void AggregateOp::UpdateSlot(AggSlot& slot, EngineBatch& batch, RowIndex i, bool only_count_all) {
    if (has_count_all_) { slot.count++; }
    if (!only_count_all) {
        if (slot.rest.empty()) {
            size_t first = static_cast<size_t>(has_count_all_);
            slot.rest.reserve(factories_.size() - first);
            for (size_t f = first; f < factories_.size(); ++f) {
                slot.rest.push_back(factories_[f].make());
            }
        }
        for (auto& agg : slot.rest) { agg->Next(batch, i); }
    }
}

void AggregateOp::BuildKey(RowIndex i, const std::vector<ColDesc>& col_descs,
                            const std::vector<ColPtr>& ptrs, std::vector<char>& buf) {
    for (size_t k = 0; k < col_descs.size(); ++k) {
        if (!col_descs[k].is_str) {
            size_t cur = buf.size();
            buf.resize(cur + sizeof(int64_t));
            __builtin_memcpy(buf.data() + cur, &ptrs[k].ints[i], sizeof(int64_t));
        } else {
            std::string_view sv = ptrs[k].svs ? ptrs[k].svs[i] : std::string_view(ptrs[k].strs[i]);
            uint32_t len = static_cast<uint32_t>(sv.size());
            size_t cur = buf.size();
            buf.resize(cur + sizeof(uint32_t) + len);
            __builtin_memcpy(buf.data() + cur, &len, sizeof(uint32_t));
            __builtin_memcpy(buf.data() + cur + sizeof(uint32_t), sv.data(), len);
        }
    }
}

void AggregateOp::ProcessBatch(EngineBatch& batch,
                                const std::vector<ColDesc>& col_descs,
                                bool only_count_all) {
    std::vector<ColPtr> ptrs(col_descs.size());
    for (size_t k = 0; k < col_descs.size(); ++k) {
        const auto& col = batch.columns[col_descs[k].col_idx];
        if (!col_descs[k].is_str) {
            ptrs[k] = {std::get<std::vector<int64_t>>(col).data(), nullptr, nullptr};
        } else if (std::holds_alternative<std::vector<std::string_view>>(col)) {
            ptrs[k] = {nullptr, std::get<std::vector<std::string_view>>(col).data(), nullptr};
        } else {
            ptrs[k] = {nullptr, nullptr, std::get<std::vector<std::string>>(col).data()};
        }
    }

    const size_t n = batch.selection.size();
    const bool use_prefetch = groups_.Size() >= kPrefetchThreshold;

    if (!use_prefetch) {
        key_buffer_.clear();
        for (size_t idx = 0; idx < n; ++idx) {
            RowIndex i = batch.selection[idx];
            key_buffer_.clear();
            BuildKey(i, col_descs, ptrs, key_buffer_);
            std::string_view lookup_key(key_buffer_.data(), key_buffer_.size());
            auto& slot = groups_.FindOrInsert(
                lookup_key, [&](std::string_view fresh_key) { return arena_.Alloc(fresh_key); });
            UpdateSlot(slot, batch, i, only_count_all);
        }
        return;
    }

    prefetch_key_buf_.clear();
    prefetch_hashes_.resize(n);
    prefetch_key_spans_.resize(n);

    for (size_t idx = 0; idx < n; ++idx) {
        RowIndex i = batch.selection[idx];
        size_t start = prefetch_key_buf_.size();
        BuildKey(i, col_descs, ptrs, prefetch_key_buf_);
        size_t end = prefetch_key_buf_.size();
        prefetch_key_spans_[idx] = {start, end - start};
        prefetch_hashes_[idx] = StringViewHash{}(
            std::string_view(prefetch_key_buf_.data() + start, end - start));
    }

    for (size_t idx = 0; idx < n; ++idx) {
        if (idx + kPrefetchDist < n) { groups_.Prefetch(prefetch_hashes_[idx + kPrefetchDist]); }
        auto [off, len] = prefetch_key_spans_[idx];
        std::string_view lookup_key(prefetch_key_buf_.data() + off, len);
        auto& slot = groups_.FindOrInsert(
            lookup_key, prefetch_hashes_[idx],
            [&](std::string_view fresh_key) { return arena_.Alloc(fresh_key); });
        UpdateSlot(slot, batch, batch.selection[idx], only_count_all);
    }
}

void AggregateOp::Run() {
    ready_ = true;
    has_count_all_ = !factories_.empty() && factories_[0].is_count_all;
    const bool only_count_all = has_count_all_ && factories_.size() == 1;

    is_string_column_.assign(group_columns_.size(), false);

    std::vector<ColDesc> col_descs;
    col_descs.reserve(group_columns_.size());

    if (auto batch = child_->GetNext()) {
        for (size_t k = 0; k < group_columns_.size(); ++k) {
            size_t col_idx = group_columns_[k];
            bool is_str = !std::holds_alternative<std::vector<int64_t>>(batch->columns[col_idx]);
            col_descs.push_back({col_idx, is_str});
            is_string_column_[k] = is_str;
        }
        ProcessBatch(*batch, col_descs, only_count_all);
        while (auto next = child_->GetNext()) {
            ProcessBatch(*next, col_descs, only_count_all);
        }
    }

    cur_idx_ = groups_.FirstUsed();
}

std::optional<EngineBatch> AggregateOp::GetNext() {
    if (!ready_) {
        Run();
    }

    constexpr size_t kBatchSize = 1024;
    if (cur_idx_ >= groups_.Capacity()) {
        return std::nullopt;
    }

    EngineBatch result;
    result.names = group_names_;

    for (const auto& name : agg_names_) {
        result.names.push_back(name);
    }

    for (size_t i = 0; i < group_columns_.size(); ++i) {
        if (is_string_column_[i]) {
            result.columns.emplace_back(std::vector<std::string_view>{});
        } else {
            result.columns.emplace_back(std::vector<int64_t>{});
        }
    }

    if (has_count_all_) {
        result.columns.emplace_back(std::vector<int64_t>{});
        for (const auto& agg : groups_.GetSlot(cur_idx_).val.rest) {
            result.columns.push_back(MakeEmptyColumnLike(agg->GetResult()));
        }
    } else {
        for (const auto& agg : groups_.GetSlot(cur_idx_).val.rest) {
            result.columns.push_back(MakeEmptyColumnLike(agg->GetResult()));
        }
    }

    for (auto& col : result.columns) {
        std::visit([](auto& v) { v.reserve(kBatchSize); }, col);
    }

    size_t count = 0;
    while (cur_idx_ < groups_.Capacity() && count < kBatchSize) {
        auto& slot = groups_.GetSlot(cur_idx_);
        std::string_view key = slot.key;
        size_t offset = 0;

        for (size_t i = 0; i < group_columns_.size(); ++i) {
            if (is_string_column_[i]) {
                uint32_t len;
                std::memcpy(&len, key.data() + offset, sizeof(uint32_t));
                offset += sizeof(uint32_t);

                std::string_view str_val(key.data() + offset, len);
                offset += len;

                std::get<std::vector<std::string_view>>(result.columns[i]).push_back(str_val);
            } else {
                int64_t int_val;
                std::memcpy(&int_val, key.data() + offset, sizeof(int64_t));
                offset += sizeof(int64_t);

                std::get<std::vector<int64_t>>(result.columns[i]).push_back(int_val);
            }
        }

        size_t agg_col_offset = group_columns_.size();
        if (has_count_all_) {
            std::get<std::vector<int64_t>>(result.columns[agg_col_offset])
                .push_back(slot.val.count);
            for (size_t i = 0; i < slot.val.rest.size(); ++i) {
                auto agg_res = slot.val.rest[i]->GetResult();
                AppendColumnValue(result.columns[agg_col_offset + 1 + i],
                                  GetColumnValue(agg_res, 0));
            }
        } else {
            for (size_t i = 0; i < slot.val.rest.size(); ++i) {
                auto agg_res = slot.val.rest[i]->GetResult();
                AppendColumnValue(result.columns[agg_col_offset + i], GetColumnValue(agg_res, 0));
            }
        }

        result.selection.push_back(static_cast<RowIndex>(count++));
        cur_idx_ = groups_.NextUsed(cur_idx_);
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

std::optional<EngineBatch> SortOp::GetNext() {
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

std::optional<EngineBatch> AsOp::GetNext() {
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

std::optional<EngineBatch> TopKOp::GetNext() {
    if (limit_ == 0) {
        return GetAllBatches(child_, 0);
    }

    struct Row {
        std::vector<ColumnValue> values;
        size_t order;
    };

    std::function<bool(const Row&, const Row&)> is_better;
    if (col_idx_2_) {
        is_better = [&](const Row& lhs, const Row& rhs) {
            const ColumnValue& l = lhs.values[col_idx_];
            const ColumnValue& r = rhs.values[col_idx_];
            if (LessColumnValue(l, r)) {
                return !reversed_;
            }
            if (LessColumnValue(r, l)) {
                return reversed_;
            }
            return LessColumnValue(lhs.values[*col_idx_2_], rhs.values[*col_idx_2_]);
        };
    } else {
        is_better = [&](const Row& lhs, const Row& rhs) {
            const ColumnValue& l = lhs.values[col_idx_];
            const ColumnValue& r = rhs.values[col_idx_];
            if (LessColumnValue(l, r)) {
                return !reversed_;
            }
            if (LessColumnValue(r, l)) {
                return reversed_;
            }
            return lhs.order < rhs.order;
        };
    }

    auto heap_cmp = [&](const Row& lhs, const Row& rhs) { return is_better(lhs, rhs); };

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

    std::sort(heap.begin(), heap.end(),
              [&](const Row& lhs, const Row& rhs) { return is_better(lhs, rhs); });

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
}

std::shared_ptr<ScanOp> Engine::MakeScan() {
    return std::make_shared<ScanOp>(path_, schema_, batch_meta_);
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

Pipeline Engine::Api() {
    return Pipeline(*this, schema_);
}

}  // namespace column_engine::internal
