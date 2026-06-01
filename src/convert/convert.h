#pragma once
#include <string>

namespace column_engine {
void ConvertToColumnar(const std::string& input_path, const std::string& schema_path,
                       const std::string& output_path, size_t batch_size = 1000000,
                       bool use_lzw = false);

void ConvertToCsv(const std::string& input_path, const std::string& schema_path,
                  const std::string& output_path);
}  // namespace column_engine
