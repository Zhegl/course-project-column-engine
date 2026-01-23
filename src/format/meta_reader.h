#include <utility>
#include <types/types.h>
#include <interface/metadata.h>
#include <io/file_reader.h>

namespace column_engine {
std::pair<std::vector<BatchMetaData>, Scheme> GetMeta(const std::string& path);
};