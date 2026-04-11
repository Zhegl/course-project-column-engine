#pragma once
#include <memory>
#include <string>
#include <vector>
#include "types/types.h"
#include <engine/engine.h>
#include <engine/queries.h>
#include <engine/batch.h>
#include <engine/parser.h>

namespace column_engine {

using QueryResult = std::vector<std::vector<std::string>>;

class Operator;

class Engine;

class Scan;

class ApiPipeline {
public:
    explicit ApiPipeline(Engine& engine, const Schema& schema);
    ApiPipeline Count(std::string arg);
    ApiPipeline Where(std::string arg);
    ApiPipeline Aggregate(std::string arg);
    QueryResult Run();

private:
    void Add(std::shared_ptr<Operator> op);
    Engine& engine_;
    QueryParser parser_;
    std::shared_ptr<Operator> root_;
    std::shared_ptr<Scan> scanner_;
};
}  // namespace column_engine