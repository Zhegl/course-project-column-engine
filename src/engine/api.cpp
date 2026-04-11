#include "api.h"
#include <memory>
#include <string>
#include "engine.h"
#include "queries.h"

namespace column_engine {

ApiPipeline::ApiPipeline(Engine& engine, const Schema& schema)
    : engine_(engine), parser_(schema) {
    scanner_ = engine.MakeScan();
    root_ = scanner_;
}

ApiPipeline ApiPipeline::Count(std::string arg) {
    if (arg == "*") {
        root_ = std::make_shared<class Aggregate>(root_, std::vector<std::shared_ptr<Aggregator>>{std::make_shared<CountAll>()});
        return *this;
    }
    throw std::runtime_error("Wrong arg for .Count: " + arg);
}

ApiPipeline ApiPipeline::Where(std::string arg) {
    root_ = std::make_shared<Filter>(root_, parser_.ParseWhere(arg));
    return *this;
}

ApiPipeline ApiPipeline::Aggregate(std::string arg) {
    root_ = std::make_shared<class Aggregate>(root_, parser_.ParseAggregate(arg));
    return *this;
}

void ApiPipeline::Add(std::shared_ptr<Operator> op) {
}

QueryResult ApiPipeline::Run() {
    scanner_->SetColumns(parser_.GetColumnsForScan());
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
