#include "queries.h"
#include <variant>
#include <vector>
#include "types/types.h"

namespace column_engine {
void CountAll::Next(EngineBatch& batch, uint16_t i) {
    ++count_;
}

std::optional<EngineBatch> CountAll::GetResult() {
    EngineBatch result;
    result.names = {"Count"};
    result.selection = {0};
    result.columns.emplace_back(std::vector<int64_t>{static_cast<int64_t>(count_)});
    return result;
}
}