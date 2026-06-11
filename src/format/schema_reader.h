#include <string>
#include <io/file_reader.h>
#include <types/types.h>

namespace column_engine {
Schema ReadSchema(const std::string& path);
};