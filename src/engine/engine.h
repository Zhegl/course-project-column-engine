#pragma once
#include <types/types.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <api.h>
#include <engine/batch.h>
#include <engine/hashmap.h>
#include <glog/logging.h>
#include <engine/queries.h>
#include <engine/parser.h>

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
    void SetColumns(std::vector<size_t> columns);

private:
    std::string path_;
    Schema schema_;
    std::vector<BatchMetaData> batch_meta_;
    std::vector<size_t> columns_;
    size_t current_row_group_ = 0;
    size_t num_row_groups_ = 0;
};

class Filter : public Operator {
public:
    Filter(std::shared_ptr<Operator> child, std::shared_ptr<FilterPredicate> pred)
        : child_(std::move(child)), pred_(std::move(pred)) {
    }

    std::optional<EngineBatch> GetNext() override;

private:
    std::shared_ptr<Operator> child_;
    std::shared_ptr<FilterPredicate> pred_;
};

class Aggregate : public Operator {
public:
    Aggregate(std::shared_ptr<Operator> child, std::vector<size_t> group_columns,
              std::vector<std::string> group_names, std::vector<AggFactory> factories)
        : child_(std::move(child)),
          group_columns_(std::move(group_columns)),
          group_names_(std::move(group_names)),
          factories_(std::move(factories)) {
    }

    std::optional<EngineBatch> GetNext() override;

private:
    std::shared_ptr<Operator> child_;
    std::vector<size_t> group_columns_;
    std::vector<std::string> group_names_;
    std::vector<AggFactory> factories_;
};

class LimitOp : public Operator {
public:
    LimitOp(std::shared_ptr<Operator> child, size_t limit)
        : child_(std::move(child)), limit_(limit) {
    }
    std::optional<EngineBatch> GetNext() override;

private:
    std::shared_ptr<Operator> child_;
    size_t limit_;
};

class Sort : public Operator {
public:
    Sort(std::shared_ptr<Operator> child, std::string col, bool reversed)
        : child_(std::move(child)), col_(col), reversed_(reversed) {
    }
    std::optional<EngineBatch> GetNext() override;

private:
    std::shared_ptr<Operator> child_;
    std::string col_;
    bool reversed_;
};

class As : public Operator {
public:
    As(std::shared_ptr<Operator> child, std::string from, std::string to)
        : child_(std::move(child)), from_(from), to_(to) {
    }
    std::optional<EngineBatch> GetNext() override;
private:
    std::shared_ptr<Operator> child_;
    std::string from_;
    std::string to_;
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
    EngineBatch Run(std::shared_ptr<Operator> root,
                    const std::vector<std::string>& selected_columns = {});
    std::shared_ptr<Scan> MakeScan();
    ApiPipeline Api();

private:
    Schema schema_;
    std::string path_;
    std::vector<BatchMetaData> batch_meta_;
};

}  // namespace column_engine
