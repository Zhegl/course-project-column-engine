#pragma once
#include "engine/hashmap.h"
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace column_engine::internal {

class BloomFilter {
public:
    explicit BloomFilter(size_t words) : bits_(words) {}

    void Add(std::string_view word) {
        for (size_t i = 0; i < word.size(); ++i) {
            for (size_t j = 1; j <= 3 && i + j <= word.size(); ++j) {
                std::string_view ngram(word.data() + i, j);
                uint64_t h = StringViewHash{}(ngram) ^ (j * 0x9e3779b97f4a7c15ULL);
                h ^= h >> 33;
                uint64_t h2 = h * 0xff51afd7ed558ccdULL;
                h2 ^= h2 >> 33;
                Set(h);
                Set(h + h2);
                Set(h + 2 * h2);

            }
        }
    }

    bool CheckIn(const uint64_t* other, size_t words) const {
        for (size_t i = 0; i < words; ++i) {
            if ((other[i] & bits_[i]) != bits_[i]) { return false; }
        }
        return true;
    }

    const std::vector<uint64_t>& Bits() const { return bits_; }

private:
    void Set(uint64_t h) {
        size_t bit = h % (bits_.size() * 64);
        bits_[bit >> 6] |= (1ULL << (bit & 63));
    }

    std::vector<uint64_t> bits_;
};

}  // namespace column_engine::internal
