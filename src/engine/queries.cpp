#include "queries.h"
#include <cstdint>
#include <ctime>
#include <variant>
#include <vector>
#include "types/types.h"

namespace column_engine {
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
    return std::vector<int64_t>{static_cast<long>(count_ > 0 ? sum_ / count_ : 0)};
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

IntConstNE::IntConstNE(size_t id_a, int64_t const_b) : id_a_(id_a), const_b_(const_b) {
}

bool IntConstNE::Check(EngineBatch& batch, RowIndex i) {
    return std::get<std::vector<int64_t>>(batch.columns[id_a_])[i] != const_b_;
}

IntConstEQ::IntConstEQ(size_t id_a, int64_t const_b) : id_a_(id_a), const_b_(const_b) {
}

bool IntConstEQ::Check(EngineBatch& batch, RowIndex i) {
    return std::get<std::vector<int64_t>>(batch.columns[id_a_])[i] == const_b_;
}

StrConstNE::StrConstNE(size_t id_a, std::string const_b)
    : id_a_(id_a), const_b_(std::move(const_b)) {
}

bool StrConstNE::Check(EngineBatch& batch, RowIndex i) {
    return std::get<std::vector<std::string>>(batch.columns[id_a_])[i] != const_b_;
}

StrConstEQ::StrConstEQ(size_t id_a, std::string const_b)
    : id_a_(id_a), const_b_(std::move(const_b)) {
}

bool StrConstEQ::Check(EngineBatch& batch, RowIndex i) {
    return std::get<std::vector<std::string>>(batch.columns[id_a_])[i] == const_b_;
}

static bool LikeMatch(const std::string& str, const std::string& pattern) {
    // split pattern by '%' and check each part appears in order
    std::vector<std::string_view> parts;
    size_t start = 0;
    bool anchored_start = !pattern.empty() && pattern[0] != '%';
    bool anchored_end = !pattern.empty() && pattern.back() != '%';
    for (size_t i = 0; i <= pattern.size(); ++i) {
        if (i == pattern.size() || pattern[i] == '%') {
            if (i > start) {
                parts.emplace_back(pattern.data() + start, i - start);
            }
            start = i + 1;
        }
    }
    if (parts.empty()) {
        return true;
    }
    size_t pos = 0;
    for (size_t pi = 0; pi < parts.size(); ++pi) {
        const auto& part = parts[pi];
        if (pi == 0 && anchored_start) {
            if (!str.starts_with(part)) {
                return false;
            }
            pos = part.size();
        } else if (pi == parts.size() - 1 && anchored_end) {
            if (str.size() < pos + part.size()) {
                return false;
            }
            return str.compare(str.size() - part.size(), part.size(), part.data(), part.size()) == 0;
        } else {
            size_t found = str.find(part.data(), pos, part.size());
            if (found == std::string::npos) {
                return false;
            }
            pos = found + part.size();
        }
    }
    return true;
}

StrLike::StrLike(size_t id_a, std::string pattern) : id_a_(id_a), infix_(std::move(pattern)) {}

bool StrLike::Check(EngineBatch& batch, RowIndex i) {
    return LikeMatch(std::get<std::vector<std::string>>(batch.columns[id_a_])[i], infix_);
}

StrNotLike::StrNotLike(size_t id_a, std::string pattern) : inner_(id_a, std::move(pattern)) {}

bool StrNotLike::Check(EngineBatch& batch, RowIndex i) {
    return !inner_.Check(batch, i);
}

ColumnValue Strftime::Get(EngineBatch& batch, RowIndex i) {
    const std::string& s = std::get<std::vector<std::string>>(batch.columns[col_idx_])[i];
    // parse "YYYY-MM-DD HH:MM:SS" or "YYYY-MM-DD"
    struct tm t{};
    if (s.size() >= 19) {
        t.tm_year = std::stoi(s.substr(0, 4)) - 1900;
        t.tm_mon  = std::stoi(s.substr(5, 2)) - 1;
        t.tm_mday = std::stoi(s.substr(8, 2));
        t.tm_hour = std::stoi(s.substr(11, 2));
        t.tm_min  = std::stoi(s.substr(14, 2));
        t.tm_sec  = std::stoi(s.substr(17, 2));
    } else if (s.size() >= 10) {
        t.tm_year = std::stoi(s.substr(0, 4)) - 1900;
        t.tm_mon  = std::stoi(s.substr(5, 2)) - 1;
        t.tm_mday = std::stoi(s.substr(8, 2));
    }
    char buf[64];
    std::strftime(buf, sizeof(buf), fmt_.c_str(), &t);
    return std::string(buf);
}

}  // namespace column_engine
