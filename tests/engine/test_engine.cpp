#include <gtest/gtest.h>
#include <convert/convert.h>
#include <engine/engine.h>
#include <io/file_writer.h>
#include "api.h"

namespace {

void Write(const std::string& path, const std::string& data) {
    column_engine::FileWriter writer(path);
    writer.Write(data.data(), data.size());
}

void ExpectResultMatches(const column_engine::QueryResult& result,
                         const std::vector<std::string>& expected_header,
                         const std::vector<std::vector<std::string>>& expected_rows) {
    ASSERT_EQ(result.size(), expected_rows.size() + 1);
    ASSERT_EQ(result[0], expected_header);
    for (size_t i = 0; i < expected_rows.size(); ++i) {
        EXPECT_EQ(result[i + 1], expected_rows[i]) << "Mismatch at row " << i + 1;
    }
}

// Schema:
//   id    int64   1..10
//   score int64   id * 10
//   name  string  "item_<id>"
//   group string  "A" if id <= 5, else "B"
class EngineTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        Write("schema.csv", "id,int64\nscore,int64\nname,string\ngroup,string\n");
        std::string csv;
        for (int i = 1; i <= 10; ++i) {
            csv += std::to_string(i) + ","
                 + std::to_string(i * 10) + ","
                 + "item_" + std::to_string(i) + ","
                 + (i <= 5 ? "A" : "B") + "\n";
        }
        Write("input.csv", csv);
        column_engine::ConvertToColumnar("input.csv", "schema.csv", "test.col", 4);
    }
};

// --- Scan / Select ---

TEST_F(EngineTest, SelectAllColumns) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().Select("id", "score", "name", "group").Run();
    ASSERT_EQ(result[0], (std::vector<std::string>{"id", "score", "name", "group"}));
    ASSERT_EQ(result.size(), 11u);
    EXPECT_EQ(result[1], (std::vector<std::string>{"1", "10", "item_1", "A"}));
    EXPECT_EQ(result[10], (std::vector<std::string>{"10", "100", "item_10", "B"}));
}

TEST_F(EngineTest, SelectSubsetColumns) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().Select("name", "id").Run();
    ASSERT_EQ(result[0], (std::vector<std::string>{"name", "id"}));
    ASSERT_EQ(result.size(), 11u);
    EXPECT_EQ(result[1], (std::vector<std::string>{"item_1", "1"}));
}

// --- Filter ---

TEST_F(EngineTest, WhereIntEq) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().Where("id = 3").Select("id", "score").Run();
    ExpectResultMatches(result, {"id", "score"}, {{"3", "30"}});
}

TEST_F(EngineTest, WhereIntNe) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().Where("id <> 3").Select("id").Run();
    ASSERT_EQ(result.size(), 10u);  // 9 rows
}

TEST_F(EngineTest, WhereStringEq) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().Where("group = 'A'").Select("id").Run();
    ASSERT_EQ(result.size(), 6u);  // 5 rows
    EXPECT_EQ(result[1][0], "1");
    EXPECT_EQ(result[5][0], "5");
}

TEST_F(EngineTest, WhereStringNe) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().Where("group <> 'A'").Select("id").Run();
    ASSERT_EQ(result.size(), 6u);  // 5 rows
    EXPECT_EQ(result[1][0], "6");
}

TEST_F(EngineTest, WhereNoResults) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().Where("id = 999").Select("id").Run();
    ASSERT_EQ(result.size(), 1u);  // header only
}

// --- Aggregate (no group by) ---

TEST_F(EngineTest, CountStar) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().Count("*").Run();
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[1][0], "10");
}

TEST_F(EngineTest, AggregateSum) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().Aggregate("SUM(score)").Select("SUM(score)").Run();
    // 10+20+...+100 = 550
    ExpectResultMatches(result, {"SUM(score)"}, {{"550"}});
}

TEST_F(EngineTest, AggregateMin) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().Aggregate("MIN(score)").Select("MIN(score)").Run();
    ExpectResultMatches(result, {"MIN(score)"}, {{"10"}});
}

TEST_F(EngineTest, AggregateMax) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().Aggregate("MAX(score)").Select("MAX(score)").Run();
    ExpectResultMatches(result, {"MAX(score)"}, {{"100"}});
}

TEST_F(EngineTest, AggregateCountDistinct) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().Aggregate("COUNT(DISTINCT group)").Select("COUNT(DISTINCT group)").Run();
    ExpectResultMatches(result, {"COUNT(DISTINCT group)"}, {{"2"}});
}

TEST_F(EngineTest, AggregateStringMin) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().Aggregate("MIN(name)").Select("MIN(name)").Run();
    // lexicographic min of item_1..item_10 is "item_1"
    ExpectResultMatches(result, {"MIN(name)"}, {{"item_1"}});
}

TEST_F(EngineTest, AggregateAfterFilter) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().Where("group = 'A'").Aggregate("SUM(score)").Select("SUM(score)").Run();
    // 10+20+30+40+50 = 150
    ExpectResultMatches(result, {"SUM(score)"}, {{"150"}});
}

// --- GroupByAggregate ---

TEST_F(EngineTest, GroupByAggregate) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api()
                      .GroupByAggregate("group", "SUM(score)")
                      .Select("group", "SUM(score)")
                      .Run();
    ASSERT_EQ(result[0], (std::vector<std::string>{"group", "SUM(score)"}));
    ASSERT_EQ(result.size(), 3u);
    std::map<std::string, std::string> sums;
    for (size_t i = 1; i < result.size(); ++i) {
        sums[result[i][0]] = result[i][1];
    }
    EXPECT_EQ(sums["A"], "150");  // 10+20+30+40+50
    EXPECT_EQ(sums["B"], "400");  // 60+70+80+90+100
}

TEST_F(EngineTest, GroupByCount) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api()
                      .GroupByAggregate("group", "COUNT(*)")
                      .Select("group", "COUNT(*)")
                      .Run();
    ASSERT_EQ(result.size(), 3u);
    std::map<std::string, std::string> counts;
    for (size_t i = 1; i < result.size(); ++i) {
        counts[result[i][0]] = result[i][1];
    }
    EXPECT_EQ(counts["A"], "5");
    EXPECT_EQ(counts["B"], "5");
}

// --- Rename ---

TEST_F(EngineTest, Rename) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().Rename("score", "pts").Select("id", "pts").Run();
    ASSERT_EQ(result[0], (std::vector<std::string>{"id", "pts"}));
    EXPECT_EQ(result[1][1], "10");
}

TEST_F(EngineTest, RenameAfterAggregate) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api()
                      .Aggregate("SUM(score)")
                      .Rename("SUM(score)", "total")
                      .Select("total")
                      .Run();
    ASSERT_EQ(result[0], (std::vector<std::string>{"total"}));
    EXPECT_EQ(result[1][0], "550");
}

// --- OrderBy + Limit (TopK) ---

TEST_F(EngineTest, TopKDesc) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().OrderBy("score DESC").Limit(3).Select("id", "score").Run();
    ASSERT_EQ(result.size(), 4u);
    EXPECT_EQ(result[1][1], "100");
    EXPECT_EQ(result[2][1], "90");
    EXPECT_EQ(result[3][1], "80");
}

TEST_F(EngineTest, TopKAsc) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().OrderBy("score ASC").Limit(3).Select("id", "score").Run();
    ASSERT_EQ(result.size(), 4u);
    EXPECT_EQ(result[1][1], "10");
    EXPECT_EQ(result[2][1], "20");
    EXPECT_EQ(result[3][1], "30");
}

TEST_F(EngineTest, TopKAfterFilter) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api()
                      .Where("group = 'B'")
                      .OrderBy("score DESC")
                      .Limit(2)
                      .Select("id", "score")
                      .Run();
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[1][0], "10");
    EXPECT_EQ(result[2][0], "9");
}

// --- Sort (OrderBy without Limit) ---

TEST_F(EngineTest, SortDesc) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().OrderBy("id DESC").Select("id").Run();
    ASSERT_EQ(result.size(), 11u);
    EXPECT_EQ(result[1][0], "10");
    EXPECT_EQ(result[10][0], "1");
}

TEST_F(EngineTest, SortAsc) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().OrderBy("id ASC").Select("id").Run();
    ASSERT_EQ(result.size(), 11u);
    EXPECT_EQ(result[1][0], "1");
    EXPECT_EQ(result[10][0], "10");
}

// --- Chains / combos ---

TEST_F(EngineTest, WhereAfterWhere) {
    column_engine::Engine engine("test.col");
    // id > 3 AND group = 'A' → id=4,5
    auto result = engine.Api()
                      .Where("id <> 1").Where("id <> 2").Where("id <> 3")
                      .Where("group = 'A'")
                      .Select("id")
                      .Run();
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[1][0], "4");
    EXPECT_EQ(result[2][0], "5");
}

TEST_F(EngineTest, GroupByAfterWhere) {
    column_engine::Engine engine("test.col");
    // only group B → one group, SUM = 60+70+80+90+100 = 400
    auto result = engine.Api()
                      .Where("group <> 'A'")
                      .GroupByAggregate("group", "COUNT(*)")
                      .Select("group", "COUNT(*)")
                      .Run();
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[1][0], "B");
    EXPECT_EQ(result[1][1], "5");
}

TEST_F(EngineTest, GroupByAfterGroupBy) {
    // GroupBy group → SUM(score), then GroupBy on result — same group col, COUNT(*)
    // After first GroupBy: schema = [group, SUM(score)]
    // Second GroupBy on group again → should give 1 row per group with COUNT=1
    column_engine::Engine engine("test.col");
    auto result = engine.Api()
                      .GroupByAggregate("group", "SUM(score)")
                      .GroupByAggregate("group", "COUNT(*)")
                      .Select("group", "COUNT(*)")
                      .Run();
    ASSERT_EQ(result.size(), 3u);
    std::map<std::string, std::string> counts;
    for (size_t i = 1; i < result.size(); ++i) {
        counts[result[i][0]] = result[i][1];
    }
    EXPECT_EQ(counts["A"], "1");
    EXPECT_EQ(counts["B"], "1");
}

TEST_F(EngineTest, TopKAfterGroupBy) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api()
                      .GroupByAggregate("group", "SUM(score)")
                      .Rename("SUM(score)", "total")
                      .OrderBy("total DESC")
                      .Limit(1)
                      .Select("group", "total")
                      .Run();
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[1][0], "B");
    EXPECT_EQ(result[1][1], "400");
}

TEST_F(EngineTest, SortAfterGroupBy) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api()
                      .GroupByAggregate("group", "SUM(score)")
                      .Rename("SUM(score)", "total")
                      .OrderBy("total ASC")
                      .Select("group", "total")
                      .Run();
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[1][0], "A");
    EXPECT_EQ(result[2][0], "B");
}

TEST_F(EngineTest, MultipleAggregates) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api()
                      .GroupByAggregate("group", "MIN(score), MAX(score), COUNT(*)")
                      .Select("group", "MIN(score)", "MAX(score)", "COUNT(*)")
                      .Run();
    ASSERT_EQ(result.size(), 3u);
    std::map<std::string, std::vector<std::string>> rows;
    for (size_t i = 1; i < result.size(); ++i) {
        rows[result[i][0]] = result[i];
    }
    EXPECT_EQ(rows["A"][1], "10");   // MIN
    EXPECT_EQ(rows["A"][2], "50");   // MAX
    EXPECT_EQ(rows["A"][3], "5");    // COUNT
    EXPECT_EQ(rows["B"][1], "60");
    EXPECT_EQ(rows["B"][2], "100");
    EXPECT_EQ(rows["B"][3], "5");
}

TEST_F(EngineTest, RenameBeforeWhere) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api()
                      .Rename("score", "pts")
                      .Where("pts = 30")
                      .Select("id", "pts")
                      .Run();
    ExpectResultMatches(result, {"id", "pts"}, {{"3", "30"}});
}

TEST_F(EngineTest, WhereNoResultsThenAggregate) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api()
                      .Where("id = 999")
                      .Aggregate("COUNT(*)")
                      .Select("COUNT(*)")
                      .Run();
    // COUNT(*) over empty set = 0 rows (no groups formed)
    ASSERT_EQ(result.size(), 1u);
}

TEST_F(EngineTest, LimitWithoutOrder) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().Limit(3).Select("id").Run();
    ASSERT_EQ(result.size(), 4u);
}

TEST_F(EngineTest, SelectSameColumnTwice) {
    column_engine::Engine engine("test.col");
    auto result = engine.Api().Select("id", "id").Run();
    ASSERT_EQ(result[0], (std::vector<std::string>{"id", "id"}));
    ASSERT_EQ(result.size(), 11u);
    EXPECT_EQ(result[1][0], result[1][1]);
}

}  // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;
    return RUN_ALL_TESTS();
}
