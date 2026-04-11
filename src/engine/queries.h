#pragma once
#include <limits>
#include <optional>
#include "batch.h"

namespace column_engine {

class Aggregator {
public:
    virtual void Next(EngineBatch& batch, uint16_t i) = 0;
    virtual ColumnData GetResult() = 0;
    virtual std::string GetName() = 0;
    virtual ~Aggregator() = default;
};

class CountAll : public Aggregator {
public:
    void Next(EngineBatch& batch, uint16_t i) override;
    ColumnData GetResult() override;
    std::string GetName() override {
        return "COUNT(*)";
    }

private:
    size_t count_{0};
};


class IntSum : public Aggregator {
public:
    explicit IntSum(size_t col_idx, std::string name)
        : col_idx_(col_idx), name_(std::move(name)) {}
    void Next(EngineBatch& batch, uint16_t i) override;
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
    void Next(EngineBatch& batch, uint16_t i) override;
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
    void Next(EngineBatch& batch, uint16_t i) override;
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
    void Next(EngineBatch& batch, uint16_t i) override;
    ColumnData GetResult() override;
    std::string GetName() override { return "AVG(" + name_ + ")"; }
private:
    size_t col_idx_;
    std::string name_;
    int64_t sum_{0};
    size_t count_{0};
};

class FilterPredicate {
public:
    virtual bool Check(EngineBatch& batch, uint16_t i) = 0;
    virtual ~FilterPredicate() = default;
};

class IntConstNE : public FilterPredicate {
public:
    IntConstNE(size_t id_a, int64_t const_b);
    bool Check(EngineBatch& batch, uint16_t i);

private:
    size_t id_a_;
    int64_t const_b_;
};

}  // namespace column_engine