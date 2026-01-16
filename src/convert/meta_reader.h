#include <utility>
#include "types.h"
#include "metadata.h"
#include "file_reader.h"

std::pair<std::vector<BatchMetaData>, Scheme> GetMeta(const std::string& path);