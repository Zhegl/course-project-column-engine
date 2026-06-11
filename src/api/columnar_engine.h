#pragma once
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace column_engine {

using QueryResult = std::vector<std::vector<std::string>>;

namespace internal {
class Pipeline;
class Engine;
}  // namespace internal

class ApiPipeline {
public:
    explicit ApiPipeline(std::unique_ptr<internal::Pipeline> impl);
    ApiPipeline(ApiPipeline&&) noexcept;
    ApiPipeline& operator=(ApiPipeline&&) noexcept;
    ~ApiPipeline();

    ApiPipeline Count(std::string arg);
    ApiPipeline Where(std::string arg);
    ApiPipeline Aggregate(std::string arg);
    ApiPipeline Limit(size_t arg);
    ApiPipeline Offset(size_t arg);
    ApiPipeline OrderBy(std::string arg);
    ApiPipeline Rename(std::string from, std::string to);
    ApiPipeline Add(std::string arg);
    ApiPipeline Case(std::string name, std::string when_cond, std::string then_expr, std::string else_expr);

    template <typename... Args>
    ApiPipeline GroupByAggregate(Args... args) {
        std::vector<std::string> all_args{std::string(args)...};
        std::string aggregates = all_args.back();
        all_args.pop_back();
        return GroupByAggregateImpl(std::move(all_args), std::move(aggregates));
    }

    template <typename... Args>
    ApiPipeline Select(Args... args) {
        return SelectImpl({std::string(args)...});
    }

    QueryResult Run();

private:
    ApiPipeline GroupByAggregateImpl(std::vector<std::string> group_columns, std::string aggregates);
    ApiPipeline SelectImpl(std::vector<std::string> columns);

    std::unique_ptr<internal::Pipeline> impl_;
};

class ColumnEngine {
public:
    explicit ColumnEngine(const std::string& path);
    ~ColumnEngine();
    ColumnEngine(ColumnEngine&&) noexcept;
    ColumnEngine& operator=(ColumnEngine&&) noexcept;

    ApiPipeline Api();

private:
    std::unique_ptr<internal::Engine> impl_;
};

using Engine = ColumnEngine;
using QueryResult = std::vector<std::vector<std::string>>;

}  // namespace column_engine
