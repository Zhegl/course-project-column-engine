#include <utility>
#include <types/types.h>
#include <types/types.h>
#include <io/file_reader.h>

namespace column_engine {
std::pair<std::vector<BatchMetaData>, Schema> GetMeta(const std::string& path);
};