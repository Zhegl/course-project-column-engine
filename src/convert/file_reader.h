#pragma once
#include <fstream>
#include <string>

class FileReader {
public:
    FileReader(const std::string& path);

    bool Read(char* data, size_t size);

    bool Eof();

    void Jump(size_t offset);

    size_t Size();

    template <typename T>
    T Read() {
        T result;
        Read(reinterpret_cast<char*>(&result), sizeof(T));
        return result;
    }

private:
    std::ifstream stream_;
};