#include "engine.h"
#include <format/meta_reader.h>
#include <io/file_reader.h>
#include <glog/logging.h>
#include <stdexcept>

namespace column_engine {

size_t GetColumnIndex(const EngineBatch& batch, const std::string& name) {
    for (size_t i = 0; i < batch.names.size(); ++i) {
        if (batch.names[i] == name) {
            return i;
        }
    }
    throw std::runtime_error("Key error: " + name);
}

// Scan

Scan::Scan(const std::string& path, Schema schema, std::vector<BatchMetaData> batch_meta)
    : path_(path),
      schema_(std::move(schema)),
      batch_meta_(std::move(batch_meta)),
      num_row_groups_(batch_meta_.size() / schema_.columns.size()) {
}

std::optional<EngineBatch> Scan::GetNext() {
    if (current_row_group_ >= num_row_groups_) {
        return std::nullopt;
    }

    size_t cols = schema_.columns.size();
    size_t rg = current_row_group_++;

    FileReader reader(path_);
    EngineBatch result;
    for (size_t col = 0; col < cols; ++col) {
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

// Engine

Engine::Engine(const std::string& path) : path_(path) {
    auto [batch_meta, schema] = GetMeta(path_);
    batch_meta_ = std::move(batch_meta);
    schema_ = std::move(schema);
    LOG(INFO) << "Engine started, " << schema_.columns.size() << " columns, "
              << batch_meta_.size() / schema_.columns.size() << " row groups";
}

std::shared_ptr<Operator> Engine::MakeScan() {
    return std::make_shared<Scan>(path_, schema_, batch_meta_);
}

EngineBatch Engine::Run(std::shared_ptr<Operator> root) {
    EngineBatch result;
    while (auto batch = root->GetNext()) {
        if (result.names.empty()) {
            result.names = batch->names;
            result.columns.resize(batch->columns.size());
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
    }
    root->Finalize();
    return result;
}

}  // namespace column_engine
