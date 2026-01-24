#include <string>
#include <io/file_reader.h>
#include <types/types.h>

namespace column_engine {
Scheme ReadScheme(const std::string& path);
};