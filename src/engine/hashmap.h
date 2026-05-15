#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>
#include "types/types.h"

namespace column_engine::internal {

template<typename T, typename Y>
struct Slot {
    T key;
    Y val;
};

template<typename T, typename Y, typename Hash>
class HashMap {
public:
    HashMap() : map_(8), is_used_(8) {
    }

    Y& operator[](const T& key) {
        size_t hash = Hash{}(key) % map_.size();

        while (true) {
            if (!is_used_[hash]) {
                map_[hash].key = key;
                is_used_[hash] = true;
                size_++;

                if (size_ * 10 > map_.size() * 7) {
                    Rebuild();
                    return operator[](key);
                }
                return map_[hash].val;
            }

            if (is_used_[hash] && map_[hash].key == key) {
                return map_[hash].val;
            }

            ++hash;
            if (hash == map_.size()) {
                hash = 0;
            }
        }

    }   

    size_t Size() const {
        return size_;
    }

    bool Empty() const {
        return size_ == 0;
    }

    struct Iterator {
        const std::vector<bool>* is_used;
        std::vector<Slot<T, Y>>* map;
        size_t idx;

        Slot<T, Y>& operator*() { return (*map)[idx]; }
        Slot<T, Y>* operator->() { return &(*map)[idx]; }

        Iterator& operator++() {
            ++idx;
            while (idx < map->size() && !(*is_used)[idx]) {
                ++idx;
            }
            return *this;
        }

        bool operator==(const Iterator& o) const { return idx == o.idx; }
        bool operator!=(const Iterator& o) const { return idx != o.idx; }
    };

    Iterator Begin() {
        size_t idx = 0;
        while (idx < map_.size() && !is_used_[idx]) {
            ++idx;
        }
        return {&is_used_, &map_, idx};
    }

    Iterator End() {
        return {&is_used_, &map_, map_.size()};
    }

private:
    void Rebuild() {
        std::vector<bool> is_used = std::vector<bool>(map_.size() * 2);
        std::vector<Slot<T, Y>> map = std::vector<Slot<T, Y>>(map_.size() * 2);

        for (size_t i = 0; i < map_.size(); ++i) {
            if (is_used_[i]) {
                size_t hash = Hash{}(map_[i].key) % map.size();
                while (is_used[hash]) {
                    ++hash;
                    if (hash == map.size()) {
                        hash = 0;
                    }
                }
                is_used[hash] = true;
                map[hash] = std::move(map_[i]);
            }
        }

        is_used_ = std::move(is_used);
        map_ = std::move(map);
    }

    std::vector<bool> is_used_;
    std::vector<Slot<T, Y>> map_;
    size_t size_{0};
};

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
using GroupHashMap = HashMap<GroupKey, T, GroupKeyHash>;

/*    


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
*/
}  // namespace column_engine::internal
// NOLINTEND(readability-identifier-naming)
