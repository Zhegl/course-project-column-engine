#include <fstream>
#include <string>

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