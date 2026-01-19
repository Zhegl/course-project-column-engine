#include <string>
void ConvertToColumnar(const std::string& input_path, const std::string& scheme_path,
                       const std::string& output_path, const size_t batch_size = 1000000);

void ConvertToCsv(const std::string& input_path, const std::string& scheme_path,
                  const std::string& output_path);