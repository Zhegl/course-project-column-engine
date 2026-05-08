#pragma once
#include <cstdint>
#include <cstddef>
#include <string>

namespace column_engine {

class FileReader {
public:
    FileReader(const std::string& path);
    ~FileReader();

    FileReader(const FileReader&) = delete;
    FileReader& operator=(const FileReader&) = delete;

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
    char* base_ = nullptr;
    size_t size_ = 0;
    size_t pos_ = 0;
    int fd_ = -1;
};

};  // namespace column_engine
