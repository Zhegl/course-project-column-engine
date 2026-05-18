#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
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

    const char* Peek(size_t size) {
        const char* ptr = base_ + pos_;
        pos_ += size;
        return ptr;
    }

    int Fd() const { return fd_; }

    friend class FileReaderView;

private:
    char* base_ = nullptr;
    size_t size_ = 0;
    size_t pos_ = 0;
    int fd_ = -1;
};

class FileReaderView {
public:
    FileReaderView(const FileReader& reader, size_t pos = 0)
        : base_(reader.base_), size_(reader.size_), pos_(pos) {}

    bool Read(char* data, size_t size) {
        if (pos_ + size > size_) { return false; }
        memcpy(data, base_ + pos_, size);
        pos_ += size;
        return true;
    }

    template <typename T>
    T Read() {
        T result;
        Read(reinterpret_cast<char*>(&result), sizeof(T));
        return result;
    }

    const char* Peek(size_t size) {
        const char* ptr = base_ + pos_;
        pos_ += size;
        return ptr;
    }

    void Jump(int64_t offset) {
        pos_ = static_cast<size_t>(static_cast<int64_t>(pos_) + offset);
    }

    size_t GetPos() const { return pos_; }

private:
    const char* base_;
    size_t size_;
    size_t pos_ = 0;
};

};  // namespace column_engine
