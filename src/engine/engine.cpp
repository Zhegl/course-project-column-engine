#include "engine.h"
#include <format/meta_reader.h>
#include <io/file_reader.h>
#include <glog/logging.h>
#include <cstdint>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace column_engine {

namespace {

ColumnValue GetColumnValue(const ColumnData& column, uint16_t i) {
    return std::visit([&](const auto& data) -> ColumnValue { return data[i]; }, column);
}

ColumnData MakeColumnData(const ColumnValue& value) {
    return std::visit([](const auto& v) -> ColumnData {
        using T = std::decay_t<decltype(v)>;
        return std::vector<T>{};
    }, value);
}

void AppendColumnValue(ColumnData& column, const ColumnValue& value) {
    std::visit([&](auto& data) {
        using T = typename std::decay_t<decltype(data)>::value_type;
        data.push_back(std::get<T>(value));
    }, column);
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
    for (uint16_t i = 0; i < static_cast<uint16_t>(num_rows); ++i) {
        result.selection[i] = i;
    }
    return result;
}

// Filter

std::optional<EngineBatch> Filter::GetNext() {
    while (auto batch = child_->GetNext()) {
        std::vector<uint16_t> new_selection;
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
            AppendColumnValue(result.columns[group_names_.size() + i], GetColumnValue(aggs[i]->GetResult(), 0));
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

EngineBatch Engine::Run(std::shared_ptr<Operator> root, const std::vector<std::string>& selected_columns) {
    EngineBatch result;
    while (auto batch = root->GetNext()) {
        if (result.names.empty()) {
            result.names = batch->names;
            for (const auto& col : batch->columns) {
                result.columns.push_back(std::visit([](const auto& v) -> ColumnData {
                    return std::decay_t<decltype(v)>{};
                }, col));
            }
        }
        for (size_t col = 0; col < batch->columns.size(); ++col) {
            std::visit([&](auto& dst, const auto& src) {
                if constexpr (std::is_same_v<std::decay_t<decltype(dst)>, std::decay_t<decltype(src)>>) {
                    for (auto i : batch->selection) {
                        dst.push_back(src[i]);
                    }
                }
            }, result.columns[col], batch->columns[col]);
        }
        for (size_t k = 0; k < batch->selection.size(); ++k) {
            result.selection.push_back(result.selection.size());
        }
    }
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

ApiPipeline Engine::Api() {
    return ApiPipeline(*this, schema_);
}

}  // namespace column_engine
