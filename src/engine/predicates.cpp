#include "engine/predicates.h"
#include <cstdint>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string_view>

namespace column_engine::internal {

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
    return GetStrAt(batch.columns[id_a_], i) != const_b_;
}

std::vector<RowIndex> StrConstNE::CheckBatch(EngineBatch& batch, const std::vector<RowIndex>& selection) {
    std::vector<RowIndex> result;
    result.reserve(selection.size());
    const auto& col = batch.columns[id_a_];
    if (std::holds_alternative<std::vector<std::string_view>>(col)) {
        const auto& sv_col = std::get<std::vector<std::string_view>>(col);
        for (auto i : selection) {
            if (sv_col[i] != const_b_) {
                result.push_back(i);
            }
        }
    } else {
        const auto& s_col = std::get<std::vector<std::string>>(col);
        for (auto i : selection) {
            if (s_col[i] != const_b_) {
                result.push_back(i);
            }
        }
    }
    return result;
}

StrConstEQ::StrConstEQ(size_t id_a, std::string const_b)
    : id_a_(id_a), const_b_(std::move(const_b)) {
}

bool StrConstEQ::Check(EngineBatch& batch, RowIndex i) {
    return GetStrAt(batch.columns[id_a_], i) == const_b_;
}

std::vector<RowIndex> StrConstEQ::CheckBatch(EngineBatch& batch, const std::vector<RowIndex>& selection) {
    std::vector<RowIndex> result;
    result.reserve(selection.size());
    const auto& col = batch.columns[id_a_];
    if (std::holds_alternative<std::vector<std::string_view>>(col)) {
        const auto& sv_col = std::get<std::vector<std::string_view>>(col);
        for (auto i : selection) {
            if (sv_col[i] == const_b_) {
                result.push_back(i);
            }
        }
    } else {
        const auto& s_col = std::get<std::vector<std::string>>(col);
        for (auto i : selection) {
            if (s_col[i] == const_b_) {
                result.push_back(i);
            }
        }
    }
    return result;
}


StrLike::StrLike(size_t id_a, std::string pattern) : id_a_(id_a), infix_(std::move(pattern)) {
    if (infix_.front() != '%' || infix_.back() != '%') {
        throw std::runtime_error("Use RegExp");
    }
}

bool StrLike::Check(EngineBatch& batch, RowIndex i) {
    const auto& str = GetStrAt(batch.columns[id_a_], i);
    const char* pat = infix_.data() + 1;
    size_t m = infix_.size() - 2;
    if (str.size() < m) {
        return false;
    }
    const char* s = str.data();
    const char* end = s + str.size() - m;
    while ((s = static_cast<const char*>(std::memchr(s, pat[0], end - s + 1)))) {
        if (std::memcmp(s, pat, m) == 0) {
            return true;
        }
        ++s;
    }
    return false;
}

StrNotLike::StrNotLike(size_t id_a, std::string pattern) : inner_(id_a, std::move(pattern)) {
}

bool StrNotLike::Check(EngineBatch& batch, RowIndex i) {
    return !inner_.Check(batch, i);
}

}  // namespace column_engine::internal
