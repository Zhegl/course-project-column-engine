#include "parser.h"
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>
#include "types/types.h"

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

size_t QueryParser::GetColumnId(const std::string& name) {
    for (size_t id = 0; id < cur_schema_.columns.size(); ++id) {
        if (name == cur_schema_.columns[id].name) {
            return id;
        }
    }
    // not in cur_schema_ yet — look up in the source schema and register for scan
    for (size_t real_id = 0; real_id < schema_.columns.size(); ++real_id) {
        if (name == schema_.columns[real_id].name) {
            columns_for_scan_.push_back(real_id);
            cur_schema_.columns.push_back(schema_.columns[real_id]);
            return cur_schema_.columns.size() - 1;
        }
    }
    throw std::runtime_error("Column not found: " + name);
}

void QueryParser::SetSchema(Schema schema) {
    cur_schema_ = schema;
}

Schema QueryParser::GetSchema() {
    return cur_schema_;
}

std::vector<size_t> QueryParser::GetColumnsForScan() {
    if (columns_for_scan_.empty()) {
        return {0};
    }
    return columns_for_scan_;
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

    if (op == "NOT") {
        std::string next;
        while (i < arg.size() && arg[i] != ' ') {
            next.push_back(arg[i++]);
        }
        ++i;
        op = "NOT " + next;
    }

    std::string val;
    while (i < arg.size()) {
        val.push_back(arg[i++]);
    }

    const std::string& type_name = cur_schema_.columns[id].type->GetTypeName();
    if (type_name == "int64") {
        int64_t v = std::stoll(val);
        if (op == "=") {
            return std::make_shared<IntConstEQ>(id, v);
        }
        if (op == "<>") {
            return std::make_shared<IntConstNE>(id, v);
        }
        if (op == "<") {
            return std::make_shared<IntConstLT>(id, v);
        }
        if (op == "<=") {
            return std::make_shared<IntConstLE>(id, v);
        }
        if (op == ">") {
            return std::make_shared<IntConstGT>(id, v);
        }
        if (op == ">=") {
            return std::make_shared<IntConstGE>(id, v);
        }
    } else if (type_name == "string") {
        std::string str_val = UnquoteStringLiteral(val);
        if (op == "=") {
            return std::make_shared<StrConstEQ>(id, std::move(str_val));
        }
        if (op == "<>") {
            return std::make_shared<StrConstNE>(id, std::move(str_val));
        }
        if (op == "<") {
            return std::make_shared<StrConstLT>(id, std::move(str_val));
        }
        if (op == "<=") {
            return std::make_shared<StrConstLE>(id, std::move(str_val));
        }
        if (op == ">") {
            return std::make_shared<StrConstGT>(id, std::move(str_val));
        }
        if (op == ">=") {
            return std::make_shared<StrConstGE>(id, std::move(str_val));
        }
        if (op == "LIKE") {
            return std::make_shared<StrLike>(id, std::move(str_val));
        }
        if (op == "NOT LIKE") {
            return std::make_shared<StrNotLike>(id, std::move(str_val));
        }
    }

    throw std::runtime_error("Unsupported WHERE: " + arg);
}

std::pair<std::vector<AggFactory>, std::vector<ColumnMetaData>> QueryParser::ParseAggregate(
    const std::string& arg) {
    std::vector<AggFactory> factories;
    std::vector<ColumnMetaData> agg_columns;
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
            agg_columns.push_back({"COUNT(*)", GetType("int64")});
            continue;
        }

        bool is_distinct = col.starts_with(kDistinctPrefix);
        std::string raw_col =
            is_distinct ? col.substr(std::char_traits<char>::length(kDistinctPrefix)) : col;

        size_t id = GetColumnId(raw_col);
        bool is_str = cur_schema_.columns[id].type->GetTypeName() == "string";
        if (func == "COUNT" && is_distinct) {
            if (is_str) {
                factories.push_back(
                    [id, raw_col]() { return std::make_shared<StrCountDistinct>(id, raw_col); });
            } else {
                factories.push_back(
                    [id, raw_col]() { return std::make_shared<IntCountDistinct>(id, raw_col); });
            }
            agg_columns.push_back({"COUNT(DISTINCT " + raw_col + ")", GetType("int64")});
        } else if (func == "SUM") {
            factories.push_back([id, raw_col]() { return std::make_shared<IntSum>(id, raw_col); });
            agg_columns.push_back({"SUM(" + raw_col + ")", GetType("int64")});
        } else if (func == "MIN") {
            if (is_str) {
                factories.push_back(
                    [id, raw_col]() { return std::make_shared<StrMin>(id, raw_col); });
                agg_columns.push_back({"MIN(" + raw_col + ")", GetType("string")});
            } else {
                factories.push_back(
                    [id, raw_col]() { return std::make_shared<IntMin>(id, raw_col); });
                agg_columns.push_back({"MIN(" + raw_col + ")", GetType("int64")});
            }
        } else if (func == "MAX") {
            if (is_str) {
                factories.push_back(
                    [id, raw_col]() { return std::make_shared<StrMax>(id, raw_col); });
                agg_columns.push_back({"MAX(" + raw_col + ")", GetType("string")});
            } else {
                factories.push_back(
                    [id, raw_col]() { return std::make_shared<IntMax>(id, raw_col); });
                agg_columns.push_back({"MAX(" + raw_col + ")", GetType("int64")});
            }
        } else if (func == "AVG") {
            factories.push_back([id, raw_col]() { return std::make_shared<IntAvg>(id, raw_col); });
            agg_columns.push_back({"AVG(" + raw_col + ")", GetType("int64")});
        } else {
            throw std::runtime_error("Unknown aggregate function: " + func);
        }
    }

    return {std::move(factories), std::move(agg_columns)};
}

std::pair<std::shared_ptr<AddColFun>, bool> QueryParser::ParseAdd(const std::string& arg) {
    // integer literal: "-123" or "42"
    if (!arg.empty() && (std::isdigit(arg[0]) || (arg[0] == '-' && arg.size() > 1))) {
        bool all_digits = true;
        for (size_t i = (arg[0] == '-' ? 1 : 0); i < arg.size(); ++i) {
            if (!std::isdigit(arg[i])) { all_digits = false; break; }
        }
        if (all_digits) {
            return {std::make_shared<IntLiteral>(std::stoll(arg)), true};
        }
    }

    // length(col)
    if (arg.starts_with("length(") && arg.back() == ')') {
        std::string col = arg.substr(7, arg.size() - 8);
        size_t id = GetColumnId(col);
        return {std::make_shared<StrLen>(id, col), true};
    }

    // strftime('fmt', col)
    if (arg.starts_with("strftime(")) {
        size_t fmt_start = arg.find('\'') + 1;
        size_t fmt_end   = arg.find('\'', fmt_start);
        std::string fmt  = arg.substr(fmt_start, fmt_end - fmt_start);
        size_t col_start = arg.find(',', fmt_end) + 2;
        std::string col  = arg.substr(col_start, arg.size() - col_start - 1);
        size_t id = GetColumnId(col);
        return {std::make_shared<Strftime>(id, col, fmt), false};
    }

    // col + N  or  col - N
    auto plus_pos  = arg.rfind(" + ");
    auto minus_pos = arg.rfind(" - ");
    if (plus_pos != std::string::npos) {
        std::string col   = arg.substr(0, plus_pos);
        int64_t delta     = std::stoll(arg.substr(plus_pos + 3));
        size_t id = GetColumnId(col);
        return {std::make_shared<IntOffset>(id, col, delta), true};
    }
    if (minus_pos != std::string::npos) {
        std::string col   = arg.substr(0, minus_pos);
        int64_t delta     = -std::stoll(arg.substr(minus_pos + 3));
        size_t id = GetColumnId(col);
        return {std::make_shared<IntOffset>(id, col, delta), true};
    }

    throw std::runtime_error("Unsupported Add expression: " + arg);
}

}  // namespace column_engine
