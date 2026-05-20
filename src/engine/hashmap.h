#pragma once

#include <cstddef>
#include <cstring>
#include <functional>
#include <vector>
#include "types/types.h"

namespace column_engine::internal {

class Arena {
public:
    std::string_view Alloc(std::string_view data) {
        if (data.empty()) {
            return std::string_view("", 0);
        }   

        size_t size = data.size();
        size_t available = current_end_ - current_ptr_;
        if (available < size) {
            AllocateNewBlock(size);
        }

        char* dst = current_ptr_;
        std::memcpy(dst, data.data(), size);
        current_ptr_ += size;

        return std::string_view(dst, size);
    }

private:
    void AllocateNewBlock(size_t min_size) {
        size_t block_size = std::max(static_cast<size_t>(65536), min_size);
        blocks_.emplace_back(block_size);
        current_ptr_ = blocks_.back().data();
        current_end_ = current_ptr_ + block_size;
    }

    std::vector<std::vector<char>> blocks_;
    char* current_ptr_ = nullptr;
    char* current_end_ = nullptr;
};

template <typename T, typename Y>
struct Slot {
    T key;
    Y val;
};

template <typename T>
bool IsEmptyKey(const T&) { 
    return false;
}

inline bool IsEmptyKey(std::string_view v) { 
    return v.empty();
}

template <typename T, typename Y, typename Hash>
class HashMap {
public:
    HashMap() : map_(8), is_used_(8) {
    }

    template <typename LookupKey, typename CreatorF>
    Y& FindOrInsert(const LookupKey& lookup_key, CreatorF&& creator) {
        size_t hash = Hash{}(lookup_key) % map_.size();

        while (true) {
            if (!is_used_[hash]) {
                T permanent_key = creator(lookup_key);
                map_[hash].key = permanent_key;
                is_used_[hash] = true;
                size_++;

                if (size_ * 2 > map_.size()) {
                    Rebuild();
                    return operator[](permanent_key);
                }
                return map_[hash].val;
            }

            if (is_used_[hash] && 
                (IsEmptyKey(map_[hash].key) && IsEmptyKey(lookup_key) ? true : map_[hash].key == lookup_key)) 
            {
                return map_[hash].val;
            }
            ++hash;
            if (hash == map_.size()) {
                hash = 0;
            }
        }
    }
    Y& operator[](const T& key) {
        size_t hash = Hash{}(key) % map_.size();

        while (true) {
            if (!is_used_[hash]) {
                map_[hash].key = key;
                is_used_[hash] = true;
                size_++;

                if (size_ * 10 > map_.size() * 5) {
                    Rebuild();
                    return operator[](key);
                }
                return map_[hash].val;
            }

            if (is_used_[hash] && 
                (IsEmptyKey(map_[hash].key) && IsEmptyKey(key) ? true : map_[hash].key == key)) 
            {
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
        const std::vector<char>* is_used;
        std::vector<Slot<T, Y>>* map;
        size_t idx;

        Slot<T, Y>& operator*() {
            return (*map)[idx];
        }
        Slot<T, Y>* operator->() {
            return &(*map)[idx];
        }

        Iterator& operator++() {
            ++idx;
            while (idx < map->size() && !(*is_used)[idx]) {
                ++idx;
            }
            return *this;
        }

        bool operator==(const Iterator& o) const {
            return idx == o.idx;
        }
        bool operator!=(const Iterator& o) const {
            return idx != o.idx;
        }
    };

    size_t Capacity() const {
        return map_.size();
    }
    bool IsUsed(size_t idx) const {
        return is_used_[idx];
    }
    Slot<T, Y>& GetSlot(size_t idx) {
        return map_[idx];
    }

    size_t FirstUsed() const {
        size_t idx = 0;
        while (idx < map_.size() && !is_used_[idx]) {
            ++idx;
        }
        return idx;
    }

    size_t NextUsed(size_t idx) const {
        ++idx;
        while (idx < map_.size() && !is_used_[idx]) {
            ++idx;
        }
        return idx;
    }

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
        std::vector<char> is_used(map_.size() * 2, 0);
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

    std::vector<char> is_used_;
    std::vector<Slot<T, Y>> map_;
    size_t size_{0};
};

struct IntHash {
    size_t operator()(int64_t v) const {
        return std::hash<int64_t>{}(v);
    }
};

struct StrHash {
    size_t operator()(const std::string& v) const {
        return std::hash<std::string>{}(v);
    }
};

struct GroupKey {
    std::vector<ColumnValue> values;

    bool operator==(const GroupKey& other) const = default;
};

struct ColumnValueHash {
    size_t operator()(const ColumnValue& value) const {
        return std::visit([](const auto& v) { return std::hash<std::decay_t<decltype(v)>>{}(v); },
                          value);
    }
};

struct StringViewHash {
    size_t operator()(std::string_view v) const {
        return std::hash<std::string_view>{}(v);
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

}  // namespace column_engine::internal
// NOLINTEND(readability-identifier-naming)
