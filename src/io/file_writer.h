#include <fstream>
#include <string>

namespace column_engine {

class FileWriter {
public:
    FileWriter(const std::string& path);

    void Write(const char* data, size_t size);

    template <typename T>
    void Write(const T& data) {
        Write(reinterpret_cast<const char*>(&data), sizeof(T));
    }

private:
    std::ofstream stream_;
};

};  // namespace column_engine