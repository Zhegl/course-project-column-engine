#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <types/types.h>

namespace column_engine {
struct EngineBatch {
    std::vector<std::string> names;
    std::vector<ColumnData> columns;
    std::vector<uint16_t> selection;
};
}  // namespace column_engine