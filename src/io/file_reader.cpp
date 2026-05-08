#include "file_reader.h"
#include <cstring>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace column_engine {

FileReader::FileReader(const std::string& path) {
    fd_ = open(path.c_str(), O_RDONLY);
    if (fd_ < 0) {
        throw std::runtime_error("Failed to open " + path);
    }
    struct stat st;
    if (fstat(fd_, &st) < 0) {
        close(fd_);
        throw std::runtime_error("Failed to stat " + path);
    }
    size_ = static_cast<size_t>(st.st_size);
    if (size_ > 0) {
        base_ = static_cast<char*>(mmap(nullptr, size_, PROT_READ, MAP_SHARED, fd_, 0));
        if (base_ == MAP_FAILED) {
            close(fd_);
            throw std::runtime_error("Failed to mmap " + path);
        }
    }
}

FileReader::~FileReader() {
    if (base_ && size_ > 0) {
        munmap(base_, size_);
    }
    if (fd_ >= 0) {
        close(fd_);
    }
}

bool FileReader::Read(char* data, size_t size) {
    if (pos_ + size > size_) {
        return false;
    }
    memcpy(data, base_ + pos_, size);
    pos_ += size;
    return true;
}

bool FileReader::Eof() {
    return pos_ >= size_;
}

void FileReader::Jump(int64_t offset) {
    pos_ = static_cast<size_t>(static_cast<int64_t>(pos_) + offset);
}

size_t FileReader::GetPos() {
    return pos_;
}

size_t FileReader::Size() {
    return size_;
}

};  // namespace column_engine
