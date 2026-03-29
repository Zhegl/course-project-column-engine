#pragma once
#include <types/types.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace column_engine {

struct EngineBatch {
    std::vector<std::string> names;
    std::vector<ColumnData> columns;
    std::vector<uint16_t> selection;
};

size_t GetColumnIndex(const EngineBatch& batch, const std::string& name);

class Operator {
public:
    virtual std::optional<EngineBatch> GetNext() = 0;
    virtual void Finalize() {}
    virtual ~Operator() = default;
};

class Scan : public Operator {
public:
    Scan(const std::string& path, Schema schema, std::vector<BatchMetaData> batch_meta);
    std::optional<EngineBatch> GetNext() override;

private:
    std::string path_;
    Schema schema_;
    std::vector<BatchMetaData> batch_meta_;
    size_t current_row_group_ = 0;
    size_t num_row_groups_ = 0;
};

template <typename Predicate>
class Filter : public Operator {
public:
    Filter(std::shared_ptr<Operator> child, Predicate pred)
        : child_(std::move(child)), pred_(std::move(pred)) {
    }

    std::optional<EngineBatch> GetNext() override {
        while (auto batch = child_->GetNext()) {
            std::vector<uint16_t> new_selection;
            for (auto i : batch->selection) {
                if (pred_(*batch, i)) {
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

private:
    std::shared_ptr<Operator> child_;
    Predicate pred_;
};

class Engine {
public:
    explicit Engine(const std::string& path);
    EngineBatch Run(std::shared_ptr<Operator> root);
    std::shared_ptr<Operator> MakeScan();

private:
    Schema schema_;
    std::string path_;
    std::vector<BatchMetaData> batch_meta_;
};

}  // namespace column_engine

