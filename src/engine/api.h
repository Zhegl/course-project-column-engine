#pragma once
#include <memory>
#include <string>
#include <vector>
#include "types/types.h"
#include <engine/engine.h>
#include <engine/queries.h>
#include <engine/batch.h>
#include <engine/queries.h>

namespace column_engine {
    
using QueryResult = std::vector<std::vector<std::string>>;

class Operator;

class Engine;

class ApiPipeline {
public:
    explicit ApiPipeline(Engine& engine, Schema& schema);
    ApiPipeline Count(std::string arg);
    ApiPipeline Where(std::string arg);
    QueryResult Run();

private:
    size_t GetColumnId(const std::string& name);
    void Add(std::shared_ptr<Operator> op);
    Engine& engine_;
    Schema& schema_;
    std::shared_ptr<Operator> root_;
};
}  // namespace column_engine