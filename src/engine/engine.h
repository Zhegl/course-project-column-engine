#pragma once
#include <types/types.h>
#include <types/scan_options.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "io/file_reader.h"
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

class ScanOp : public Operator {
public:
    ScanOp(const std::string& path, Schema schema, std::vector<BatchMetaData> batch_meta);
    std::optional<EngineBatch> GetNext() override;
    void SetColumns(std::vector<size_t> columns);
    void SetScanOptions(std::vector<std::shared_ptr<ScanOptions>> options);

private:
    FileReader reader_;
    Schema schema_;
    std::vector<BatchMetaData> batch_meta_;
    std::vector<size_t> columns_;
    std::vector<std::shared_ptr<ScanOptions>> scan_options_;
    size_t current_row_group_ = 0;
    size_t num_row_groups_ = 0;
};

class FilterOp : public Operator {
public:
    FilterOp(std::shared_ptr<Operator> child, std::shared_ptr<FilterPredicate> pred)
        : child_(std::move(child)), pred_(std::move(pred)) {
    }

    std::optional<EngineBatch> GetNext() override;

private:
    std::shared_ptr<Operator> child_;
    std::shared_ptr<FilterPredicate> pred_;
};

enum class GroupKeyType { Int, Str, Multi };


class AggregateOp : public Operator {
public:
    static constexpr size_t kPrefetchThreshold = 1024;
    static constexpr size_t kPrefetchDist = 16;

    AggregateOp(std::shared_ptr<Operator> child, std::vector<size_t> group_columns,
                std::vector<std::string> group_names, std::vector<AggFactory> factories,
                std::vector<std::string> agg_names, GroupKeyType key_type)
        : child_(std::move(child)),
          group_columns_(std::move(group_columns)),
          group_names_(std::move(group_names)),
          factories_(std::move(factories)),
          agg_names_(std::move(agg_names)),
          key_type_(key_type) {
    }

    std::optional<EngineBatch> GetNext() override;

private:
    struct AggSlot {
        int64_t count{0};
        std::vector<std::unique_ptr<Aggregator>> rest;
    };


    struct ColDesc {
        size_t col_idx;
        bool is_str;
    };

    struct ColPtr {
        const int64_t* ints;
        const std::string_view* svs;
        const std::string* strs;
    };


    void Run();
    void ProcessBatch(EngineBatch& batch, const std::vector<ColDesc>& col_descs, bool only_count_all);
    void UpdateSlot(AggSlot& slot, EngineBatch& batch, RowIndex i, bool only_count_all);
    void BuildKey(RowIndex i, const std::vector<ColDesc>& col_descs, const std::vector<ColPtr>& ptrs, std::vector<char>& buf);

    bool ready_{false};
    size_t cur_idx_{0};
    bool has_count_all_{false};
    GroupKeyType key_type_;
    std::shared_ptr<Operator> child_;
    std::vector<size_t> group_columns_;
    std::vector<std::string> group_names_;
    std::vector<std::string> agg_names_;
    std::vector<AggFactory> factories_;

    HashMap<std::string_view, AggSlot, StringViewHash> groups_;
    Arena arena_;
    std::vector<bool> is_string_column_;
    std::vector<char> key_buffer_;
    std::vector<char> prefetch_key_buf_;
    std::vector<size_t> prefetch_hashes_;
    std::vector<std::pair<size_t, size_t>> prefetch_key_spans_;
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

class SortOp : public Operator {
public:
    SortOp(std::shared_ptr<Operator> child, size_t col_idx, bool reversed)
        : child_(std::move(child)), col_idx_(col_idx), reversed_(reversed) {
    }
    std::optional<EngineBatch> GetNext() override;

private:
    std::shared_ptr<Operator> child_;
    size_t col_idx_;
    bool reversed_;
};

class AsOp : public Operator {
public:
    AsOp(std::shared_ptr<Operator> child, std::string from, std::string to)
        : child_(std::move(child)), from_(std::move(from)), to_(std::move(to)) {
    }
    std::optional<EngineBatch> GetNext() override;
private:
    std::shared_ptr<Operator> child_;
    std::string from_;
    std::string to_;
};


class TopKOp : public Operator {
public:
    TopKOp(std::shared_ptr<Operator> child, size_t col_idx, bool reversed, size_t limit)
        : child_(std::move(child)), col_idx_(col_idx), reversed_(reversed), limit_(limit) {
    }
    TopKOp(std::shared_ptr<Operator> child, size_t col_idx, size_t col_idx_2, bool reversed, size_t limit)
        : child_(std::move(child)), col_idx_(col_idx), col_idx_2_(col_idx_2), reversed_(reversed), limit_(limit) {
    }
    std::optional<EngineBatch> GetNext() override;
private:
    std::shared_ptr<Operator> child_;
    size_t col_idx_;
    std::optional<size_t> col_idx_2_;
    bool reversed_;
    size_t limit_;
};

template <typename T>
class AddColOp : public Operator {
public:
    AddColOp(std::shared_ptr<Operator> child, std::shared_ptr<AddColFun> fun, size_t col_idx)
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
class AddCaseOp : public Operator {
public:
    AddCaseOp(std::shared_ptr<Operator> child, std::shared_ptr<AddColFun> then_fun,
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

class Pipeline;

class Engine {
public:
    explicit Engine(const std::string& path);
    EngineBatch Run(std::shared_ptr<Operator> root,
                    const std::vector<size_t>& selected_columns = {});
    std::shared_ptr<ScanOp> MakeScan();
    Pipeline Api();

private:
    Schema schema_;
    std::string path_;
    std::vector<BatchMetaData> batch_meta_;
};

}  // namespace column_engine::internal
