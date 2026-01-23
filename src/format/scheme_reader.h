#include <string>
#include <io/file_reader.h>
#include <interface/metadata.h>

namespace column_engine {
Scheme ReadScheme(const std::string& path);
};