#include "file_reader.h"
#include <stdexcept>

FileReader::FileReader(const std::string& path) : stream_(path, std::ios::binary) {
    if (!stream_) {
        throw std::runtime_error("Failed to open " + path);
    }
}

bool FileReader::Read(char* data, size_t size) {
    if (stream_.eof()) {
        return false;
    }
    stream_.read(data, size);
    return true;
}
