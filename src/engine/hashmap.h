#pragma once
// NOLINTBEGIN(readability-identifier-naming)

#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>
#include "types/types.h"

namespace column_engine::internal {

struct GroupKey {
    std::vector<ColumnValue> values;

    bool operator==(const GroupKey& other) const = default;
};

struct ColumnValueHash {
    size_t operator()(const ColumnValue& value) const {
        return std::visit([](const auto& v) { return std::hash<std::decay_t<decltype(v)>>{}(v); }, value);
    }
};

struct GroupKeyHash {
    size_t operator()(const GroupKey& key) const {
        size_t hash = 0;
        ColumnValueHash value_hash;
        for (const auto& value : key.values) {
            hash ^= value_hash(value) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

template <typename T>
class HashMap {
public:

    using MapType = std::unordered_map<GroupKey, T, GroupKeyHash>;
    using iterator = typename MapType::iterator;
    using const_iterator = typename MapType::const_iterator;

    T& operator[](const GroupKey& key) {
        return data_[key];
    }

    iterator begin() {
        return data_.begin();
    }

    iterator end() {
        return data_.end();
    }

    const_iterator begin() const {
        return data_.begin();
    }

    const_iterator end() const {
        return data_.end();
    }

    bool empty() const {
        return data_.empty();
    }

    size_t size() const {
        return data_.size();
    }

private:
    MapType data_;
};

}  // namespace column_engine::internal
// NOLINTEND(readability-identifier-naming)
