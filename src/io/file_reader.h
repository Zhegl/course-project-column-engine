#pragma once
#include <cstdint>
#include <fstream>
#include <string>

namespace column_engine {

class FileReader {
public:
    FileReader(const std::string& path);

    bool Read(char* data, size_t size);

    bool Eof();

    void Jump(int64_t offset);

    size_t Size();

    size_t GetPos();

    template <typename T>
    T Read() {
        T result;
        Read(reinterpret_cast<char*>(&result), sizeof(T));
        return result;
    }

private:
    std::ifstream stream_;
};

};  // namespace column_engine