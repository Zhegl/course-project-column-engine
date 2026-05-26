#pragma once

#include <cstddef>
#include <cstring>
#include <functional>
#include <vector>
#include "types/types.h"

namespace column_engine::internal {

class Arena {
public:
    Arena() = default;

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    Arena(Arena&& other) noexcept 
        : blocks_(std::move(other.blocks_)),
          current_ptr_(other.current_ptr_),
          current_end_(other.current_end_),
          next_block_size_(other.next_block_size_) {
        other.current_ptr_ = nullptr;
        other.current_end_ = nullptr;
    }

    ~Arena() {
        for (char* block : blocks_) {
            delete[] block;
        }
    }

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
        size_t block_size = std::max(next_block_size_, min_size);
        
        char* new_block = new char[block_size];
        blocks_.push_back(new_block);
        
        current_ptr_ = new_block;
        current_end_ = current_ptr_ + block_size;
        next_block_size_ = std::min(static_cast<size_t>(67108864), block_size * 2);
    }

    std::vector<char*> blocks_;
    char* current_ptr_ = nullptr;
    char* current_end_ = nullptr;
    size_t next_block_size_ = 4194304;
};

template <typename T, typename Y>
struct Slot {
    T key;
    Y val;
    size_t hash{0};
    bool occupied{false};
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
    HashMap() : map_(8) {
    }

    template <typename LookupKey, typename CreatorF>
    Y& FindOrInsert(const LookupKey& lookup_key, CreatorF&& creator) {
        size_t raw_hash = Hash{}(lookup_key);
        size_t hash = raw_hash & (map_.size() - 1);

        while (true) {
            if (!map_[hash].occupied) {
                T permanent_key = creator(lookup_key);
                map_[hash].key = permanent_key;
                map_[hash].hash = raw_hash;
                map_[hash].occupied = true;
                size_++;

                if (size_ * 2 > map_.size()) {
                    Rebuild();
                    return operator[](permanent_key);
                }
                return map_[hash].val;
            }

            if (map_[hash].key == lookup_key) {
                return map_[hash].val;
            }
            ++hash;
            if (hash == map_.size()) {
                hash = 0;
            }
        }
    }

    Y& operator[](const T& key) {
        size_t raw_hash = Hash{}(key);
        size_t hash = raw_hash & (map_.size() - 1);

        while (true) {
            if (!map_[hash].occupied) {
                map_[hash].key = key;
                map_[hash].hash = raw_hash;
                map_[hash].occupied = true;
                size_++;

                if (size_ * 2 > map_.size()) {
                    Rebuild();
                    return operator[](key);
                }
                return map_[hash].val;
            }

            if (map_[hash].key == key) {
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
            while (idx < map->size() && !(*map)[idx].occupied) {
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
        return map_[idx].occupied;
    }
    Slot<T, Y>& GetSlot(size_t idx) {
        return map_[idx];
    }

    size_t FirstUsed() const {
        size_t idx = 0;
        while (idx < map_.size() && !map_[idx].occupied) {
            ++idx;
        }
        return idx;
    }

    size_t NextUsed(size_t idx) const {
        ++idx;
        while (idx < map_.size() && !map_[idx].occupied) {
            ++idx;
        }
        return idx;
    }

    Iterator Begin() {
        size_t idx = 0;
        while (idx < map_.size() && !map_[idx].occupied) {
            ++idx;
        }
        return {&map_, idx};
    }

    Iterator End() {
        return {&map_, map_.size()};
    }

private:
    void Rebuild() {
        std::vector<Slot<T, Y>> map(map_.size() * 2);

        for (size_t i = 0; i < map_.size(); ++i) {
            if (map_[i].occupied) {
                size_t hash = map_[i].hash & (map.size() - 1);
                while (map[hash].occupied) {
                    ++hash;
                    if (hash == map.size()) {
                        hash = 0;
                    }
                }
                map[hash] = std::move(map_[i]);
            }
        }

        map_ = std::move(map);
    }

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


struct ColumnValueHash {
    size_t operator()(const ColumnValue& value) const {
        return std::visit([](const auto& v) { return std::hash<std::decay_t<decltype(v)>>{}(v); },
                          value);
    }
};

struct StringViewHash {
    size_t operator()(std::string_view v) const {
        static constexpr uint64_t kMul = 0x9e3779b97f4a7c15ULL;
        const char* data = v.data();
        size_t len = v.size();

        uint64_t h = len;
        size_t i = 0;
        for (; i + 8 <= len; i += 8) {
            uint64_t chunk;
            __builtin_memcpy(&chunk, data + i, 8);
            h ^= chunk;
            h *= kMul;
        }
        if (i < len) {
            uint64_t chunk = 0;
            __builtin_memcpy(&chunk, data + i, len - i);
            h ^= chunk;
            h *= kMul;
        }
        return h;
    }
};



}  // namespace column_engine::internal
// NOLINTEND(readability-identifier-naming)
