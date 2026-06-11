#pragma once
#include <stdexcept>
#include <string>
#include "engine/batch.h"

namespace column_engine::internal {

inline bool LessColumnValue(const ColumnValue& lhs, const ColumnValue& rhs) {
    return std::visit(
        [](const auto& left, const auto& right) -> bool {
            using L = std::decay_t<decltype(left)>;
            using R = std::decay_t<decltype(right)>;
            if constexpr (std::is_same_v<L, R>) {
                return left < right;
            }
            throw std::runtime_error("Column type mismatch in comparison");
        },
        lhs, rhs);
}

inline ColumnValue GetColumnValue(const ColumnData& column, RowIndex i) {
    return std::visit([&](const auto& data) -> ColumnValue {
        using T = typename std::decay_t<decltype(data)>::value_type;
        if constexpr (std::is_same_v<T, std::string_view>) {
            return std::string(data[i]);
        } else {
            return data[i];
        }
    }, column);
}

inline ColumnData MakeColumnData(const ColumnValue& value) {
    return std::visit(
        [](const auto& v) -> ColumnData {
            using T = std::decay_t<decltype(v)>;
            return std::vector<T>{};
        },
        value);
}

inline ColumnData MakeEmptyColumnLike(const ColumnData& column) {
    return std::visit(
        [](const auto& v) -> ColumnData {
            using T = typename std::decay_t<decltype(v)>::value_type;
            return std::vector<T>{};
        },
        column);
}

inline void AppendColumnValue(ColumnData& column, const ColumnValue& value) {
    std::visit(
        [&](auto& data) {
            using T = typename std::decay_t<decltype(data)>::value_type;
            if constexpr (std::is_same_v<T, std::string_view>) {
                data.push_back(std::string_view(std::get<std::string>(value)));
            } else {
                data.push_back(std::get<T>(value));
            }
        },
        column);
}

inline size_t GetColumnSize(const ColumnData& column) {
    return std::visit([](const auto& data) { return data.size(); }, column);
}

inline void ClearColumn(ColumnData& column) {
    std::visit([](auto& data) { data.clear(); }, column);
}

inline void AppendSelectedValues(ColumnData& dst, const ColumnData& src,
                                 const std::vector<RowIndex>& selection, size_t limit = 0) {
    std::visit(
        [&](auto& out, const auto& in) {
            if constexpr (std::is_same_v<std::decay_t<decltype(out)>,
                                         std::decay_t<decltype(in)>>) {
                for (auto i : selection) {
                    if (limit != 0 && out.size() >= limit) {
                        break;
                    }
                    out.push_back(in[i]);
                }
            }
        },
        dst, src);
}

inline bool LessAt(const ColumnData& column, RowIndex lhs, RowIndex rhs) {
    return std::visit([&](const auto& values) { return values[lhs] < values[rhs]; }, column);
}

inline std::string ColumnValueToStringAt(const ColumnData& column, RowIndex i) {
    return std::visit(
        [&](const auto& values) -> std::string {
            using T = typename std::decay_t<decltype(values)>::value_type;
            if constexpr (std::is_same_v<T, int64_t>) {
                return std::to_string(values[i]);
            } else {
                return std::string(values[i]);
            }
        },
        column);
}

}  // namespace column_engine::internal
