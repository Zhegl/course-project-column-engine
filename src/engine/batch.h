#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <types/types.h>

namespace column_engine::internal {
using RowIndex = uint32_t;

struct EngineBatch {
    std::vector<std::string> names;
    std::vector<ColumnData> columns;
    std::vector<RowIndex> selection;
};
}  // namespace column_engine::internal
