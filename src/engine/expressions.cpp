#include "engine/expressions.h"
#include <ctime>

namespace column_engine::internal {

ColumnValue Strftime::Get(EngineBatch& batch, RowIndex i) {
    auto sv = GetStrAt(batch.columns[col_idx_], i);
    const char* s = sv.data();
    size_t slen = sv.size();
    auto parse2 = [](const char* p) { return (p[0] - '0') * 10 + (p[1] - '0'); };
    auto parse4 = [](const char* p) {
        return (p[0]-'0')*1000 + (p[1]-'0')*100 + (p[2]-'0')*10 + (p[3]-'0');
    };
    struct tm t {};
    if (slen >= 19) {
        t.tm_year = parse4(s) - 1900;
        t.tm_mon  = parse2(s + 5) - 1;
        t.tm_mday = parse2(s + 8);
        t.tm_hour = parse2(s + 11);
        t.tm_min  = parse2(s + 14);
        t.tm_sec  = parse2(s + 17);
    } else if (slen >= 10) {
        t.tm_year = parse4(s) - 1900;
        t.tm_mon  = parse2(s + 5) - 1;
        t.tm_mday = parse2(s + 8);
    }
    char buf[64];
    std::strftime(buf, sizeof(buf), fmt_.c_str(), &t);
    return std::string(buf);
}

}  // namespace column_engine::internal
