#include "engine/aggregators.h"

namespace column_engine::internal {

void CountAll::Next(EngineBatch& batch, RowIndex i) {
    ++count_;
}

ColumnData CountAll::GetResult() {
    return std::vector<int64_t>{static_cast<int64_t>(count_)};
}

void IntCountDistinct::Next(EngineBatch& batch, RowIndex i) {
    values_.insert(std::get<std::vector<int64_t>>(batch.columns[col_idx_])[i]);
}

ColumnData IntCountDistinct::GetResult() {
    return std::vector<int64_t>{static_cast<int64_t>(values_.size())};
}

void StrCountDistinct::Next(EngineBatch& batch, RowIndex i) {
    values_.insert(std::get<std::vector<std::string>>(batch.columns[col_idx_])[i]);
}

ColumnData StrCountDistinct::GetResult() {
    return std::vector<int64_t>{static_cast<int64_t>(values_.size())};
}

void IntSum::Next(EngineBatch& batch, RowIndex i) {
    sum_ += std::get<std::vector<int64_t>>(batch.columns[col_idx_])[i];
}
ColumnData IntSum::GetResult() {
    return std::vector<int64_t>{sum_};
}

void IntMin::Next(EngineBatch& batch, RowIndex i) {
    int64_t v = std::get<std::vector<int64_t>>(batch.columns[col_idx_])[i];
    if (v < min_) {
        min_ = v;
    }
}
ColumnData IntMin::GetResult() {
    return std::vector<int64_t>{min_};
}

void IntMax::Next(EngineBatch& batch, RowIndex i) {
    int64_t v = std::get<std::vector<int64_t>>(batch.columns[col_idx_])[i];
    if (v > max_) {
        max_ = v;
    }
}
ColumnData IntMax::GetResult() {
    return std::vector<int64_t>{max_};
}

void IntAvg::Next(EngineBatch& batch, RowIndex i) {
    sum_ += std::get<std::vector<int64_t>>(batch.columns[col_idx_])[i];
    ++count_;
}
ColumnData IntAvg::GetResult() {
    return std::vector<int64_t>{static_cast<int64_t>(count_ > 0 ? sum_ / count_ : 0)};
}

void StrMin::Next(EngineBatch& batch, RowIndex i) {
    const auto& v = std::get<std::vector<std::string>>(batch.columns[col_idx_])[i];
    if (!min_ || v < *min_) {
        min_ = v;
    }
}
ColumnData StrMin::GetResult() {
    return std::vector<std::string>{min_.value_or("")};
}

void StrMax::Next(EngineBatch& batch, RowIndex i) {
    const auto& v = std::get<std::vector<std::string>>(batch.columns[col_idx_])[i];
    if (!max_ || v > *max_) {
        max_ = v;
    }
}
ColumnData StrMax::GetResult() {
    return std::vector<std::string>{max_.value_or("")};
}

}  // namespace column_engine::internal
