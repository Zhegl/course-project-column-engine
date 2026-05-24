#pragma once
#include "hashmap.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace column_engine::internal {

class BloomFilter {
public:
    BloomFilter(size_t size) : bits_(size) {
    }

    void Set(uint64_t h) {
        size_t bit = h % (bits_.size() * 64);
        bits_[bit >> 6] |= (1ULL << (bit & 63));
    }
    bool Test(uint64_t h) const {
        size_t bit = h % (bits_.size() * 64);
        return bits_[bit >> 6] & (1ULL << (bit & 63));
    }
    void Add(std::string_view word) {
        std::string cur;
        for (size_t i = 0; i < word.size(); ++i) {
            cur.clear();
            for (size_t j = 0; j < 3 && i + j < word.size(); ++j) {
                cur.push_back(word[i + j]);
                uint64_t h = StringViewHash{}(cur) ^ ((j + 1) * 0x9e3779b97f4a7c15ULL);
                h ^= h >> 33;
                uint64_t h2 = h * 0xff51afd7ed558ccdULL;
                h2 ^= h2 >> 33;
                Set(h);
                Set(h + h2) ;
                Set(h + 2*h2);
            }
        }
    }

    bool CheckIn(const uint64_t* other, size_t words) const {
        for (size_t i = 0; i < words; ++i) {
            if ((other[i] & bits_[i]) != bits_[i]) return false;
        }
        return true;
    }

    std::vector<uint64_t> Get() {
        return bits_;
    } 


private:
    std::vector<uint64_t> bits_;
};

}