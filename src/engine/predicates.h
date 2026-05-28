#pragma once
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>
#include <vector>
#include "engine/batch.h"

namespace column_engine::internal {

inline std::string_view GetStrAt(const ColumnData& col, RowIndex i) {
    if (std::holds_alternative<std::vector<std::string_view>>(col)) {
        return std::get<std::vector<std::string_view>>(col)[i];
    }
    return std::get<std::vector<std::string>>(col)[i];
}

class FilterPredicate {
public:
    virtual bool Check(const EngineBatch& batch, RowIndex i) = 0;
    virtual std::vector<RowIndex> CheckBatch(const EngineBatch& batch, const std::vector<RowIndex>& selection) {
        std::vector<RowIndex> result;
        result.reserve(selection.size());
        for (auto i : selection) {
            if (Check(batch, i)) {
                result.push_back(i);
            }
        }
        return result;
    }
    virtual ~FilterPredicate() = default;
};

class IntConstNE : public FilterPredicate {
public:
    IntConstNE(size_t id_a, int64_t const_b) : id_a_(id_a), const_b_(const_b) {}
    [[gnu::always_inline]] bool Check(const EngineBatch& batch, RowIndex i) override {
        return std::get<std::vector<int64_t>>(batch.columns[id_a_])[i] != const_b_;
    }
private:
    size_t id_a_;
    int64_t const_b_;
};

class IntConstEQ : public FilterPredicate {
public:
    IntConstEQ(size_t id_a, int64_t const_b) : id_a_(id_a), const_b_(const_b) {}
    bool Check(const EngineBatch& batch, RowIndex i) override {
        return std::get<std::vector<int64_t>>(batch.columns[id_a_])[i] == const_b_;
    }
private:
    size_t id_a_;
    int64_t const_b_;
};

class StrConstNE : public FilterPredicate {
public:
    StrConstNE(size_t id_a, std::string const_b) : id_a_(id_a), const_b_(std::move(const_b)) {}
    bool Check(const EngineBatch& batch, RowIndex i) override;
    std::vector<RowIndex> CheckBatch(const EngineBatch& batch, const std::vector<RowIndex>& selection) override;
private:
    size_t id_a_;
    std::string const_b_;
};

class StrConstEQ : public FilterPredicate {
public:
    StrConstEQ(size_t id_a, std::string const_b) : id_a_(id_a), const_b_(std::move(const_b)) {}
    bool Check(const EngineBatch& batch, RowIndex i) override;
    std::vector<RowIndex> CheckBatch(const EngineBatch& batch, const std::vector<RowIndex>& selection) override;
private:
    size_t id_a_;
    std::string const_b_;
};

class IntConstLT : public FilterPredicate {
public:
    IntConstLT(size_t id_a, int64_t const_b) : id_a_(id_a), const_b_(const_b) {}
    bool Check(const EngineBatch& batch, RowIndex i) override {
        return std::get<std::vector<int64_t>>(batch.columns[id_a_])[i] < const_b_;
    }
private:
    size_t id_a_;
    int64_t const_b_;
};

class IntConstLE : public FilterPredicate {
public:
    IntConstLE(size_t id_a, int64_t const_b) : id_a_(id_a), const_b_(const_b) {}
    bool Check(const EngineBatch& batch, RowIndex i) override {
        return std::get<std::vector<int64_t>>(batch.columns[id_a_])[i] <= const_b_;
    }
private:
    size_t id_a_;
    int64_t const_b_;
};

class IntConstGT : public FilterPredicate {
public:
    IntConstGT(size_t id_a, int64_t const_b) : id_a_(id_a), const_b_(const_b) {}
    bool Check(const EngineBatch& batch, RowIndex i) override {
        return std::get<std::vector<int64_t>>(batch.columns[id_a_])[i] > const_b_;
    }
private:
    size_t id_a_;
    int64_t const_b_;
};

class IntConstGE : public FilterPredicate {
public:
    IntConstGE(size_t id_a, int64_t const_b) : id_a_(id_a), const_b_(const_b) {}
    bool Check(const EngineBatch& batch, RowIndex i) override {
        return std::get<std::vector<int64_t>>(batch.columns[id_a_])[i] >= const_b_;
    }
private:
    size_t id_a_;
    int64_t const_b_;
};

class StrConstLT : public FilterPredicate {
public:
    StrConstLT(size_t id_a, std::string const_b) : id_a_(id_a), const_b_(std::move(const_b)) {}
    bool Check(const EngineBatch& batch, RowIndex i) override {
        return GetStrAt(batch.columns[id_a_], i) < const_b_;
    }
private:
    size_t id_a_;
    std::string const_b_;
};

class StrConstLE : public FilterPredicate {
public:
    StrConstLE(size_t id_a, std::string const_b) : id_a_(id_a), const_b_(std::move(const_b)) {}
    bool Check(const EngineBatch& batch, RowIndex i) override {
        return GetStrAt(batch.columns[id_a_], i) <= const_b_;
    }
private:
    size_t id_a_;
    std::string const_b_;
};

class StrConstGT : public FilterPredicate {
public:
    StrConstGT(size_t id_a, std::string const_b) : id_a_(id_a), const_b_(std::move(const_b)) {}
    bool Check(const EngineBatch& batch, RowIndex i) override {
        return GetStrAt(batch.columns[id_a_], i) > const_b_;
    }
private:
    size_t id_a_;
    std::string const_b_;
};

class StrConstGE : public FilterPredicate {
public:
    StrConstGE(size_t id_a, std::string const_b) : id_a_(id_a), const_b_(std::move(const_b)) {}
    bool Check(const EngineBatch& batch, RowIndex i) override {
        return GetStrAt(batch.columns[id_a_], i) >= const_b_;
    }
private:
    size_t id_a_;
    std::string const_b_;
};

class IntConstIN : public FilterPredicate {
public:
    IntConstIN(size_t id_a, std::vector<int64_t> values)
        : id_a_(id_a), values_(values.begin(), values.end()) {}
    bool Check(const EngineBatch& batch, RowIndex i) override {
        return values_.count(std::get<std::vector<int64_t>>(batch.columns[id_a_])[i]) > 0;
    }
private:
    size_t id_a_;
    std::unordered_set<int64_t> values_;
};

class StrLike : public FilterPredicate {
public:
    StrLike(size_t id_a, std::string pattern);
    bool Check(const EngineBatch& batch, RowIndex i) override;
private:
    size_t id_a_;
    std::string prefix_;
    std::string suffix_;
    std::string infix_;
};

class StrNotLike : public FilterPredicate {
public:
    StrNotLike(size_t id_a, std::string pattern);
    bool Check(const EngineBatch& batch, RowIndex i) override;
private:
    StrLike inner_;
};

}  // namespace column_engine::internal
