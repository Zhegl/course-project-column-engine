#include "schema_reader.h"
#include <cctype>
#include <stdexcept>
#include <string>
#include <types/types.h>

namespace column_engine {

bool IsSensible(char c) {
    return std::isgraph(c);
}

Schema ReadSchema(const std::string& path) {
    Schema result;
    FileReader reader(path);
    char symbol;

    int status = 0;
    std::string name;
    std::string current_str;

    while (reader.Read(&symbol, 1)) {
        if (symbol == ',') {
            if (status != 0) {
                throw std::runtime_error("syntax error in " + path);
            }
            name = current_str;
            current_str.clear();
            ++status;
        } else if (symbol == '\n') {
            if (status != 1 && !current_str.empty()) {
                throw std::runtime_error("syntax error in " + path);
            }
            if (status == 1) {
                result.columns.emplace_back(name, GetType(current_str));
            }
            current_str.clear();
            status = 0;
        } else if (IsSensible(symbol)) {
            current_str.push_back(symbol);
        }
    }

    if (status != 1 && !current_str.empty()) {
        throw std::runtime_error("syntax error in " + path);
    }
    if (status == 1) {
        result.columns.emplace_back(name, GetType(current_str));
    }

    return result;
}

};  // namespace column_engine