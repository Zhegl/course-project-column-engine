#include "engine/predicates.h"
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

StrConstEQ::StrConstEQ(size_t id_a, std::string const_b)
    : id_a_(id_a), const_b_(std::move(const_b)) {
}

bool StrConstEQ::Check(EngineBatch& batch, RowIndex i) {
    return GetStrAt(batch.columns[id_a_], i) == const_b_;
}

static bool LikeMatch(const std::string_view str, const std::string& pattern) {
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
            return str.compare(str.size() - part.size(), part.size(), part.data(), part.size()) ==
                   0;
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

StrLike::StrLike(size_t id_a, std::string pattern) : id_a_(id_a), infix_(std::move(pattern)) {
}

bool StrLike::Check(EngineBatch& batch, RowIndex i) {
    return LikeMatch(GetStrAt(batch.columns[id_a_], i), infix_);
}

StrNotLike::StrNotLike(size_t id_a, std::string pattern) : inner_(id_a, std::move(pattern)) {
}

bool StrNotLike::Check(EngineBatch& batch, RowIndex i) {
    return !inner_.Check(batch, i);
}

}  // namespace column_engine::internal
