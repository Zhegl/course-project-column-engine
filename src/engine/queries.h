#pragma once
#include <limits>
#include <optional>
#include <unordered_set>
#include "batch.h"
#include "types/types.h"

namespace column_engine {

class Aggregator {
public:
    virtual void Next(EngineBatch& batch, RowIndex i) = 0;
    virtual ColumnData GetResult() = 0;
    virtual std::string GetName() = 0;
    virtual ~Aggregator() = default;
};

class CountAll : public Aggregator {
public:
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() override;
    std::string GetName() override {
        return "COUNT(*)";
    }

private:
    size_t count_{0};
};

class IntCountDistinct : public Aggregator {
public:
    explicit IntCountDistinct(size_t col_idx, std::string name)
        : col_idx_(col_idx), name_(std::move(name)) {}
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() override;
    std::string GetName() override { return "COUNT(DISTINCT " + name_ + ")"; }

private:
    size_t col_idx_;
    std::string name_;
    std::unordered_set<int64_t> values_;
};

class StrCountDistinct : public Aggregator {
public:
    explicit StrCountDistinct(size_t col_idx, std::string name)
        : col_idx_(col_idx), name_(std::move(name)) {}
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() override;
    std::string GetName() override { return "COUNT(DISTINCT " + name_ + ")"; }

private:
    size_t col_idx_;
    std::string name_;
    std::unordered_set<std::string> values_;
};


class IntSum : public Aggregator {
public:
    explicit IntSum(size_t col_idx, std::string name)
        : col_idx_(col_idx), name_(std::move(name)) {}
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() override;
    std::string GetName() override { return "SUM(" + name_ + ")"; }
private:
    size_t col_idx_;
    std::string name_;
    int64_t sum_{0};
};

class IntMin : public Aggregator {
public:
    explicit IntMin(size_t col_idx, std::string name)
        : col_idx_(col_idx), name_(std::move(name)) {}
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() override;
    std::string GetName() override { return "MIN(" + name_ + ")"; }
private:
    size_t col_idx_;
    std::string name_;
    int64_t min_{std::numeric_limits<int64_t>::max()};
};

class IntMax : public Aggregator {
public:
    explicit IntMax(size_t col_idx, std::string name)
        : col_idx_(col_idx), name_(std::move(name)) {}
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() override;
    std::string GetName() override { return "MAX(" + name_ + ")"; }
private:
    size_t col_idx_;
    std::string name_;
    int64_t max_{std::numeric_limits<int64_t>::min()};
};

class IntAvg : public Aggregator {
public:
    explicit IntAvg(size_t col_idx, std::string name)
        : col_idx_(col_idx), name_(std::move(name)) {}
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() override;
    std::string GetName() override { return "AVG(" + name_ + ")"; }
private:
    size_t col_idx_;
    std::string name_;
    int64_t sum_{0};
    size_t count_{0};
};

class StrMin : public Aggregator {
public:
    explicit StrMin(size_t col_idx, std::string name)
        : col_idx_(col_idx), name_(std::move(name)) {}
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() override;
    std::string GetName() override { return "MIN(" + name_ + ")"; }
private:
    size_t col_idx_;
    std::string name_;
    std::optional<std::string> min_;
};

class StrMax : public Aggregator {
public:
    explicit StrMax(size_t col_idx, std::string name)
        : col_idx_(col_idx), name_(std::move(name)) {}
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() override;
    std::string GetName() override { return "MAX(" + name_ + ")"; }
private:
    size_t col_idx_;
    std::string name_;
    std::optional<std::string> max_;
};

class FilterPredicate {
public:
    virtual bool Check(EngineBatch& batch, RowIndex i) = 0;
    virtual ~FilterPredicate() = default;
};

class IntConstNE : public FilterPredicate {
public:
    IntConstNE(size_t id_a, int64_t const_b);
    bool Check(EngineBatch& batch, RowIndex i);

private:
    size_t id_a_;
    int64_t const_b_;
};

class IntConstEQ : public FilterPredicate {
public:
    IntConstEQ(size_t id_a, int64_t const_b);
    bool Check(EngineBatch& batch, RowIndex i);

private:
    size_t id_a_;
    int64_t const_b_;
};

class StrConstNE : public FilterPredicate {
public:
    StrConstNE(size_t id_a, std::string const_b);
    bool Check(EngineBatch& batch, RowIndex i) override;

private:
    size_t id_a_;
    std::string const_b_;
};

class StrConstEQ : public FilterPredicate {
public:
    StrConstEQ(size_t id_a, std::string const_b);
    bool Check(EngineBatch& batch, RowIndex i) override;

private:
    size_t id_a_;
    std::string const_b_;
};

class IntConstLT : public FilterPredicate {
public:
    IntConstLT(size_t id_a, int64_t const_b) : id_a_(id_a), const_b_(const_b) {}
    bool Check(EngineBatch& batch, RowIndex i) override {
        return std::get<std::vector<int64_t>>(batch.columns[id_a_])[i] < const_b_;
    }
private:
    size_t id_a_; int64_t const_b_;
};

class IntConstLE : public FilterPredicate {
public:
    IntConstLE(size_t id_a, int64_t const_b) : id_a_(id_a), const_b_(const_b) {}
    bool Check(EngineBatch& batch, RowIndex i) override {
        return std::get<std::vector<int64_t>>(batch.columns[id_a_])[i] <= const_b_;
    }
private:
    size_t id_a_; int64_t const_b_;
};

class IntConstGT : public FilterPredicate {
public:
    IntConstGT(size_t id_a, int64_t const_b) : id_a_(id_a), const_b_(const_b) {}
    bool Check(EngineBatch& batch, RowIndex i) override {
        return std::get<std::vector<int64_t>>(batch.columns[id_a_])[i] > const_b_;
    }
private:
    size_t id_a_; int64_t const_b_;
};

class IntConstGE : public FilterPredicate {
public:
    IntConstGE(size_t id_a, int64_t const_b) : id_a_(id_a), const_b_(const_b) {}
    bool Check(EngineBatch& batch, RowIndex i) override {
        return std::get<std::vector<int64_t>>(batch.columns[id_a_])[i] >= const_b_;
    }
private:
    size_t id_a_; int64_t const_b_;
};

class StrConstLT : public FilterPredicate {
public:
    StrConstLT(size_t id_a, std::string const_b) : id_a_(id_a), const_b_(std::move(const_b)) {}
    bool Check(EngineBatch& batch, RowIndex i) override {
        return std::get<std::vector<std::string>>(batch.columns[id_a_])[i] < const_b_;
    }
private:
    size_t id_a_; std::string const_b_;
};

class StrConstLE : public FilterPredicate {
public:
    StrConstLE(size_t id_a, std::string const_b) : id_a_(id_a), const_b_(std::move(const_b)) {}
    bool Check(EngineBatch& batch, RowIndex i) override {
        return std::get<std::vector<std::string>>(batch.columns[id_a_])[i] <= const_b_;
    }
private:
    size_t id_a_; std::string const_b_;
};

class StrConstGT : public FilterPredicate {
public:
    StrConstGT(size_t id_a, std::string const_b) : id_a_(id_a), const_b_(std::move(const_b)) {}
    bool Check(EngineBatch& batch, RowIndex i) override {
        return std::get<std::vector<std::string>>(batch.columns[id_a_])[i] > const_b_;
    }
private:
    size_t id_a_; std::string const_b_;
};

class StrConstGE : public FilterPredicate {
public:
    StrConstGE(size_t id_a, std::string const_b) : id_a_(id_a), const_b_(std::move(const_b)) {}
    bool Check(EngineBatch& batch, RowIndex i) override {
        return std::get<std::vector<std::string>>(batch.columns[id_a_])[i] >= const_b_;
    }
private:
    size_t id_a_; std::string const_b_;
};

class StrLike : public FilterPredicate {
public:
    StrLike(size_t id_a, std::string pattern);
    bool Check(EngineBatch& batch, RowIndex i) override;

private:
    size_t id_a_;
    std::string prefix_;
    std::string suffix_;
    std::string infix_;  // non-empty only for %x% patterns
};

class StrNotLike : public FilterPredicate {
public:
    StrNotLike(size_t id_a, std::string pattern);
    bool Check(EngineBatch& batch, RowIndex i) override;

private:
    StrLike inner_;
};


class AddColFun {
public:
    virtual ColumnValue Get(EngineBatch& batch, RowIndex i) = 0;
    virtual std::string GetName() = 0;
    virtual ~AddColFun() = default;
};

class IntLiteral : public AddColFun {
public:
    explicit IntLiteral(int64_t val) : val_(val) {}
    ColumnValue Get(EngineBatch&, RowIndex) override { return val_; }
    std::string GetName() override { return std::to_string(val_); }
private:
    int64_t val_;
};

class IntOffset : public AddColFun {
public:
    IntOffset(size_t col_idx, std::string col_name, int64_t delta)
        : col_idx_(col_idx), col_name_(std::move(col_name)), delta_(delta) {}
    ColumnValue Get(EngineBatch& batch, RowIndex i) override {
        return std::get<std::vector<int64_t>>(batch.columns[col_idx_])[i] + delta_;
    }
    std::string GetName() override {
        if (delta_ >= 0) {
            return col_name_ + " + " + std::to_string(delta_);
        }
        return col_name_ + " - " + std::to_string(-delta_);
    }
private:
    size_t col_idx_;
    std::string col_name_;
    int64_t delta_;
};

class StrLen : public AddColFun {
public:
    StrLen(size_t col_idx, std::string col_name)
        : col_idx_(col_idx), col_name_(std::move(col_name)) {}
    ColumnValue Get(EngineBatch& batch, RowIndex i) override {
        return static_cast<int64_t>(
            std::get<std::vector<std::string>>(batch.columns[col_idx_])[i].size());
    }
    std::string GetName() override { return "length(" + col_name_ + ")"; }
private:
    size_t col_idx_;
    std::string col_name_;
};

class Strftime : public AddColFun {
public:
    Strftime(size_t col_idx, std::string col_name, std::string fmt)
        : col_idx_(col_idx), col_name_(std::move(col_name)), fmt_(std::move(fmt)) {}
    ColumnValue Get(EngineBatch& batch, RowIndex i) override;
    std::string GetName() override { return "strftime('" + fmt_ + "', " + col_name_ + ")"; }
private:
    size_t col_idx_;
    std::string col_name_;
    std::string fmt_;
};

}  // namespace column_engine
