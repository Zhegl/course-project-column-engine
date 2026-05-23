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
    virtual size_t StateSize() const = 0;
    virtual void InitState(char* state) const = 0;
    virtual void NextRaw(char* state, EngineBatch& batch, RowIndex i) const = 0;
    virtual ColumnData GetResultRaw(char* state) const = 0;
    virtual void Destroy(char*) const {}

    // legacy — используются только для построения пустых колонок результата
    virtual void Next(EngineBatch& batch, RowIndex i) {}
    virtual ColumnData GetResult() { return std::vector<int64_t>{}; }
    virtual ~Aggregator() = default;
};

class CountAll : public Aggregator {
public:
    size_t StateSize() const override { return sizeof(int64_t); }
    void InitState(char* p) const override { *reinterpret_cast<int64_t*>(p) = 0; }
    void NextRaw(char* p, EngineBatch&, RowIndex) const override {
        ++*reinterpret_cast<int64_t*>(p);
    }
    ColumnData GetResultRaw(char* p) const override {
        return std::vector<int64_t>{*reinterpret_cast<int64_t*>(p)};
    }
};

class IntSum : public Aggregator {
public:
    explicit IntSum(size_t col_idx) : col_idx_(col_idx) {}
    size_t StateSize() const override { return sizeof(int64_t); }
    void InitState(char* p) const override { *reinterpret_cast<int64_t*>(p) = 0; }
    void NextRaw(char* p, EngineBatch& batch, RowIndex i) const override {
        *reinterpret_cast<int64_t*>(p) +=
            std::get<std::vector<int64_t>>(batch.columns[col_idx_])[i];
    }
    ColumnData GetResultRaw(char* p) const override {
        return std::vector<int64_t>{*reinterpret_cast<int64_t*>(p)};
    }
private:
    size_t col_idx_;
};

class IntMin : public Aggregator {
public:
    explicit IntMin(size_t col_idx) : col_idx_(col_idx) {}
    size_t StateSize() const override { return sizeof(int64_t); }
    void InitState(char* p) const override {
        *reinterpret_cast<int64_t*>(p) = std::numeric_limits<int64_t>::max();
    }
    void NextRaw(char* p, EngineBatch& batch, RowIndex i) const override {
        int64_t v = std::get<std::vector<int64_t>>(batch.columns[col_idx_])[i];
        int64_t& cur = *reinterpret_cast<int64_t*>(p);
        if (v < cur) { cur = v; }
    }
    ColumnData GetResultRaw(char* p) const override {
        return std::vector<int64_t>{*reinterpret_cast<int64_t*>(p)};
    }
private:
    size_t col_idx_;
};

class IntMax : public Aggregator {
public:
    explicit IntMax(size_t col_idx) : col_idx_(col_idx) {}
    size_t StateSize() const override { return sizeof(int64_t); }
    void InitState(char* p) const override {
        *reinterpret_cast<int64_t*>(p) = std::numeric_limits<int64_t>::min();
    }
    void NextRaw(char* p, EngineBatch& batch, RowIndex i) const override {
        int64_t v = std::get<std::vector<int64_t>>(batch.columns[col_idx_])[i];
        int64_t& cur = *reinterpret_cast<int64_t*>(p);
        if (v > cur) { cur = v; }
    }
    ColumnData GetResultRaw(char* p) const override {
        return std::vector<int64_t>{*reinterpret_cast<int64_t*>(p)};
    }
private:
    size_t col_idx_;
};

struct IntAvgState {
    __int128 sum;
    int64_t count;
};

class IntAvg : public Aggregator {
public:
    explicit IntAvg(size_t col_idx) : col_idx_(col_idx) {}
    size_t StateSize() const override { return sizeof(IntAvgState); }
    void InitState(char* p) const override {
        new(p) IntAvgState{0, 0};
    }
    void NextRaw(char* p, EngineBatch& batch, RowIndex i) const override {
        auto* s = reinterpret_cast<IntAvgState*>(p);
        s->sum += std::get<std::vector<int64_t>>(batch.columns[col_idx_])[i];
        ++s->count;
    }
    ColumnData GetResultRaw(char* p) const override {
        auto* s = reinterpret_cast<IntAvgState*>(p);
        return std::vector<int64_t>{s->count > 0 ? static_cast<int64_t>(s->sum / s->count) : 0};
    }
private:
    size_t col_idx_;
};

class StrMin : public Aggregator {
public:
    explicit StrMin(size_t col_idx) : col_idx_(col_idx) {}
    size_t StateSize() const override { return sizeof(std::string*); }
    void InitState(char* p) const override { *reinterpret_cast<std::string**>(p) = nullptr; }
    void Destroy(char* p) const override { delete *reinterpret_cast<std::string**>(p); }
    void NextRaw(char* p, EngineBatch& batch, RowIndex i) const override {
        std::string*& s = *reinterpret_cast<std::string**>(p);
        const auto& v = GetStrAt(batch.columns[col_idx_], i);
        if (!s || v < *s) {
            if (!s) { s = new std::string(v); } else { *s = v; }
        }
    }
    ColumnData GetResultRaw(char* p) const override {
        std::string* s = *reinterpret_cast<std::string**>(p);
        return std::vector<std::string>{s ? *s : ""};
    }
private:
    size_t col_idx_;
};

class StrMax : public Aggregator {
public:
    explicit StrMax(size_t col_idx) : col_idx_(col_idx) {}
    size_t StateSize() const override { return sizeof(std::string*); }
    void InitState(char* p) const override { *reinterpret_cast<std::string**>(p) = nullptr; }
    void Destroy(char* p) const override { delete *reinterpret_cast<std::string**>(p); }
    void NextRaw(char* p, EngineBatch& batch, RowIndex i) const override {
        std::string*& s = *reinterpret_cast<std::string**>(p);
        const auto& v = GetStrAt(batch.columns[col_idx_], i);
        if (!s || v > *s) {
            if (!s) { s = new std::string(v); } else { *s = v; }
        }
    }
    ColumnData GetResultRaw(char* p) const override {
        std::string* s = *reinterpret_cast<std::string**>(p);
        return std::vector<std::string>{s ? *s : ""};
    }
private:
    size_t col_idx_;
};

using IntCDMap = HashMap<int64_t, std::monostate, IntHash>;
using StrCDMap = HashMap<std::string, std::monostate, StrHash>;

class IntCountDistinct : public Aggregator {
public:
    explicit IntCountDistinct(size_t col_idx) : col_idx_(col_idx) {}
    size_t StateSize() const override { return sizeof(IntCDMap*); }
    void InitState(char* p) const override { *reinterpret_cast<IntCDMap**>(p) = new IntCDMap(); }
    void Destroy(char* p) const override { delete *reinterpret_cast<IntCDMap**>(p); }
    void NextRaw(char* p, EngineBatch& batch, RowIndex i) const override {
        (**reinterpret_cast<IntCDMap**>(p))[std::get<std::vector<int64_t>>(batch.columns[col_idx_])[i]];
    }
    ColumnData GetResultRaw(char* p) const override {
        return std::vector<int64_t>{static_cast<int64_t>((*reinterpret_cast<IntCDMap**>(p))->Size())};
    }
private:
    size_t col_idx_;
};

class StrCountDistinct : public Aggregator {
public:
    explicit StrCountDistinct(size_t col_idx) : col_idx_(col_idx) {}
    size_t StateSize() const override { return sizeof(StrCDMap*); }
    void InitState(char* p) const override { *reinterpret_cast<StrCDMap**>(p) = new StrCDMap(); }
    void Destroy(char* p) const override { delete *reinterpret_cast<StrCDMap**>(p); }
    void NextRaw(char* p, EngineBatch& batch, RowIndex i) const override {
        (**reinterpret_cast<StrCDMap**>(p))[std::string(GetStrAt(batch.columns[col_idx_], i))];
    }
    ColumnData GetResultRaw(char* p) const override {
        return std::vector<int64_t>{static_cast<int64_t>((*reinterpret_cast<StrCDMap**>(p))->Size())};
    }
private:
    size_t col_idx_;
};

}  // namespace column_engine::internal
