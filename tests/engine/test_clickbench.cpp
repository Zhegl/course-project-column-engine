#include <gtest/gtest.h>
#include <convert/convert.h>
#include <engine/engine.h>
#include <glog/logging.h>
#include "api.h"

namespace {

void ExpectResultMatches(const column_engine::QueryResult& result,
                         const std::vector<std::string>& expected_header,
                         const std::vector<std::vector<std::string>>& expected_rows) {
    ASSERT_EQ(result.size(), expected_rows.size() + 1);
    ASSERT_EQ(result[0], expected_header);
    for (size_t i = 0; i < expected_rows.size(); ++i) {
        EXPECT_EQ(result[i + 1], expected_rows[i]) << "Mismatch at row " << i + 1;
    }
}

}  // namespace

class ClickBench : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        //column_engine::ConvertToColumnar("hits_sample.csv", "hits_schema.csv", "col.col", 10000);
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
                      .Aggregate("COUNT(DISTINCT UserID)")
                      .Select("COUNT(DISTINCT UserID)")
                      .Run();
    std::cout << result[1][0] << "\n";
    ASSERT_EQ(result[0].size(), 1);
    EXPECT_EQ(result[0][0], "COUNT(DISTINCT UserID)");
    EXPECT_EQ(result[1][0], "79842");
}

TEST_F(ClickBench, Q5) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Aggregate("COUNT(DISTINCT SearchPhrase)")
                      .Select("COUNT(DISTINCT SearchPhrase)")
                      .Run();
    std::cout << result[1][0] << "\n";
    ASSERT_EQ(result[0].size(), 1);
    EXPECT_EQ(result[0][0], "COUNT(DISTINCT SearchPhrase)");
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
                      .GroupByAggregate("RegionID", "COUNT(DISTINCT UserID)")
                      .Rename("COUNT(DISTINCT UserID)", "u")
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

TEST_F(ClickBench, Q9) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .GroupByAggregate(
                          "RegionID",
                          "SUM(AdvEngineID), COUNT(*), AVG(ResolutionWidth), COUNT(DISTINCT UserID)")
                      .Rename("COUNT(*)", "c")
                      .OrderBy("c DESC")
                      .Limit(10)
                      .Select("RegionID", "SUM(AdvEngineID)", "c", "AVG(ResolutionWidth)",
                              "COUNT(DISTINCT UserID)")
                      .Run();
    const std::vector<std::vector<std::string>> expected_rows = {
        {"229", "38044", "426412", "1612", "27961"},
        {"2", "12801", "148193", "1593", "10413"},
        {"208", "2673", "30614", "1490", "3073"},
        {"1", "1802", "28577", "1623", "1720"},
        {"34", "508", "14329", "1592", "1428"},
        {"47", "1041", "13661", "1637", "943"},
        {"158", "78", "13294", "1576", "1110"},
        {"7", "1166", "11679", "1627", "647"},
        {"42", "642", "11547", "1625", "956"},
        {"184", "30", "10157", "1614", "987"},
    };
    ExpectResultMatches(result,
                        {"RegionID", "SUM(AdvEngineID)", "c", "AVG(ResolutionWidth)",
                         "COUNT(DISTINCT UserID)"},
                        expected_rows);
}

TEST_F(ClickBench, Q10) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("MobilePhoneModel <> ''")
                      .GroupByAggregate("MobilePhoneModel", "COUNT(DISTINCT UserID)")
                      .Rename("COUNT(DISTINCT UserID)", "u")
                      .OrderBy("u DESC")
                      .Limit(10)
                      .Select("MobilePhoneModel", "u")
                      .Run();
    const std::vector<std::vector<std::string>> expected_rows = {
        {"iPad", "2303"},
        {"iPhone", "107"},
        {"A500", "34"},
        {"N8-00", "12"},
        {"GT-P7300B", "12"},
        {"iPho", "11"},
        {"3110000", "6"},
        {"IQ245Plus", "5"},
        {"eagle75", "4"},
        {"GT-S5830", "3"},
    };
    ExpectResultMatches(result, {"MobilePhoneModel", "u"}, expected_rows);
}

TEST_F(ClickBench, Q11) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("MobilePhoneModel <> ''")
                      .GroupByAggregate("MobilePhone", "MobilePhoneModel", "COUNT(DISTINCT UserID)")
                      .Rename("COUNT(DISTINCT UserID)", "u")
                      .OrderBy("u DESC")
                      .Limit(10)
                      .Select("MobilePhone", "MobilePhoneModel", "u")
                      .Run();
    const std::vector<std::vector<std::string>> expected_rows = {
        {"1", "iPad", "1967"},
        {"5", "iPad", "97"},
        {"7", "iPad", "79"},
        {"6", "iPad", "55"},
        {"6", "iPhone", "37"},
        {"26", "iPhone", "36"},
        {"118", "A500", "34"},
        {"32", "iPad", "29"},
        {"60", "iPad", "22"},
        {"13", "iPad", "12"},
    };
    ExpectResultMatches(result, {"MobilePhone", "MobilePhoneModel", "u"}, expected_rows);
}

TEST_F(ClickBench, Q12) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("SearchPhrase <> ''")
                      .GroupByAggregate("SearchPhrase", "COUNT(*)")
                      .Rename("COUNT(*)", "c")
                      .OrderBy("c DESC")
                      .Limit(10)
                      .Select("SearchPhrase", "c")
                      .Run();
    const std::vector<std::vector<std::string>> expected_rows = {
        {"ведомосквы вместу", "4943"},
        {"ведомосквы вы из", "2471"},
        {"ведомосквиталия страции", "2026"},
        {"ведомосковский", "1686"},
        {"покеты рецепт засня", "961"},
        {"рецепты сбербан", "788"},
        {"авторий", "705"},
        {"ведомосква", "446"},
        {"ведомосквы новые водительная болгарин", "411"},
        {"инстанец жизнь", "391"},
    };
    ExpectResultMatches(result, {"SearchPhrase", "c"}, expected_rows);
}

TEST_F(ClickBench, Q13) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("SearchPhrase <> ''")
                      .GroupByAggregate("SearchPhrase", "COUNT(DISTINCT UserID)")
                      .Rename("COUNT(DISTINCT UserID)", "u")
                      .OrderBy("u DESC")
                      .Limit(10)
                      .Select("SearchPhrase", "u")
                      .Run();
    const std::vector<std::vector<std::string>> expected_rows = {
        {"ведомосквы вместу", "1381"},
        {"ведомосквы вы из", "678"},
        {"ведомосквиталия страции", "658"},
        {"рецепты сбербан", "594"},
        {"ведомосковский", "407"},
        {"инстанец жизнь", "292"},
        {"покеты рецепт засня", "281"},
        {"авторий", "196"},
        {"рецепт блиноленские", "135"},
        {"ведомосква", "129"},
    };
    ExpectResultMatches(result, {"SearchPhrase", "u"}, expected_rows);
}

TEST_F(ClickBench, Q14) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("SearchPhrase <> ''")
                      .GroupByAggregate("SearchEngineID", "SearchPhrase", "COUNT(*)")
                      .Rename("COUNT(*)", "c")
                      .OrderBy("c DESC")
                      .Limit(10)
                      .Select("SearchEngineID", "SearchPhrase", "c")
                      .Run();
    const std::vector<std::vector<std::string>> expected_rows = {
        {"2", "ведомосквы вместу", "3478"},
        {"2", "ведомосквы вы из", "1857"},
        {"2", "ведомосковский", "1682"},
        {"2", "ведомосквиталия страции", "1434"},
        {"4", "покеты рецепт засня", "959"},
        {"2", "рецепты сбербан", "737"},
        {"3", "ведомосквы вместу", "660"},
        {"2", "авторий", "576"},
        {"3", "ведомосквиталия страции", "494"},
        {"4", "ведомосквы вместу", "442"},
    };
    ExpectResultMatches(result, {"SearchEngineID", "SearchPhrase", "c"}, expected_rows);
}

TEST_F(ClickBench, Q15) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .GroupByAggregate("UserID", "COUNT(*)")
                      .OrderBy("COUNT(*) DESC")
                      .Limit(10)
                      .Select("UserID", "COUNT(*)")
                      .Run();
    const std::vector<std::vector<std::string>> expected_rows = {
        {"1508127196834704092", "1303"},
        {"3205616454965152970", "949"},
        {"502693359570399458", "893"},
        {"873022393995828557", "876"},
        {"2256536417172705921", "695"},
        {"340634745528635910", "610"},
        {"72709437341035504", "560"},
        {"5705194083846317709", "532"},
        {"1257144732630861346", "524"},
        {"4885305169967046117", "516"},
    };
    ExpectResultMatches(result, {"UserID", "COUNT(*)"}, expected_rows);
}

TEST_F(ClickBench, Q16) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .GroupByAggregate("UserID", "SearchPhrase", "COUNT(*)")
                      .OrderBy("COUNT(*) DESC")
                      .Limit(10)
                      .Select("UserID", "SearchPhrase", "COUNT(*)")
                      .Run();
    const std::vector<std::vector<std::string>> expected_rows = {
        {"1508127196834704092", "", "1303"},
        {"3205616454965152970", "", "949"},
        {"502693359570399458", "", "893"},
        {"873022393995828557", "", "876"},
        {"2256536417172705921", "", "695"},
        {"340634745528635910", "", "610"},
        {"72709437341035504", "", "560"},
        {"5705194083846317709", "", "532"},
        {"614605011960296602", "", "506"},
        {"775643969820522877", "", "483"},
    };
    ExpectResultMatches(result, {"UserID", "SearchPhrase", "COUNT(*)"}, expected_rows);
}

TEST_F(ClickBench, Q17) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .GroupByAggregate("UserID", "SearchPhrase", "COUNT(*)")
                      .Limit(10)
                      .Select("UserID", "SearchPhrase", "COUNT(*)")
                      .Run();
    const std::vector<std::vector<std::string>> expected_rows = {
        {"-9219796849203399214", "", "3"},
        {"-9219054275930888834", "", "1"},
        {"-9218024831767047585", "жена дата киноафиша анке", "1"},
        {"-9214794650081452866", "", "4"},
        {"-9214751021948998350", "авомосква веб каменисный", "2"},
        {"-9213728704863893851", "", "2"},
        {"-9213728704863893851", "чагин выпуска на волна 2 сезон 24 резюме онлайн", "2"},
        {"-9213106781151947221", "", "9"},
        {"-9208956738506700293", "", "2"},
        {"-9206351631809765116", "", "5"},
    };
    ExpectResultMatches(result, {"UserID", "SearchPhrase", "COUNT(*)"}, expected_rows);
}

TEST_F(ClickBench, Q18) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .GroupByAggregate("UserID", "strftime('%M', EventTime)", "SearchPhrase",
                                        "COUNT(*)")
                      .OrderBy("COUNT(*) DESC")
                      .Limit(10)
                      .Select("UserID", "strftime('%M', EventTime)", "SearchPhrase", "COUNT(*)")
                      .Run();
    const std::vector<std::vector<std::string>> expected_rows = {
        {"5216851095034646002", "51", "", "80"},
        {"5216851095034646002", "52", "", "67"},
        {"1074353211169645510", "8", "", "37"},
        {"1220910554975721402", "13", "", "35"},
        {"1074353211169645510", "19", "", "34"},
        {"4673379180966332110", "0", "", "34"},
        {"614605011960296602", "18", "", "34"},
        {"1074353211169645510", "9", "", "33"},
        {"1508127196834704092", "14", "", "33"},
        {"502693359570399458", "59", "", "33"},
    };
    ExpectResultMatches(result,
                        {"UserID", "strftime('%M', EventTime)", "SearchPhrase", "COUNT(*)"},
                        expected_rows);
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
