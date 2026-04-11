#include "parser.h"
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace column_engine {

QueryParser::QueryParser(const Schema& schema) : schema_(schema) {
}

size_t QueryParser::GetColumnId(const std::string& name) {
    if (column_idx_.find(name) != column_idx_.end()) {
        return column_idx_[name].first;
    }
    for (size_t id = 0; id < schema_.columns.size(); ++id) {
        if (name == schema_.columns[id].name) {
            column_idx_[name] = {column_idx_.size(), id};
            return column_idx_[name].first;
        }
    }
    throw std::runtime_error("Column not found: " + name);
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

    std::string op;
    while (i < arg.size() && arg[i] != ' ') {
        op.push_back(arg[i++]);
    }
    ++i;

    std::string val;
    while (i < arg.size() && arg[i] != ' ') {
        val.push_back(arg[i++]);
    }

    if (schema_.columns[id].type->GetTypeName() == "int64") {
        if (op == "<>") {
            return std::make_shared<IntConstNE>(id, std::stoll(val));
        }
    }

    throw std::runtime_error("Unsupported WHERE: " + arg);
}

std::vector<std::shared_ptr<Aggregator>> QueryParser::ParseAggregate(const std::string& arg) {
    std::vector<std::shared_ptr<Aggregator>> aggs;
    size_t i = 0;

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
            aggs.push_back(std::make_shared<CountAll>());
            continue;
        }

        size_t id = GetColumnId(col);
        if (func == "SUM") {
            aggs.push_back(std::make_shared<IntSum>(id, col));
        } else if (func == "MIN") {
            aggs.push_back(std::make_shared<IntMin>(id, col));
        } else if (func == "MAX") {
            aggs.push_back(std::make_shared<IntMax>(id, col));
        } else if (func == "AVG") {
            aggs.push_back(std::make_shared<IntAvg>(id, col));
        } else {
            throw std::runtime_error("Unknown aggregate function: " + func);
        }
    }

    return aggs;
}

}  // namespace column_engine
