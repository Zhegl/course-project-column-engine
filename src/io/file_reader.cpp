#include "file_reader.h"
#include <cstddef>
#include <stdexcept>

namespace column_engine {

FileReader::FileReader(const std::string& path) : stream_(path, std::ios::binary) {
    if (!stream_) {
        throw std::runtime_error("Failed to open " + path);
    }
}

bool FileReader::Eof() {
    return stream_.peek() == std::istream::traits_type::eof();
}

bool FileReader::Read(char* data, size_t size) {
    stream_.read(data, size);
    if (stream_.eof()) {
        return false;
    }
    return true;
}

void FileReader::Jump(size_t offset) {
    stream_.seekg(offset, std::ios::cur);
}

size_t FileReader::Size() {
    std::streampos pos = stream_.tellg();
    stream_.seekg(0, std::ios::end);
    size_t result = static_cast<size_t>(stream_.tellg());
    stream_.seekg(pos);
    return result;
}

};  // namespace column_engine