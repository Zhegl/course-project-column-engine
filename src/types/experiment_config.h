#pragma once
#include <cstddef>

namespace column_engine {

struct ExperimentConfig {
    bool use_bloom = true;
    size_t rle_threshold = 5;  // RLE if unique*threshold < total

    static ExperimentConfig& Get() {
        static ExperimentConfig instance;
        return instance;
    }

private:
    ExperimentConfig() = default;
};

}  // namespace column_engine
