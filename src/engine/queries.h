#pragma once
#include <optional>
#include "batch.h"
#include "engine.h"

namespace column_engine {
class CountAll {
public:
    void Next(EngineBatch& batch, uint16_t i);
    std::optional<EngineBatch> GetResult();
private:
    size_t count_{0};
};

}  // namespace column_engine