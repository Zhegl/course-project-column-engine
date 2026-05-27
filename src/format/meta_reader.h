#pragma once
#include <utility>
#include <types/types.h>

namespace column_engine {
std::pair<std::vector<BatchMetaData>, Schema> GetMeta(const std::string& path);
}  // namespace column_engine
