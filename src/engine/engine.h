#pragma once
#include <types/types.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <api.h>
#include <engine/batch.h>
#include <glog/logging.h>
#include <engine/queries.h>

namespace column_engine {

size_t GetColumnIndex(const EngineBatch& batch, const std::string& name);

class Operator {
public:
    virtual std::optional<EngineBatch> GetNext() = 0;
    virtual ~Operator() = default;
    void SetChild(std::shared_ptr<Operator> child);
private:
    std::shared_ptr<Operator> child_;
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


class Filter : public Operator {
public:
    Filter(std::shared_ptr<Operator> child, std::shared_ptr<FilterPredicate> pred)
        : child_(std::move(child)), pred_(std::move(pred)) {
    }

    std::optional<EngineBatch> GetNext() override {
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

private:
    std::shared_ptr<Operator> child_;
    std::shared_ptr<FilterPredicate> pred_;
};

class Aggregate : public Operator {
public:
    Aggregate(std::shared_ptr<Operator> child, std::shared_ptr<Aggregator> agg)
        : child_(std::move(child)),
          agg_(std::move(agg)) {
    }

    std::optional<EngineBatch> GetNext() override {
        LOG(INFO) << "Agg";
        bool is_empty = true;
        while (auto batch = child_->GetNext()) {
            is_empty &= batch->selection.empty();
            for (auto i : batch->selection) {
                agg_->Next(*batch, i);
            }
        }
        if (is_empty) {
            return std::nullopt;
        }
        auto result = agg_->GetResult();
        return result;
    }

private:
    std::shared_ptr<Operator> child_;
    std::shared_ptr<Aggregator> agg_;
};

/*
template <typename Comparator>
class OrderBy : public Operator {
public:
    OrderBy(std::shared_ptr<Operator> child, Comparator cmp, size_t limit, size_t offset)
        : child_(std::move(child)),
          cmp_(std::move(cmp)) {
    }

    std::optional<EngineBatch> GetNext() override {
        
        while (auto batch = child_->GetNext()) {
            for (auto i : batch->selection) {
                agg_.Next(*batch, i);
            }
        }
        return agg_.GetResult();
    }

private:
    std::shared_ptr<Operator> child_;
    Comparator cmp_;
};
*/

class ApiPipeline;

class Engine {
public:
    explicit Engine(const std::string& path);
    EngineBatch Run(std::shared_ptr<Operator> root);
    std::shared_ptr<Operator> MakeScan();
    ApiPipeline Api();

private:
    Schema schema_;
    std::string path_;
    std::vector<BatchMetaData> batch_meta_;
};

}  // namespace column_engine
