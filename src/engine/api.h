#pragma once
#include <memory>
#include <string>
#include <vector>
#include <engine/engine.h>
#include <engine/queries.h>
#include <engine/batch.h>


namespace column_engine {
    
using QueryResult = std::vector<std::vector<std::string>>;

class Operator;

class Engine;

class ApiPipeline {
public:
    explicit ApiPipeline(Engine& engine);
    ApiPipeline Count(std::string arg);
    QueryResult Run();

private:
    void Add(std::shared_ptr<Operator> op);
    Engine& engine_;
    std::shared_ptr<Operator> root_;
};
}  // namespace column_engine