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
        // column_engine::ConvertToColumnar("hits_sample.csv", "hits_schema.csv", "col.col", 8192);
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
    ASSERT_EQ(result[0], (std::vector<std::string>{"UserID", "SearchPhrase", "COUNT(*)"}));
    ASSERT_EQ(result.size(), 11u);
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

TEST_F(ClickBench, Q24) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("SearchPhrase <> ''")
                      .OrderBy("EventTime ASC")
                      .Limit(10)
                      .Select("SearchPhrase", "EventTime")
                      .Run();
    ASSERT_EQ(result[0], (std::vector<std::string>{"SearchPhrase", "EventTime"}));
    ASSERT_EQ(result.size(), 11u);
    // первые 4 строки детерминированы (уникальные таймстэмпы)
    EXPECT_EQ(result[1], (std::vector<std::string>{"ведомосквы не удалог на ногтей денье", "2013-07-14 20:00:03"}));
    EXPECT_EQ(result[2], (std::vector<std::string>{"ведомосквы не удалог на ногтей денье", "2013-07-14 20:00:03"}));
    EXPECT_EQ(result[3], (std::vector<std::string>{"армянск", "2013-07-14 20:00:05"}));
    EXPECT_EQ(result[4], (std::vector<std::string>{"армянск", "2013-07-14 20:00:05"}));
    // все 10 строк в диапазоне 20:00:03 — 20:00:09
    EXPECT_LE(result[1][1], "2013-07-14 20:00:09");
    EXPECT_GE(result[10][1], "2013-07-14 20:00:03");
}

TEST_F(ClickBench, Q25) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("SearchPhrase <> ''")
                      .OrderBy("SearchPhrase ASC")
                      .Limit(10)
                      .Select("SearchPhrase")
                      .Run();
    ASSERT_EQ(result[0], (std::vector<std::string>{"SearchPhrase"}));
    ASSERT_EQ(result.size(), 11u);
    const std::vector<std::vector<std::string>> expected_rows = {
        {"'exis disco ryder injected cuda 7269"},
        {"'kbnyjuj gjhnf gtgthm vfibys row 3 ставе"},
        {"'kbnyjuj gjhnf gtgthm vfibys row 3 ставе"},
        {"'kbnyst exfcnm vekmnbdfhrf"},
        {"'kbnyst exfcnm vekmnbdfhrf"},
        {"(http://kommedium=cpc&utm_source=main происход"},
        {"+100 дизелькатровский стой"},
        {"+100 дизелькатровский стой"},
        {"+100500 4.5 отзывы"},
        {"+100500 4.5 отзывы"},
    };
    ExpectResultMatches(result, {"SearchPhrase"}, expected_rows);
}

TEST_F(ClickBench, Q20) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("URL LIKE %google%")
                      .Aggregate("COUNT(*)")
                      .Select("COUNT(*)")
                      .Run();
    EXPECT_EQ(result[1][0], "95");
}

TEST_F(ClickBench, Q21) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("URL LIKE %google%")
                      .Where("SearchPhrase <> ''")
                      .GroupByAggregate("SearchPhrase", "MIN(URL), COUNT(*)")
                      .Rename("COUNT(*)", "c")
                      .OrderBy("c DESC")
                      .Limit(10)
                      .Select("SearchPhrase", "MIN(URL)", "c")
                      .Run();
    const std::vector<std::vector<std::string>> expected_rows = {
        {"один инструктура птахани нюши смотреть краси", "http://bdsm_position/2624217,2013-07-01:2013/frl-4/transport.ru/google%2F", "2"},
    };
    ExpectResultMatches(result, {"SearchPhrase", "MIN(URL)", "c"}, expected_rows);
}

TEST_F(ClickBench, Q22) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("Title LIKE %Google%")
                      .Where("URL NOT LIKE %.google.%")
                      .Where("SearchPhrase <> ''")
                      .GroupByAggregate("SearchPhrase", "MIN(URL), MIN(Title), COUNT(*), COUNT(DISTINCT UserID)")
                      .Rename("COUNT(*)", "c")
                      .OrderBy("c DESC")
                      .Limit(10)
                      .Select("SearchPhrase", "MIN(URL)", "MIN(Title)", "c", "COUNT(DISTINCT UserID)")
                      .Run();
    const std::vector<std::vector<std::string>> expected_rows = {
        {"коптимиквиды юриста с роуз рая", "https://produkty%2Fpulove.ru/booklyattion-war-sinij-9182/women", "Легко на участные участников., Цены - Стильная парнем. Саганрог догадения : Турции, купить у 10 дне кольные машинки не представки - Новая с избиение спродажа: котята 2014 г.в. Цена: 47500-10ECO060 – -------- купить квартиру Оренбург (России Galantrax Flamiliada Google, Nо 18 фотоконверк Супер Кардиган", "45", "12"},
        {"коптимиквиды юрий жд ворожные моем", "https://produkty%2Fpulove.ru/booklyattion-war-sinij-9182/women", "Легко на участные участников., Цены - Стильная парнем. Саганрог догадения : Турции, купить у 10 дне кольные машинки не представки - Новая с избиение спродажа: котята 2014 г.в. Цена: 47500-10ECO060 – -------- купить квартиру Оренбург (России Galantrax Flamiliada Google, Nо 18 фотоконверк Супер Кардиган", "16", "6"},
        {"ведомосквы вместу", "https://produkty%2Fpulove.ru/booklyattion-war-sinij-9182/women", "Convent-мененции: Бизнес спродажа коттекст) Скейтшоп Proskater.ru - Дизайнер) 1992 г.в. Цена дачного века Кированнале актеры Google (La Charm Boxer группатии, оформационка NIKE TRADE-IN 6750$, (г. Днепрочитании онлайники — Избранное упражнения - играть и цене, выполная", "15", "9"},
        {"вспомидоры,отека обучение стека", "https://produkty%2Fpulove.ru/booklyattion-war-sinij-9182/women", "Легко на участные участников., Цены - Стильная парнем. Саганрог догадения : Турции, купить у 10 дне кольные машинки не представки - Новая с избиение спродажа: котята 2014 г.в. Цена: 47500-10ECO060 – -------- купить квартиру Оренбург (России Galantrax Flamiliada Google, Nо 18 фотоконверк Супер Кардиган", "10", "1"},
        {"коптимизаностиницы", "https://produkty%2Fpulove.ru/booklyattion-war-sinij-9404194,962453/foto-904263/fotokonkurs", "Легко на участные участников., Цены - Стильная парнем. Саганрог догадения : Турции, купить у 10 дне кольные машинки не представки - Новая с избиение спродажа: котята 2014 г.в. Цена: 47500-10ECO060 – -------- купить квартиру Оренбург (России Galantrax Flamiliada Google, Nо 18 фотоконверк Супер Кардиган", "8", "2"},
        {"ведомосквиталия страции", "https://produkty%2Fpulove.ru/booklyattion-war-sinij-9182/women", "Легко на участные участников., Цены - Стильная парнем. Саганрог догадения : Турции, купить у 10 дне кольные машинки не представки - Новая с избиение спродажа: котята 2014 г.в. Цена: 47500-10ECO060 – -------- купить квартиру Оренбург (России Galantrax Flamiliada Google, Nо 18 фотоконверк Супер Кардиган", "8", "3"},
        {"коптимашевск но в хорошем качестве", "https://produkty%2Fpulove.ru/booklyattion-war-sinij-9182/women", "Легко на участные участников., Цены - Стильная парнем. Саганрог догадения : Турции, купить у 10 дне кольные машинки не представки - Новая с избиение спродажа: котята 2014 г.в. Цена: 47500-10ECO060 – -------- купить квартиру Оренбург (России Galantrax Flamiliada Google, Nо 18 фотоконверк Супер Кардиган", "6", "3"},
        {"поттек кисловая коньюктивное", "https://produkty%2Fpulove.ru/booklyattion-war-sinij-9182/women", "Легко на участные участников., Цены - Стильная парнем. Саганрог догадения : Турции, купить у 10 дне кольные машинки не представки - Новая с избиение спродажа: котята 2014 г.в. Цена: 47500-10ECO060 – -------- купить квартиру Оренбург (России Galantrax Flamiliada Google, Nо 18 фотоконверк Супер Кардиган", "5", "1"},
        {"вспомидоры,отзывы луи видация", "https://produkty%2Fpulove.ru/booklyattion-war-sinij-9182/women", "Легко на участные участников., Цены - Стильная парнем. Саганрог догадения : Турции, купить у 10 дне кольные машинки не представки - Новая с избиение спродажа: котята 2014 г.в. Цена: 47500-10ECO060 – -------- купить квартиру Оренбург (России Galantrax Flamiliada Google, Nо 18 фотоконверк Супер Кардиган", "5", "2"},
        {"коптимиквиды юрий последняя", "https://produkty%2Fpulove.ru/booklyattion-war-sinij-9404194,962453/foto", "Легко на участные участников., Цены - Стильная парнем. Саганрог догадения : Турции, купить у 10 дне кольные машинки не представки - Новая с избиение спродажа: котята 2014 г.в. Цена: 47500-10ECO060 – -------- купить квартиру Оренбург (России Galantrax Flamiliada Google, Nо 18 фотоконверк Супер Кардиган", "5", "1"},
    };
    ExpectResultMatches(result, {"SearchPhrase", "MIN(URL)", "MIN(Title)", "c", "COUNT(DISTINCT UserID)"}, expected_rows);
}

TEST_F(ClickBench, Q31) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("SearchPhrase <> ''")
                      .GroupByAggregate("SearchEngineID", "ClientIP", "COUNT(*), SUM(IsRefresh), AVG(ResolutionWidth)")
                      .Rename("COUNT(*)", "c")
                      .OrderBy("c DESC")
                      .Limit(10)
                      .Select("SearchEngineID", "ClientIP", "c", "SUM(IsRefresh)", "AVG(ResolutionWidth)")
                      .Run();
    ASSERT_EQ(result[0], (std::vector<std::string>{"SearchEngineID", "ClientIP", "c", "SUM(IsRefresh)", "AVG(ResolutionWidth)"}));
    ASSERT_EQ(result.size(), 11u);
    // top row is deterministic — SearchEngineID=2 dominates
    EXPECT_EQ(result[1][0], "2");
}

TEST_F(ClickBench, Q33) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .GroupByAggregate("WatchID", "ClientIP", "COUNT(*), SUM(IsRefresh), AVG(ResolutionWidth)")
                      .Rename("COUNT(*)", "c")
                      .OrderBy("c DESC")
                      .Limit(10)
                      .Select("WatchID", "ClientIP", "c", "SUM(IsRefresh)", "AVG(ResolutionWidth)")
                      .Run();
    ASSERT_EQ(result[0], (std::vector<std::string>{"WatchID", "ClientIP", "c", "SUM(IsRefresh)", "AVG(ResolutionWidth)"}));
    // WatchID is unique per row so all counts are 1 — order is non-deterministic
    ASSERT_EQ(result.size(), 11u);
    for (size_t i = 1; i <= 10; ++i) {
        EXPECT_EQ(result[i][2], "1");
    }
}

TEST_F(ClickBench, Q34) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .GroupByAggregate("URL", "COUNT(*)")
                      .Rename("COUNT(*)", "c")
                      .OrderBy("c DESC")
                      .Limit(10)
                      .Select("URL", "c")
                      .Run();
    const std::vector<std::vector<std::string>> expected_rows = {
        {"http://irr.ru/index.php?showalbum/login-leniya7777294,938303130", "58970"},
        {"http://komme%2F27.0.1453.116", "29580"},
        {"https://produkty%2Fproduct", "11464"},
        {"http://irr.ru/index.php?showalbum/login-kapusta-advert2668]=0&order_by=0", "10480"},
        {"http://irr.ru/index.php?showalbum/login-kapustic/product_name", "10128"},
        {"http://irr.ru/index.php", "7758"},
        {"https://produkty%2F", "6649"},
        {"http://irr.ru/index.php?showalbum/login", "6141"},
        {"https://produkty/kurortmag", "5764"},
        {"https://produkty%2Fpulove.ru/album/login", "5495"},
    };
    ExpectResultMatches(result, {"URL", "c"}, expected_rows);
}

TEST_F(ClickBench, Q36) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("CounterID = 62")
                      .Where("EventDate >= '2013-07-01'")
                      .Where("EventDate <= '2013-07-31'")
                      .Where("DontCountHits = 0")
                      .Where("IsRefresh = 0")
                      .Where("URL <> ''")
                      .GroupByAggregate("URL", "COUNT(*)")
                      .Rename("COUNT(*)", "PageViews")
                      .OrderBy("PageViews DESC")
                      .Limit(10)
                      .Select("URL", "PageViews")
                      .Run();
    const std::vector<std::vector<std::string>> expected_rows = {
        {"http://irr.ru/index.php?showalbum/login-leniya7777294,938303130", "56533"},
        {"http://komme%2F27.0.1453.116", "28819"},
        {"http://irr.ru/index.php?showalbum/login-kapusta-advert2668]=0&order_by=0", "10325"},
        {"http://irr.ru/index.php?showalbum/login-kapustic/product_name", "9650"},
        {"http://irr.ru/index.php", "7530"},
        {"http://irr.ru/index.php?showalbum/login", "6032"},
        {"http://komme%2F27.0.1453.116 Safari%2F5.0 (compatible; MSIE 9.0;", "4271"},
        {"http://irr.ru/index.php?showalbum/login-kupalnik", "2475"},
        {"http://irr.ru/index.php?showalbum/login-kapusta-advert27256.html_params", "2300"},
        {"http://komme%2F27.0.1453.116 Safari", "1612"},
    };
    ExpectResultMatches(result, {"URL", "PageViews"}, expected_rows);
}

TEST_F(ClickBench, Q37) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("CounterID = 62")
                      .Where("EventDate >= '2013-07-01'")
                      .Where("EventDate <= '2013-07-31'")
                      .Where("DontCountHits = 0")
                      .Where("IsRefresh = 0")
                      .Where("Title <> ''")
                      .GroupByAggregate("Title", "COUNT(*)")
                      .Rename("COUNT(*)", "PageViews")
                      .OrderBy("PageViews DESC")
                      .Limit(10)
                      .Select("Title", "PageViews")
                      .Run();
    const std::vector<std::vector<std::string>> expected_rows = {
        {"Тест (Россия) - Яндекс", "67544"},
        {"Шарарай), Выбрать! - обсуждаются на голд: Шоубиз - Свободная историс", "46670"},
        {"Приморск - IRR.ru", "46530"},
        {"Брюки New Era H (Асус) 258 общая выплаток, горшечными", "21166"},
        {"Теплоску на", "13432"},
        {"Приморск (Россия) - Яндекс.Видео", "8260"},
        {"AUTO.ria.ua ™ - Аппер", "8115"},
        {"Dave and Hotpoint sport – самые вещие", "7866"},
        {"OWAProfessign), продать", "5754"},
        {"Труси - Шоубиз", "5692"},
    };
    ExpectResultMatches(result, {"Title", "PageViews"}, expected_rows);
}

TEST_F(ClickBench, DISABLED_Q42) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("CounterID = 62")
                      .Where("EventDate >= '2013-07-14'")
                      .Where("EventDate <= '2013-07-15'")
                      .Where("IsRefresh = 0")
                      .Where("DontCountHits = 0")
                      .GroupByAggregate("strftime('%M', EventTime)", "COUNT(*)")
                      .Rename("COUNT(*)", "PageViews")
                      .OrderBy("strftime('%M', EventTime) ASC")
                      .Limit(10)
                      .Select("strftime('%M', EventTime)", "PageViews")
                      .Run();
    const std::vector<std::vector<std::string>> expected_rows = {
        {"2013-07-14 20:00:00", "256"},
        {"2013-07-14 20:01:00", "259"},
        {"2013-07-14 20:02:00", "256"},
        {"2013-07-14 20:03:00", "238"},
        {"2013-07-14 20:04:00", "255"},
        {"2013-07-14 20:05:00", "282"},
        {"2013-07-14 20:06:00", "227"},
        {"2013-07-14 20:07:00", "265"},
        {"2013-07-14 20:08:00", "231"},
        {"2013-07-14 20:09:00", "218"},
    };
    ExpectResultMatches(result, {"strftime('%M', EventTime)", "PageViews"}, expected_rows);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;
    int result = RUN_ALL_TESTS();
    google::ShutdownGoogleLogging();
    return result;
}
