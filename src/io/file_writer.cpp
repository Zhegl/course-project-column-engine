#include "file_writer.h"
#include <stdexcept>

namespace column_engine {

FileWriter::FileWriter(const std::string& path) : stream_(path, std::ios::binary) {
    if (!stream_) {
        throw std::runtime_error("Failed to open " + path);
    }
}

void FileWriter::Write(const char* data, size_t size) {
    stream_.write(data, size);
}

};