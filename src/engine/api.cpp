#include "api.h"
#include <memory>
#include <string>
#include "engine.h"
#include "queries.h"
#include "types/types.h"

namespace column_engine {

ApiPipeline::ApiPipeline(Engine& engine, Schema& schema) : engine_(engine), schema_(schema) {
    root_ = engine.MakeScan();
}

ApiPipeline ApiPipeline::Count(std::string arg) {
    if (arg == "*") {
        root_ = std::make_shared<Aggregate>(root_, std::make_shared<CountAll>());
        return *this;
    }
    throw std::runtime_error("Wrong arg for .Count: " + arg);
}

ApiPipeline ApiPipeline::Where(std::string arg) {
    std::string name;
    size_t i = 0;
    while (i < arg.size()) {
        if (arg[i] == ' ') {
            ++i;
            break;
        }
        name.push_back(arg[i]);
        ++i;
    }

    size_t id = GetColumnId(name);
    if (schema_.columns[id].type->GetTypeName() == "int64") {
        std::string op;
        while (i < arg.size()) {
            if (arg[i] == ' ') {
                ++i;
                break;
            }
            op.push_back(arg[i]);
            ++i;
        }
        if (op == "<>") {
            std::string c;
            while (i < arg.size()) {
                if (arg[i] == ' ') {
                    ++i;
                    break;
                }
                c.push_back(arg[i]);
                ++i;
            }
            root_ = std::make_shared<Filter>(root_, std::make_shared<IntConstNE>(id, stoll(c)));
            return *this;
        }
    }

    throw std::runtime_error("Wrong arg for .Where: " + arg);
}

void ApiPipeline::Add(std::shared_ptr<Operator> op) {
}

size_t ApiPipeline::GetColumnId(const std::string& name) {
    for (size_t id = 0; id < schema_.columns.size(); ++id) {
        if (name == schema_.columns[id].name) {
            return id;
        }
    }
    throw std::runtime_error("Column not found: " + name);
}

QueryResult ApiPipeline::Run() {
    EngineBatch batch = engine_.Run(root_);
    QueryResult result;
    result.push_back(batch.names);
    for (auto i : batch.selection) {
        result.emplace_back();
        for (size_t col = 0; col < result[0].size(); ++col) {
            std::visit([&](const auto& vec) {
                using T = std::decay_t<decltype(vec[0])>;
                if constexpr (std::is_same_v<T, int64_t>) {
                    result[result.size() - 1].emplace_back(std::to_string(vec[i]));
                } else {
                    result[result.size() - 1].emplace_back(vec[i]);
                }
            }, batch.columns[col]);
        }
    }
    return result;
}

}  // namespace column_engine