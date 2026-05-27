#pragma once
#include <memory>

namespace column_engine::internal {
class BloomFilter;
}

namespace column_engine {

struct ScanOptions {
    virtual ~ScanOptions() = default;
};

struct StringScanOptions : ScanOptions {
    bool skip_empty{false};
    std::shared_ptr<internal::BloomFilter> bloom;
};

}  // namespace column_engine
