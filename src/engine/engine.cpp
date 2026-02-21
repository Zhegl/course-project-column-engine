#include "engine.h"
#include <glog/logging.h>
#include <format/meta_reader.h>
#include <memory>
#include <file_reader.h>
#include <types/types.h>

namespace column_engine {
Engine::Engine(const std::string& path) : path_(path) {
    auto [batch_meta, schema] = GetMeta(path_);
    LOG(INFO) << "Engine stated";
}

std::shared_ptr<EngineBatch> Engine::Run(std::shared_ptr<Operator> root) {
    std::shared_ptr<EngineBatch> result;
    auto it = root;
    while (it->GetNext()) {
        it = it->GetNext();
    }
    it->SetNext(std::make_shared<Sink>(Sink(result)));

    FileReader reader(path_);
    for (size_t i = 0; i < batch_meta_.size() / schema_.columns.size(); ++i) {
        root->Process(GetEngineBatch(i));
    }

    it = root;
    while (it->GetNext()) {
        it->Finalize();
        it = it->GetNext();
    }
    return result;
}

std::shared_ptr<EngineBatch> Engine::GetEngineBatch(size_t row_group) {
    size_t cols = schema_.columns.size();
    FileReader reader(path_);
    std::shared_ptr<EngineBatch> result = std::make_shared<EngineBatch>(EngineBatch());
    for (size_t col = 0; col < cols; ++col) {
        reader.Jump(batch_meta_[row_group * cols + col].offset - reader.GetPos());
        result->names.push_back(schema_.columns[col].name);
        result->columns.emplace_back(
            schema_.columns[col].type->GetBatch(batch_meta_[row_group * cols + col].size, reader));
    }
    return result;
}


Sink::Sink(std::shared_ptr<EngineBatch> batch) : batch_(batch) {
}

void Sink::Finalize() {
}

void Sink::Process(std::shared_ptr<EngineBatch> batch) {
    if (batch_->names.empty()) {
        batch_->names = batch->names;
    }
    for (size_t i = 0; i < batch_->columns.size(); ++i) {
        if (auto* dst = std::get_if<std::vector<int64_t>>(&batch_->columns[i])) {
            auto& src = std::get<std::vector<int64_t>>(batch->columns[i]);
            dst->insert(dst->end(), src.begin(), src.end());
        } else if (auto* dst = std::get_if<std::vector<std::string>>(&batch_->columns[i])) {
            auto& src = std::get<std::vector<std::string>>(batch->columns[i]);
            dst->insert(dst->end(), src.begin(), src.end());
        }
    }
}

}  // namespace column_engine