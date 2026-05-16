#pragma once
#include <limits>
#include <optional>
#include <string>
#include <unordered_set>
#include <variant>
#include "engine/batch.h"
#include "engine/predicates.h"
#include "types/types.h"
#include "hashmap.h"

namespace column_engine::internal {

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
        : col_idx_(col_idx), name_(std::move(name)) {
    }
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() override;
    std::string GetName() override {
        return "COUNT(DISTINCT " + name_ + ")";
    }

private:
    size_t col_idx_;
    std::string name_;
    HashMap<int64_t, std::monostate, IntHash> values_;
};

class StrCountDistinct : public Aggregator {
public:
    explicit StrCountDistinct(size_t col_idx, std::string name)
        : col_idx_(col_idx), name_(std::move(name)) {
    }
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() override;
    std::string GetName() override {
        return "COUNT(DISTINCT " + name_ + ")";
    }

private:
    size_t col_idx_;
    std::string name_;
    HashMap<std::string, std::monostate, StrHash> values_;
};

class IntSum : public Aggregator {
public:
    explicit IntSum(size_t col_idx, std::string name) : col_idx_(col_idx), name_(std::move(name)) {
    }
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() override;
    std::string GetName() override {
        return "SUM(" + name_ + ")";
    }

private:
    size_t col_idx_;
    std::string name_;
    int64_t sum_{0};
};

class IntMin : public Aggregator {
public:
    explicit IntMin(size_t col_idx, std::string name) : col_idx_(col_idx), name_(std::move(name)) {
    }
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() override;
    std::string GetName() override {
        return "MIN(" + name_ + ")";
    }

private:
    size_t col_idx_;
    std::string name_;
    int64_t min_{std::numeric_limits<int64_t>::max()};
};

class IntMax : public Aggregator {
public:
    explicit IntMax(size_t col_idx, std::string name) : col_idx_(col_idx), name_(std::move(name)) {
    }
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() override;
    std::string GetName() override {
        return "MAX(" + name_ + ")";
    }

private:
    size_t col_idx_;
    std::string name_;
    int64_t max_{std::numeric_limits<int64_t>::min()};
};

class IntAvg : public Aggregator {
public:
    explicit IntAvg(size_t col_idx, std::string name) : col_idx_(col_idx), name_(std::move(name)) {
    }
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() override;
    std::string GetName() override {
        return "AVG(" + name_ + ")";
    }

private:
    size_t col_idx_;
    std::string name_;
    __int128 sum_{0};
    size_t count_{0};
};

class StrMin : public Aggregator {
public:
    explicit StrMin(size_t col_idx, std::string name) : col_idx_(col_idx), name_(std::move(name)) {
    }
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() override;
    std::string GetName() override {
        return "MIN(" + name_ + ")";
    }

private:
    size_t col_idx_;
    std::string name_;
    std::optional<std::string> min_;
};

class StrMax : public Aggregator {
public:
    explicit StrMax(size_t col_idx, std::string name) : col_idx_(col_idx), name_(std::move(name)) {
    }
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() override;
    std::string GetName() override {
        return "MAX(" + name_ + ")";
    }

private:
    size_t col_idx_;
    std::string name_;
    std::optional<std::string> max_;
};

}  // namespace column_engine::internal
