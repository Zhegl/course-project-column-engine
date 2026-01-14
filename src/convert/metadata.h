#include <string>
#include <vector>

enum class ColumnType { String = 0, Int64 = 1 };

struct ColumnMetaData {
    std::string name;
    ColumnType type;
};

struct Scheme {
    std::vector<ColumnMetaData> columns;
};