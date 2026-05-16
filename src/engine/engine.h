#pragma once
#include <types/types.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "file_reader.h"
#include <engine/batch.h>
#include <engine/hashmap.h>
#include <glog/logging.h>
#include <engine/predicates.h>
#include <engine/expressions.h>
#include <engine/parser.h>

namespace column_engine::internal {


class Operator {
public:
    virtual std::optional<EngineBatch> GetNext() = 0;
    virtual ~Operator() = default;
};

class Scan : public Operator {
public:
    Scan(const std::string& path, Schema schema, std::vector<BatchMetaData> batch_meta);
    std::optional<EngineBatch> GetNext() override;
    void SetColumns(std::vector<size_t> columns);

private:
    FileReader reader_;
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

enum class GroupKeyType { Int, Str, Multi };

class Aggregate : public Operator {
public:
    using AggVec = std::vector<std::shared_ptr<Aggregator>>;

    Aggregate(std::shared_ptr<Operator> child, std::vector<size_t> group_columns,
              std::vector<std::string> group_names, std::vector<AggFactory> factories,
              GroupKeyType key_type)
        : child_(std::move(child)),
          group_columns_(std::move(group_columns)),
          group_names_(std::move(group_names)),
          factories_(std::move(factories)),
          key_type_(key_type) {
    }

    std::optional<EngineBatch> GetNext() override;

private:
    void Run();
    bool ready_{false};
    size_t cur_idx_{0};
    GroupKeyType key_type_;
    std::shared_ptr<Operator> child_;
    std::vector<size_t> group_columns_;
    std::vector<std::string> group_names_;
    std::vector<AggFactory> factories_;

    std::optional<HashMap<int64_t, AggVec, IntHash>>    int_groups_;
    std::optional<HashMap<std::string, AggVec, StrHash>> str_groups_;
    std::optional<GroupHashMap<AggVec>>                  multi_groups_;
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

class OffsetOp : public Operator {
public:
    OffsetOp(std::shared_ptr<Operator> child, size_t offset)
        : child_(std::move(child)), offset_(offset) {
    }
    std::optional<EngineBatch> GetNext() override;

private:
    std::shared_ptr<Operator> child_;
    size_t offset_;
    size_t skipped_{0};
};

class Sort : public Operator {
public:
    Sort(std::shared_ptr<Operator> child, size_t col_idx, bool reversed)
        : child_(std::move(child)), col_idx_(col_idx), reversed_(reversed) {
    }
    std::optional<EngineBatch> GetNext() override;

private:
    std::shared_ptr<Operator> child_;
    size_t col_idx_;
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


class TopK : public Operator {
public:
    TopK(std::shared_ptr<Operator> child, size_t col_idx, bool reversed, size_t limit)
        : child_(std::move(child)), col_idx_(col_idx), reversed_(reversed), limit_(limit) {
    }
    TopK(std::shared_ptr<Operator> child, size_t col_idx, size_t col_idx_2, bool reversed, size_t limit)
        : child_(std::move(child)), col_idx_(col_idx), col_idx_2_(col_idx_2), reversed_(reversed), limit_(limit) {
    }
    std::optional<EngineBatch> GetNext() override;
private:
    std::shared_ptr<Operator> child_;
    size_t col_idx_;
    int64_t col_idx_2_{-1};
    bool reversed_;
    size_t limit_;
};

template <typename T>
class AddCol : public Operator {
public:
    AddCol(std::shared_ptr<Operator> child, std::shared_ptr<AddColFun> fun, size_t col_idx)
        : child_(std::move(child)), fun_(std::move(fun)), col_idx_(col_idx) {
    }

    std::optional<EngineBatch> GetNext() override {
        if (auto batch = child_->GetNext()) {
            std::vector<T> new_col(batch->selection.back() + 1);
            for (auto i : batch->selection) {
                new_col[i] = std::get<T>(fun_->Get(batch.value(), i));
            }
            batch->columns.insert(batch->columns.begin() + col_idx_, std::move(new_col));
            batch->names.insert(batch->names.begin() + col_idx_, fun_->GetName());
            return batch;
        }
        return std::nullopt;
    }

private:
    std::shared_ptr<Operator> child_;
    std::shared_ptr<AddColFun> fun_;
    size_t col_idx_;
};


template <typename T>
class AddCase : public Operator {
public:
    AddCase(std::shared_ptr<Operator> child, std::shared_ptr<AddColFun> then_fun,
            std::shared_ptr<AddColFun> else_fun, size_t col_idx,
            std::shared_ptr<FilterPredicate> pred, std::string name)
        : child_(std::move(child)), then_fun_(std::move(then_fun)),
          else_fun_(std::move(else_fun)), col_idx_(col_idx),
          pred_(std::move(pred)), name_(std::move(name)) {}

    std::optional<EngineBatch> GetNext() override {
        if (auto batch = child_->GetNext()) {
            std::vector<T> new_col(batch->selection.back() + 1);
            for (auto i : batch->selection) {
                if (pred_->Check(*batch, i)) {
                    new_col[i] = std::get<T>(then_fun_->Get(batch.value(), i));
                } else {
                    new_col[i] = std::get<T>(else_fun_->Get(batch.value(), i));
                }
            }
            batch->columns.insert(batch->columns.begin() + col_idx_, std::move(new_col));
            batch->names.insert(batch->names.begin() + col_idx_, name_);
            return batch;
        }
        return std::nullopt;
    }

private:
    std::shared_ptr<Operator> child_;
    std::shared_ptr<AddColFun> then_fun_;
    std::shared_ptr<AddColFun> else_fun_;
    std::shared_ptr<FilterPredicate> pred_;
    std::string name_;
    size_t col_idx_;
};

class ApiPipeline;

class Engine {
public:
    explicit Engine(const std::string& path);
    EngineBatch Run(std::shared_ptr<Operator> root,
                    const std::vector<size_t>& selected_columns = {});
    std::shared_ptr<Scan> MakeScan();
    ApiPipeline Api();

private:
    Schema schema_;
    std::string path_;
    std::vector<BatchMetaData> batch_meta_;
};

}  // namespace column_engine::internal
