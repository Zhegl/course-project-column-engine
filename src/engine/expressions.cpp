#include "engine/expressions.h"
#include <ctime>

namespace column_engine::internal {

ColumnValue Strftime::Get(EngineBatch& batch, RowIndex i) {
    const std::string& s = std::get<std::vector<std::string>>(batch.columns[col_idx_])[i];
    struct tm t {};
    if (s.size() >= 19) {
        t.tm_year = std::stoi(s.substr(0, 4)) - 1900;
        t.tm_mon = std::stoi(s.substr(5, 2)) - 1;
        t.tm_mday = std::stoi(s.substr(8, 2));
        t.tm_hour = std::stoi(s.substr(11, 2));
        t.tm_min = std::stoi(s.substr(14, 2));
        t.tm_sec = std::stoi(s.substr(17, 2));
    } else if (s.size() >= 10) {
        t.tm_year = std::stoi(s.substr(0, 4)) - 1900;
        t.tm_mon = std::stoi(s.substr(5, 2)) - 1;
        t.tm_mday = std::stoi(s.substr(8, 2));
    }
    char buf[64];
    std::strftime(buf, sizeof(buf), fmt_.c_str(), &t);
    return std::string(buf);
}

}  // namespace column_engine::internal
