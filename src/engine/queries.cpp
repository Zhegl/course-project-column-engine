#include "queries.h"
#include <cstdint>
#include <variant>
#include <vector>
#include "types/types.h"

namespace column_engine {
void CountAll::Next(EngineBatch& batch, uint16_t i) {
    ++count_;
}

ColumnData CountAll::GetResult() {
    return std::vector<int64_t>{static_cast<int64_t>(count_)};
}

void IntSum::Next(EngineBatch& batch, uint16_t i) {
    sum_ += std::get<std::vector<int64_t>>(batch.columns[col_idx_])[i];
}
ColumnData IntSum::GetResult() {
    return std::vector<int64_t>{sum_};
}

void IntMin::Next(EngineBatch& batch, uint16_t i) {
    int64_t v = std::get<std::vector<int64_t>>(batch.columns[col_idx_])[i];
    if (v < min_) {
        min_ = v;
    }
}
ColumnData IntMin::GetResult() {
    return std::vector<int64_t>{min_};
}

void IntMax::Next(EngineBatch& batch, uint16_t i) {
    int64_t v = std::get<std::vector<int64_t>>(batch.columns[col_idx_])[i];
    if (v > max_) {
        max_ = v;
    }
}
ColumnData IntMax::GetResult() {
    return std::vector<int64_t>{max_};
}

void IntAvg::Next(EngineBatch& batch, uint16_t i) {
    sum_ += std::get<std::vector<int64_t>>(batch.columns[col_idx_])[i];
    ++count_;
}
ColumnData IntAvg::GetResult() {
    return std::vector<int64_t>{static_cast<long>(count_ > 0 ? sum_ / count_ : 0)};
}

void StrMin::Next(EngineBatch& batch, uint16_t i) {
    const auto& v = std::get<std::vector<std::string>>(batch.columns[col_idx_])[i];
    if (!min_ || v < *min_) {
        min_ = v;
    }
}
ColumnData StrMin::GetResult() {
    return std::vector<std::string>{min_.value_or("")};
}

void StrMax::Next(EngineBatch& batch, uint16_t i) {
    const auto& v = std::get<std::vector<std::string>>(batch.columns[col_idx_])[i];
    if (!max_ || v > *max_) {
        max_ = v;
    }
}
ColumnData StrMax::GetResult() {
    return std::vector<std::string>{max_.value_or("")};
}

IntConstNE::IntConstNE(size_t id_a, int64_t const_b) : id_a_(id_a), const_b_(const_b) {
}

bool IntConstNE::Check(EngineBatch& batch, uint16_t i) {
    return std::get<std::vector<int64_t>>(batch.columns[id_a_])[i] != const_b_;
}

IntConstEQ::IntConstEQ(size_t id_a, int64_t const_b) : id_a_(id_a), const_b_(const_b) {
}

bool IntConstEQ::Check(EngineBatch& batch, uint16_t i) {
    return std::get<std::vector<int64_t>>(batch.columns[id_a_])[i] == const_b_;
}

}  // namespace column_engine
