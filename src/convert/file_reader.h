#include <fstream>
#include <string>

class FileReader {
public:
    FileReader(const std::string& path);

    void Read(char* data, size_t size);

    template <typename T>
    T Read() {
        T result;
        Read(reinterpret_cast<char*>(&result), sizeof(T));
        return result;
    }

private:
    std::ifstream stream_;
};