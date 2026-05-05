#include <gtest/gtest.h>
#include <convert/convert.h>
#include <engine/engine.h>
#include <glog/logging.h>
#include "api.h"

class ClickBench : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        column_engine::ConvertToColumnar("hits_sample.csv", "hits_schema.csv", "col.col", 10000);
    }
};


TEST_F(ClickBench, Q0) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api().Aggregate("COUNT(*)").Select("COUNT(*)").Run();
    std::cout << result[1][0] << "\n";
    EXPECT_EQ(result[1][0], "999977");
}

TEST_F(ClickBench, Q1) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api().Where("AdvEngineID <> 0").Aggregate("COUNT(*)").Select("COUNT(*)").Run();
    std::cout << result[1][0] << "\n";
    EXPECT_EQ(result[1][0], "14174");
}

TEST_F(ClickBench, Q2) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Aggregate("SUM(AdvEngineID), COUNT(*), AVG(ResolutionWidth)")
                      .Select("SUM(AdvEngineID)", "COUNT(*)", "AVG(ResolutionWidth)")
                      .Run();
    std::cout << result[1][0] << " " << result[1][1] << " " << result[1][2] << "\n";
    EXPECT_EQ(result[1][0], "80778");
    EXPECT_EQ(result[1][1], "999977");
    EXPECT_EQ(result[1][2], "1604");
}

TEST_F(ClickBench, Q3) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api().Aggregate("AVG(UserID)").Select("AVG(UserID)").Run();
    std::cout << result[1][0] << "\n";
    EXPECT_EQ(result[1][0], "15567353102073");
}

TEST_F(ClickBench, Q4) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .GroupBy("UserID")
                      .Aggregate("COUNT(*)")
                      .Select("COUNT(*)")
                      .Run();
    std::cout << result[1][0] << "\n";
    ASSERT_EQ(result[0].size(), 1);
    EXPECT_EQ(result[0][0], "COUNT(*)");
    EXPECT_EQ(result[1][0], "79842");
}

TEST_F(ClickBench, Q5) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .GroupBy("SearchPhrase")
                      .Aggregate("COUNT(*)")
                      .Select("COUNT(*)")
                      .Run();
    std::cout << result[1][0] << "\n";
    ASSERT_EQ(result[0].size(), 1);
    EXPECT_EQ(result[0][0], "COUNT(*)");
    EXPECT_EQ(result[1][0], "18316");
}

TEST_F(ClickBench, Q6) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Aggregate("MIN(EventDate), MAX(EventDate)")
                      .Select("MIN(EventDate)", "MAX(EventDate)")
                      .Run();
    std::cout << result[1][0] << "\t" << result[1][1] << "\n";
    EXPECT_EQ(result[1][0], "2013-07-15");
    EXPECT_EQ(result[1][1], "2013-07-15");
}


TEST_F(ClickBench, Q7) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("AdvEngineID <> 0")
                      .GroupByAggregate("AdvEngineID", "COUNT(*)")
                      .OrderBy("COUNT(*) DESC")
                      .Select("AdvEngineID", "COUNT(*)")
                      .Run();
    
    ASSERT_EQ(result.size(), 6);
    ASSERT_EQ(result[0].size(), 2);

    EXPECT_EQ(result[0][0], "AdvEngineID");
    EXPECT_EQ(result[0][1], "COUNT(*)");

    EXPECT_EQ(result[1][0], "2");
    EXPECT_EQ(result[1][1], "9543");

    EXPECT_EQ(result[2][0], "13");
    EXPECT_EQ(result[2][1], "4592");

    EXPECT_EQ(result[3][0], "52");
    EXPECT_EQ(result[3][1], "34");

    EXPECT_EQ(result[4][0], "50");
    EXPECT_EQ(result[4][1], "4");

    EXPECT_EQ(result[5][0], "28");
    EXPECT_EQ(result[5][1], "1");


}

TEST_F(ClickBench, Q8) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .GroupBy("RegionID", "UserID")
                      .GroupByAggregate("RegionID", "COUNT(*)")
                      .Rename("COUNT(*)", "u")
                      .OrderBy("u DESC")
                      .Limit(10)
                      .Select("RegionID", "u")
                      .Run();

    ASSERT_EQ(result.size(), 11);
    ASSERT_EQ(result[0].size(), 2);

    EXPECT_EQ(result[0][0], "RegionID");
    EXPECT_EQ(result[0][1], "u");

    EXPECT_EQ(result[1][0], "229");
    EXPECT_EQ(result[1][1], "27961");

    EXPECT_EQ(result[2][0], "2");
    EXPECT_EQ(result[2][1], "10413");

    EXPECT_EQ(result[3][0], "208");
    EXPECT_EQ(result[3][1], "3073");

    EXPECT_EQ(result[4][0], "1");
    EXPECT_EQ(result[4][1], "1720");

    EXPECT_EQ(result[5][0], "34");
    EXPECT_EQ(result[5][1], "1428");

    EXPECT_EQ(result[6][0], "158");
    EXPECT_EQ(result[6][1], "1110");

    EXPECT_EQ(result[7][0], "184");
    EXPECT_EQ(result[7][1], "987");

    EXPECT_EQ(result[8][0], "107");
    EXPECT_EQ(result[8][1], "966");

    EXPECT_EQ(result[9][0], "42");
    EXPECT_EQ(result[9][1], "956");

    EXPECT_EQ(result[10][0], "47");
    EXPECT_EQ(result[10][1], "943");
}

TEST_F(ClickBench, Q19) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("UserID = 435090932899640449")
                      .Select("UserID")
                      .Run();
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].size(), 1);
    EXPECT_EQ(result[0][0], "UserID");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;
    int result = RUN_ALL_TESTS();
    google::ShutdownGoogleLogging();
    return result;
}
