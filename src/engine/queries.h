#pragma once
#include <optional>
#include "batch.h"

namespace column_engine {

class Aggregator {
public:
    virtual void Next(EngineBatch& batch, uint16_t i) = 0;
    virtual std::optional<EngineBatch> GetResult() = 0;
    virtual ~Aggregator() = default;
};

class CountAll : public Aggregator {
public:
    void Next(EngineBatch& batch, uint16_t i);
    std::optional<EngineBatch> GetResult();
private:
    size_t count_{0};
};


class FilterPredicate {
public:
    virtual bool Check(EngineBatch& batch, uint16_t i) = 0;
    virtual ~FilterPredicate() = default;
};

class IntConstNE : public FilterPredicate {
public:
    IntConstNE(size_t id_a, int64_t const_b);
    bool Check(EngineBatch& batch, uint16_t i);

private:
    size_t id_a_, const_b_;
};

}  // namespace column_engine