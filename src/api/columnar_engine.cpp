#include "api/columnar_engine.h"
#include "engine/api.h"
#include "engine/engine.h"

namespace column_engine {

// ApiPipeline

ApiPipeline::ApiPipeline(std::unique_ptr<internal::ApiPipeline> impl)
    : impl_(std::move(impl)) {}

ApiPipeline::ApiPipeline(ApiPipeline&&) noexcept = default;
ApiPipeline& ApiPipeline::operator=(ApiPipeline&&) noexcept = default;
ApiPipeline::~ApiPipeline() = default;

ApiPipeline ApiPipeline::Count(std::string arg) {
    return ApiPipeline(std::make_unique<internal::ApiPipeline>(impl_->Count(std::move(arg))));
}

ApiPipeline ApiPipeline::Where(std::string arg) {
    return ApiPipeline(std::make_unique<internal::ApiPipeline>(impl_->Where(std::move(arg))));
}

ApiPipeline ApiPipeline::Aggregate(std::string arg) {
    return ApiPipeline(std::make_unique<internal::ApiPipeline>(impl_->Aggregate(std::move(arg))));
}

ApiPipeline ApiPipeline::Limit(size_t arg) {
    return ApiPipeline(std::make_unique<internal::ApiPipeline>(impl_->Limit(arg)));
}

ApiPipeline ApiPipeline::Offset(size_t arg) {
    return ApiPipeline(std::make_unique<internal::ApiPipeline>(impl_->Offset(arg)));
}

ApiPipeline ApiPipeline::OrderBy(std::string arg) {
    return ApiPipeline(std::make_unique<internal::ApiPipeline>(impl_->OrderBy(std::move(arg))));
}

ApiPipeline ApiPipeline::Rename(std::string from, std::string to) {
    return ApiPipeline(std::make_unique<internal::ApiPipeline>(impl_->Rename(std::move(from), std::move(to))));
}

ApiPipeline ApiPipeline::Add(std::string arg) {
    return ApiPipeline(std::make_unique<internal::ApiPipeline>(impl_->Add(std::move(arg))));
}

ApiPipeline ApiPipeline::Case(std::string name, std::string when_cond, std::string then_expr, std::string else_expr) {
    return ApiPipeline(std::make_unique<internal::ApiPipeline>(
        impl_->Case(std::move(name), std::move(when_cond), std::move(then_expr), std::move(else_expr))));
}

ApiPipeline ApiPipeline::GroupByAggregateImpl(std::vector<std::string> group_columns, std::string aggregates) {
    std::vector<std::string> all_args = std::move(group_columns);
    all_args.push_back(std::move(aggregates));
    return ApiPipeline(std::make_unique<internal::ApiPipeline>(impl_->GroupByAggregateVec(std::move(all_args))));
}

ApiPipeline ApiPipeline::SelectImpl(std::vector<std::string> columns) {
    return ApiPipeline(std::make_unique<internal::ApiPipeline>(impl_->SelectVec(columns)));
}

QueryResult ApiPipeline::Run() {
    return impl_->Run();
}

// ColumnEngine

ColumnEngine::ColumnEngine(const std::string& path)
    : impl_(std::make_unique<internal::Engine>(path)) {}

ColumnEngine::~ColumnEngine() = default;
ColumnEngine::ColumnEngine(ColumnEngine&&) noexcept = default;
ColumnEngine& ColumnEngine::operator=(ColumnEngine&&) noexcept = default;

ApiPipeline ColumnEngine::Api() {
    return ApiPipeline(std::make_unique<internal::ApiPipeline>(impl_->Api()));
}

}  // namespace column_engine
