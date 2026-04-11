#include "queries.h"
#include <cstdint>
#include <variant>
#include <vector>
#include "types/types.h"

namespace column_engine {
void CountAll::Next(EngineBatch& batch, uint16_t i) {
    ++count_;
}

std::optional<EngineBatch> CountAll::GetResult() {
    EngineBatch result;
    result.names = {"Count"};
    result.selection = {0};
    result.columns.emplace_back(std::vector<int64_t>{static_cast<int64_t>(count_)});
    return result;
}

IntConstNE::IntConstNE(size_t id_a, int64_t const_b) : id_a_(id_a), const_b_(const_b) {
}

bool IntConstNE::Check(EngineBatch& batch, uint16_t i) {
    return std::get<std::vector<int64_t>>(batch.columns[id_a_])[i] != const_b_;
}

}  // namespace column_engine