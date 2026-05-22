#include "types.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>
#include <glog/logging.h>

namespace column_engine {

std::shared_ptr<ColumnTypeName> GetType(const std::string& name) {
    if (name == "int64") {
        return std::make_shared<ColumnTypeInt64>();
    } else if (name == "string") {
        return std::make_shared<ColumnTypeString>();
    }
    LOG(INFO) << "Unknown type: " << name;
    return std::make_shared<ColumnTypeString>();
}

std::string ColumnTypeToString(ColumnValue data) {
    if (std::holds_alternative<int64_t>(data)) {
        return std::to_string(std::get<int64_t>(data));
    } else if (std::holds_alternative<std::string>(data)) {
        return std::get<std::string>(data);
    }
    throw std::runtime_error("Unknown type in ColumnTypeToString");
}

std::string ColumnTypeString::GetTypeName() {
    return "string";
}
ColumnValue ColumnTypeString::ConvertType(std::string val) {
    return val;
}

size_t ColumnTypeString::WriteType(std::vector<ColumnValue> data, FileWriter& writer) {
    std::map<std::string, int64_t> dict;
    std::vector<std::string> reverse_dict;

    uint32_t words_size = 0;

    for (auto& val : data) {
        if (dict.find(std::get<std::string>(val)) == dict.end()) {
            dict[std::get<std::string>(val)] = dict.size();
            reverse_dict.emplace_back(std::get<std::string>(val));
            words_size += std::get<std::string>(val).size();
        }
    }

    size_t result = 0;

    uint16_t n_words = static_cast<uint16_t>(data.size());
    uint16_t n_uwords = static_cast<uint16_t>(dict.size());

    // header: n_words(2) + n_uwords(2) + final_offset(4) + (n_uwords+1)*offsets(4 each)
    uint32_t final_offset = sizeof(n_words) + sizeof(n_uwords) + sizeof(uint32_t) +
                            (n_uwords + 1) * sizeof(uint32_t) + words_size;

    writer.Write(n_words);
    result += sizeof(n_words);
    writer.Write(n_uwords);
    result += sizeof(n_uwords);
    writer.Write(final_offset);
    result += sizeof(final_offset);

    uint32_t pos = 0;
    writer.Write(pos);
    result += sizeof(pos);
    for (const auto& str : reverse_dict) {
        pos += static_cast<uint32_t>(str.size());
        writer.Write(pos);
        result += sizeof(pos);
    }

    for (const auto& str : reverse_dict) {
        writer.Write(str.data(), str.size());
        result += str.size();
    }

    std::vector<ColumnValue> idx;
    idx.reserve(data.size());
    for (const auto& val : data) {
        idx.emplace_back(dict[std::get<std::string>(val)]);
    }

    ColumnTypeInt64 helper;
    result += helper.WriteType(idx, writer);

    return result;
}

ColumnData ColumnTypeString::GetBatch(size_t size, FileReader& reader) {
    uint16_t n_words = reader.Read<uint16_t>();
    uint16_t n_uwords = reader.Read<uint16_t>();
    uint32_t final_offset = reader.Read<uint32_t>();

    char* offsets_raw = reader.Peek((n_uwords + 1) * sizeof(uint32_t));

    size_t header_size = sizeof(n_words) + sizeof(n_uwords) + sizeof(final_offset) +
                         (n_uwords + 1) * sizeof(uint32_t);
    size_t raw_size = final_offset - header_size;
    char* raw = reader.Peek(raw_size);
    madvise(static_cast<void*>(raw), raw_size, MADV_WILLNEED);

    ColumnTypeInt64 helper;
    auto indices_data = helper.GetBatch(0, reader);
    const auto& indices = std::get<std::vector<int64_t>>(indices_data);

    std::vector<std::string_view> result;
    result.reserve(n_words);
    for (size_t i = 0; i < n_words; ++i) {
        auto pos = static_cast<size_t>(indices[i]);
        
        uint32_t current_offset;
        uint32_t next_offset;
        std::memcpy(&current_offset, offsets_raw + (pos * sizeof(uint32_t)), sizeof(uint32_t));
        std::memcpy(&next_offset, offsets_raw + ((pos + 1) * sizeof(uint32_t)), sizeof(uint32_t));

        result.emplace_back(raw + current_offset, next_offset - current_offset);
    }

    return result;
}

std::string ColumnTypeInt64::GetTypeName() {
    return "int64";
}

ColumnValue ColumnTypeInt64::ConvertType(std::string val) {
    try {
        return static_cast<int64_t>(stoll(val));
    } catch (...) {
        LOG(ERROR) << val << " is not a int64";
        return 0ll;
    }
}

size_t ColumnTypeInt64::WriteType(std::vector<ColumnValue> data, FileWriter& writer) {
    const size_t block_size = 512;
    const size_t rle_decision = 5;

    uint16_t blocks = static_cast<uint16_t>((data.size() + block_size - 1) / block_size);
    uint32_t n_values = static_cast<uint32_t>(data.size());

    size_t result = 0;

    std::vector<int64_t> min_val(blocks);
    std::vector<int64_t> max_val(blocks);
    std::vector<uint8_t> sz(blocks, 1);
    std::unordered_set<int64_t> unique;

    for (size_t i = 0; i < data.size(); ++i) {
        if (i % block_size == 0 || min_val[i / block_size] > std::get<int64_t>(data[i])) {
            min_val[i / block_size] = std::get<int64_t>(data[i]);
        }
        if (i % block_size == 0 || max_val[i / block_size] < std::get<int64_t>(data[i])) {
            max_val[i / block_size] = std::get<int64_t>(data[i]);
        }
        unique.insert(std::get<int64_t>(data[i]));
    }

    bool rle = (unique.size() * rle_decision < data.size());

    writer.Write<bool>(rle);   result += sizeof(bool);
    writer.Write(n_values);    result += sizeof(n_values);
    writer.Write(blocks);      result += sizeof(blocks);

    for (size_t i = 0; i < blocks; ++i) {
        uint64_t delta = static_cast<uint64_t>(max_val[i]) - static_cast<uint64_t>(min_val[i]);
        if (delta != 0) {
            sz[i] = static_cast<uint8_t>((64 - __builtin_clzll(delta) + 7) / 8);
        }
        uint32_t block_start = static_cast<uint32_t>(i * block_size);
        writer.Write(min_val[i]);   result += sizeof(int64_t);
        writer.Write(sz[i]);        result += sizeof(uint8_t);
        writer.Write(block_start);  result += sizeof(uint32_t);
    }

    if (rle) {
        uint64_t val_last = 0;
        uint8_t cnt = 0;
        for (size_t i = 0; i < data.size(); ++i) {
            uint64_t val_norm = static_cast<uint64_t>(std::get<int64_t>(data[i])) -
                                static_cast<uint64_t>(min_val[i / block_size]);
            if (cnt != 0 && (val_norm != val_last || i / block_size != (i - 1) / block_size || cnt == 255)) {
                writer.Write<uint8_t>(cnt);
                writer.Write(reinterpret_cast<const char*>(&val_last), sz[(i - 1) / block_size]);
                result += sizeof(uint8_t) + sz[(i - 1) / block_size];
                cnt = 0;
            }
            val_last = val_norm;
            ++cnt;
        }
        writer.Write<uint8_t>(cnt);
        writer.Write(reinterpret_cast<const char*>(&val_last), sz.back());
        result += sizeof(uint8_t) + sz.back();
    } else {
        for (size_t i = 0; i < data.size(); ++i) {
            uint64_t val_norm = static_cast<uint64_t>(std::get<int64_t>(data[i])) -
                                static_cast<uint64_t>(min_val[i / block_size]);
            writer.Write(reinterpret_cast<const char*>(&val_norm), sz[i / block_size]);
            result += sz[i / block_size];
        }
    }

    return result;
}

ColumnData ColumnTypeInt64::GetBatch(size_t, FileReader& reader) {
    std::vector<int64_t> result;
    std::vector<int64_t> min_val;
    std::vector<uint8_t> read_sz;
    std::vector<uint32_t> block_start;

    bool rle = reader.Read<bool>();
    uint32_t n_values = reader.Read<uint32_t>();
    uint16_t blocks = reader.Read<uint16_t>();

    for (size_t i = 0; i < blocks; ++i) {
        min_val.push_back(reader.Read<int64_t>());
        read_sz.push_back(reader.Read<uint8_t>());
        block_start.push_back(reader.Read<uint32_t>());
    }

    result.reserve(n_values);

    if (rle) {
        size_t count = 0;
        size_t block = 0;
        uint64_t val = 0;
        while (count < n_values) {
            if (block + 1 < blocks && count >= block_start[block + 1]) {
                ++block;
            }
            uint8_t n = reader.Read<uint8_t>();
            val = 0;
            reader.Read(reinterpret_cast<char*>(&val), read_sz[block]);
            int64_t res = static_cast<int64_t>(val + static_cast<uint64_t>(min_val[block]));
            for (uint8_t k = 0; k < n; ++k) {
                result.push_back(res);
            }
            count += n;
        }
    } else {
        for (size_t block = 0; block < blocks; ++block) {
            size_t n = (block + 1 < blocks ? block_start[block + 1] : n_values) - block_start[block];
            uint8_t sz = read_sz[block];
            int64_t base = min_val[block];
            const char* ptr = reader.Peek(n * sz);
            for (size_t i = 0; i < n; ++i) {
                uint64_t v = 0;
                memcpy(&v, ptr + i * sz, sz);
                result.push_back(base + static_cast<int64_t>(v));
            }
        }
    }

    return result;
}

};  // namespace column_engine
