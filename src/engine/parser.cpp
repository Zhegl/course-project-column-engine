#include "parser.h"
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace column_engine {

namespace {

std::string UnquoteStringLiteral(std::string value) {
    if (value.size() >= 2 && value.front() == '\'' && value.back() == '\'') {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

}  // namespace

QueryParser::QueryParser(const Schema& schema) : schema_(schema) {
}

size_t QueryParser::GetColumnId(const std::string& name, bool real) {
    if (column_idx_.find(name) != column_idx_.end()) {
        if (real) {
            return column_idx_[name].second;
        }
        return column_idx_[name].first;
    }
    for (size_t id = 0; id < schema_.columns.size(); ++id) {
        if (name == schema_.columns[id].name) {
            column_idx_[name] = {column_idx_.size(), id};
            if (real) {
                return column_idx_[name].second;
            }
            return column_idx_[name].first;
        }
    }
    throw std::runtime_error("Column not found: " + name);
}

bool QueryParser::EnsureColumnForSelect(const std::string& name) {
    if (column_idx_.find(name) != column_idx_.end()) {
        return true;
    }
    for (size_t id = 0; id < schema_.columns.size(); ++id) {
        if (name == schema_.columns[id].name) {
            column_idx_[name] = {column_idx_.size(), id};
            return true;
        }
    }
    return false;
}

std::vector<size_t> QueryParser::GetColumnsForScan() {
    std::vector<std::pair<size_t, size_t>> tmp;
    for (auto [key, val] : column_idx_) {
        tmp.emplace_back(val);
    }
    std::sort(tmp.begin(), tmp.end());
    std::vector<size_t> result;
    for (auto [_, val] : tmp) {
        result.emplace_back(val);
    }
    if (result.empty()) {
        result.emplace_back(0);
    }
    return result;
}

std::shared_ptr<FilterPredicate> QueryParser::ParseWhere(const std::string& arg) {
    size_t i = 0;

    std::string name;
    while (i < arg.size() && arg[i] != ' ') {
        name.push_back(arg[i++]);
    }
    ++i;

    size_t id = GetColumnId(name);
    size_t real_id = GetColumnId(name, true);

    std::string op;
    while (i < arg.size() && arg[i] != ' ') {
        op.push_back(arg[i++]);
    }
    ++i;

    std::string val;
    while (i < arg.size() && arg[i] != ' ') {
        val.push_back(arg[i++]);
    }

    if (schema_.columns[real_id].type->GetTypeName() == "int64") {
        if (op == "=") {
            return std::make_shared<IntConstEQ>(id, std::stoll(val));
        }
        if (op == "<>") {
            return std::make_shared<IntConstNE>(id, std::stoll(val));
        }
    } else if (schema_.columns[real_id].type->GetTypeName() == "string") {
        std::string str_val = UnquoteStringLiteral(val);
        if (op == "=") {
            return std::make_shared<StrConstEQ>(id, std::move(str_val));
        }
        if (op == "<>") {
            return std::make_shared<StrConstNE>(id, std::move(str_val));
        }
    }

    throw std::runtime_error("Unsupported WHERE: " + arg);
}

std::vector<AggFactory> QueryParser::ParseAggregate(const std::string& arg) {
    std::vector<AggFactory> factories;
    size_t i = 0;
    constexpr const char* kDistinctPrefix = "DISTINCT ";

    while (i < arg.size()) {
        while (i < arg.size() && (arg[i] == ' ' || arg[i] == ',')) {
            ++i;
        }
        if (i >= arg.size()) {
            break;
        }

        std::string func;
        while (i < arg.size() && arg[i] != '(') {
            func.push_back(arg[i++]);
        }
        ++i;
        std::string col;
        while (i < arg.size() && arg[i] != ')') {
            col.push_back(arg[i++]);
        }
        ++i;

        if (func == "COUNT" && col == "*") {
            factories.push_back([]() { return std::make_shared<CountAll>(); });
            continue;
        }

        bool is_distinct = col.starts_with(kDistinctPrefix);
        std::string raw_col = is_distinct ? col.substr(std::char_traits<char>::length(kDistinctPrefix))
                                          : col;

        size_t real_id = GetColumnId(raw_col, true);
        size_t id = GetColumnId(raw_col, false);
        bool is_str = schema_.columns[real_id].type->GetTypeName() == "string";
        if (func == "COUNT" && is_distinct) {
            if (is_str) {
                factories.push_back(
                    [id, raw_col]() { return std::make_shared<StrCountDistinct>(id, raw_col); });
            } else {
                factories.push_back(
                    [id, raw_col]() { return std::make_shared<IntCountDistinct>(id, raw_col); });
            }
        } else if (func == "SUM") {
            factories.push_back([id, raw_col]() { return std::make_shared<IntSum>(id, raw_col); });
        } else if (func == "MIN") {
            if (is_str) {
                factories.push_back(
                    [id, raw_col]() { return std::make_shared<StrMin>(id, raw_col); });
            } else {
                factories.push_back(
                    [id, raw_col]() { return std::make_shared<IntMin>(id, raw_col); });
            }
        } else if (func == "MAX") {
            if (is_str) {
                factories.push_back(
                    [id, raw_col]() { return std::make_shared<StrMax>(id, raw_col); });
            } else {
                factories.push_back(
                    [id, raw_col]() { return std::make_shared<IntMax>(id, raw_col); });
            }
        } else if (func == "AVG") {
            factories.push_back([id, raw_col]() { return std::make_shared<IntAvg>(id, raw_col); });
        } else {
            throw std::runtime_error("Unknown aggregate function: " + func);
        }
    }

    return factories;
}

}  // namespace column_engine
