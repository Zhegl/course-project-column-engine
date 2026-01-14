#include "file_reader.h"
#include <stdexcept>

FileReader::FileReader(const std::string& path) : stream_(path, std::ios::binary) {
    if (!stream_) {
        throw std::runtime_error("Failed to open " + path);
    }
}

void FileReader::Read(char* data, size_t size) {
    stream_.read(data, size);
}
