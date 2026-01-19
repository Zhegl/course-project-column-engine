#include <utility>
#include <types/types.h>
#include <interface/metadata.h>
#include <io/file_reader.h>

std::pair<std::vector<BatchMetaData>, Scheme> GetMeta(const std::string& path);