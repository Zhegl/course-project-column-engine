#pragma once
#include <limits>
#include <optional>
#include <string>
#include <variant>
#include "engine/batch.h"
#include "types/types.h"
#include "engine/hashmap.h"

namespace column_engine::internal {

class Aggregator {
public:
    virtual void Next(EngineBatch& batch, RowIndex i) = 0;
    virtual ColumnData GetResult() const = 0;
    virtual ~Aggregator() = default;
};

class CountAll : public Aggregator {
public:
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() const override;

private:
    size_t count_{0};
};

class IntCountDistinct : public Aggregator {
public:
    explicit IntCountDistinct(size_t col_idx) : col_idx_(col_idx) {}
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() const override;

private:
    size_t col_idx_;
    HashMap<int64_t, std::monostate, IntHash> values_;
};

class StrCountDistinct : public Aggregator {
public:
    explicit StrCountDistinct(size_t col_idx) : col_idx_(col_idx) {}
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() const override;

private:
    size_t col_idx_;
    HashMap<std::string, std::monostate, StrHash> values_;
};

class IntSum : public Aggregator {
public:
    explicit IntSum(size_t col_idx) : col_idx_(col_idx) {}
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() const override;

private:
    size_t col_idx_;
    int64_t sum_{0};
};

class IntMin : public Aggregator {
public:
    explicit IntMin(size_t col_idx) : col_idx_(col_idx) {}
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() const override;

private:
    size_t col_idx_;
    int64_t min_{std::numeric_limits<int64_t>::max()};
};

class IntMax : public Aggregator {
public:
    explicit IntMax(size_t col_idx) : col_idx_(col_idx) {}
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() const override;

private:
    size_t col_idx_;
    int64_t max_{std::numeric_limits<int64_t>::min()};
};

class IntAvg : public Aggregator {
public:
    explicit IntAvg(size_t col_idx) : col_idx_(col_idx) {}
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() const override;

private:
    size_t col_idx_;
    __int128 sum_{0};
    size_t count_{0};
};

class StrMin : public Aggregator {
public:
    explicit StrMin(size_t col_idx) : col_idx_(col_idx) {}
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() const override;

private:
    size_t col_idx_;
    std::optional<std::string> min_;
};

class StrMax : public Aggregator {
public:
    explicit StrMax(size_t col_idx) : col_idx_(col_idx) {}
    void Next(EngineBatch& batch, RowIndex i) override;
    ColumnData GetResult() const override;

private:
    size_t col_idx_;
    std::optional<std::string> max_;
};

}  // namespace column_engine::internal
