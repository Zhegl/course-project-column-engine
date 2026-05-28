#include <gtest/gtest.h>
#include <convert/convert.h>
#include <api/columnar_engine.h>
#include <glog/logging.h>

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
        column_engine::ConvertToColumnar("hits_sample.csv", "hits_schema.csv", "col.col", 8192);
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
                      .Add("strftime('%M', EventTime)")
                      .GroupByAggregate("UserID", "strftime('%M', EventTime)", "SearchPhrase",
                                        "COUNT(*)")
                      .OrderBy("COUNT(*) DESC")
                      .Limit(10)
                      .Select("UserID", "strftime('%M', EventTime)", "SearchPhrase", "COUNT(*)")
                      .Run();
    const std::vector<std::vector<std::string>> expected_rows = {
        {"5216851095034646002", "51", "", "80"},
        {"5216851095034646002", "52", "", "67"},
        {"1074353211169645510", "08", "", "37"},
        {"1220910554975721402", "13", "", "35"},
        {"4673379180966332110", "00", "", "34"},
        {"614605011960296602", "18", "", "34"},
        {"1074353211169645510", "19", "", "34"},
        {"502693359570399458", "59", "", "33"},
        {"1074353211169645510", "09", "", "33"},
        {"1508127196834704092", "09", "", "33"},
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

TEST_F(ClickBench, Q26) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("SearchPhrase <> ''")
                      .OrderBy("SearchPhrase ASC")
                      .Limit(10)
                      .Select("SearchPhrase")
                      .Run();
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

TEST_F(ClickBench, Q28) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("URL <> ''")
                      .Add("length(URL)")
                      .GroupByAggregate("CounterID", "AVG(length(URL)), COUNT(*), MIN(URL)")
                      .Rename("AVG(length(URL))", "l")
                      .Rename("COUNT(*)", "c")
                      .Where("c > 100000")
                      .OrderBy("l DESC")
                      .Limit(25)
                      .Select("CounterID", "l", "c", "MIN(URL)")
                      .Run();
    ASSERT_EQ(result[0], (std::vector<std::string>{"CounterID", "l", "c", "MIN(URL)"}));
    ASSERT_LE(result.size(), 26u);
}

TEST_F(ClickBench, Q29) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Add("ResolutionWidth + 0").Add("ResolutionWidth + 1").Add("ResolutionWidth + 2")
                      .Add("ResolutionWidth + 3").Add("ResolutionWidth + 4").Add("ResolutionWidth + 5")
                      .Add("ResolutionWidth + 6").Add("ResolutionWidth + 7").Add("ResolutionWidth + 8")
                      .Add("ResolutionWidth + 9").Add("ResolutionWidth + 10").Add("ResolutionWidth + 11")
                      .Add("ResolutionWidth + 12").Add("ResolutionWidth + 13").Add("ResolutionWidth + 14")
                      .Add("ResolutionWidth + 15").Add("ResolutionWidth + 16").Add("ResolutionWidth + 17")
                      .Add("ResolutionWidth + 18").Add("ResolutionWidth + 19").Add("ResolutionWidth + 20")
                      .Add("ResolutionWidth + 21").Add("ResolutionWidth + 22").Add("ResolutionWidth + 23")
                      .Add("ResolutionWidth + 24").Add("ResolutionWidth + 25").Add("ResolutionWidth + 26")
                      .Add("ResolutionWidth + 27").Add("ResolutionWidth + 28").Add("ResolutionWidth + 29")
                      .Add("ResolutionWidth + 30").Add("ResolutionWidth + 31").Add("ResolutionWidth + 32")
                      .Add("ResolutionWidth + 33").Add("ResolutionWidth + 34").Add("ResolutionWidth + 35")
                      .Add("ResolutionWidth + 36").Add("ResolutionWidth + 37").Add("ResolutionWidth + 38")
                      .Add("ResolutionWidth + 39").Add("ResolutionWidth + 40").Add("ResolutionWidth + 41")
                      .Add("ResolutionWidth + 42").Add("ResolutionWidth + 43").Add("ResolutionWidth + 44")
                      .Add("ResolutionWidth + 45").Add("ResolutionWidth + 46").Add("ResolutionWidth + 47")
                      .Add("ResolutionWidth + 48").Add("ResolutionWidth + 49").Add("ResolutionWidth + 50")
                      .Add("ResolutionWidth + 51").Add("ResolutionWidth + 52").Add("ResolutionWidth + 53")
                      .Add("ResolutionWidth + 54").Add("ResolutionWidth + 55").Add("ResolutionWidth + 56")
                      .Add("ResolutionWidth + 57").Add("ResolutionWidth + 58").Add("ResolutionWidth + 59")
                      .Add("ResolutionWidth + 60").Add("ResolutionWidth + 61").Add("ResolutionWidth + 62")
                      .Add("ResolutionWidth + 63").Add("ResolutionWidth + 64").Add("ResolutionWidth + 65")
                      .Add("ResolutionWidth + 66").Add("ResolutionWidth + 67").Add("ResolutionWidth + 68")
                      .Add("ResolutionWidth + 69").Add("ResolutionWidth + 70").Add("ResolutionWidth + 71")
                      .Add("ResolutionWidth + 72").Add("ResolutionWidth + 73").Add("ResolutionWidth + 74")
                      .Add("ResolutionWidth + 75").Add("ResolutionWidth + 76").Add("ResolutionWidth + 77")
                      .Add("ResolutionWidth + 78").Add("ResolutionWidth + 79").Add("ResolutionWidth + 80")
                      .Add("ResolutionWidth + 81").Add("ResolutionWidth + 82").Add("ResolutionWidth + 83")
                      .Add("ResolutionWidth + 84").Add("ResolutionWidth + 85").Add("ResolutionWidth + 86")
                      .Add("ResolutionWidth + 87").Add("ResolutionWidth + 88").Add("ResolutionWidth + 89")
                      .Aggregate("SUM(ResolutionWidth + 0), SUM(ResolutionWidth + 1), SUM(ResolutionWidth + 2), SUM(ResolutionWidth + 3), SUM(ResolutionWidth + 4), SUM(ResolutionWidth + 5), SUM(ResolutionWidth + 6), SUM(ResolutionWidth + 7), SUM(ResolutionWidth + 8), SUM(ResolutionWidth + 9), SUM(ResolutionWidth + 10), SUM(ResolutionWidth + 11), SUM(ResolutionWidth + 12), SUM(ResolutionWidth + 13), SUM(ResolutionWidth + 14), SUM(ResolutionWidth + 15), SUM(ResolutionWidth + 16), SUM(ResolutionWidth + 17), SUM(ResolutionWidth + 18), SUM(ResolutionWidth + 19), SUM(ResolutionWidth + 20), SUM(ResolutionWidth + 21), SUM(ResolutionWidth + 22), SUM(ResolutionWidth + 23), SUM(ResolutionWidth + 24), SUM(ResolutionWidth + 25), SUM(ResolutionWidth + 26), SUM(ResolutionWidth + 27), SUM(ResolutionWidth + 28), SUM(ResolutionWidth + 29), SUM(ResolutionWidth + 30), SUM(ResolutionWidth + 31), SUM(ResolutionWidth + 32), SUM(ResolutionWidth + 33), SUM(ResolutionWidth + 34), SUM(ResolutionWidth + 35), SUM(ResolutionWidth + 36), SUM(ResolutionWidth + 37), SUM(ResolutionWidth + 38), SUM(ResolutionWidth + 39), SUM(ResolutionWidth + 40), SUM(ResolutionWidth + 41), SUM(ResolutionWidth + 42), SUM(ResolutionWidth + 43), SUM(ResolutionWidth + 44), SUM(ResolutionWidth + 45), SUM(ResolutionWidth + 46), SUM(ResolutionWidth + 47), SUM(ResolutionWidth + 48), SUM(ResolutionWidth + 49), SUM(ResolutionWidth + 50), SUM(ResolutionWidth + 51), SUM(ResolutionWidth + 52), SUM(ResolutionWidth + 53), SUM(ResolutionWidth + 54), SUM(ResolutionWidth + 55), SUM(ResolutionWidth + 56), SUM(ResolutionWidth + 57), SUM(ResolutionWidth + 58), SUM(ResolutionWidth + 59), SUM(ResolutionWidth + 60), SUM(ResolutionWidth + 61), SUM(ResolutionWidth + 62), SUM(ResolutionWidth + 63), SUM(ResolutionWidth + 64), SUM(ResolutionWidth + 65), SUM(ResolutionWidth + 66), SUM(ResolutionWidth + 67), SUM(ResolutionWidth + 68), SUM(ResolutionWidth + 69), SUM(ResolutionWidth + 70), SUM(ResolutionWidth + 71), SUM(ResolutionWidth + 72), SUM(ResolutionWidth + 73), SUM(ResolutionWidth + 74), SUM(ResolutionWidth + 75), SUM(ResolutionWidth + 76), SUM(ResolutionWidth + 77), SUM(ResolutionWidth + 78), SUM(ResolutionWidth + 79), SUM(ResolutionWidth + 80), SUM(ResolutionWidth + 81), SUM(ResolutionWidth + 82), SUM(ResolutionWidth + 83), SUM(ResolutionWidth + 84), SUM(ResolutionWidth + 85), SUM(ResolutionWidth + 86), SUM(ResolutionWidth + 87), SUM(ResolutionWidth + 88), SUM(ResolutionWidth + 89)")
                      .Run();
    ASSERT_EQ(result.size(), 2u);
    const std::vector<std::string> expected_row = {
        "1604051916","1605051893","1606051870","1607051847","1608051824","1609051801","1610051778","1611051755","1612051732","1613051709",
        "1614051686","1615051663","1616051640","1617051617","1618051594","1619051571","1620051548","1621051525","1622051502","1623051479",
        "1624051456","1625051433","1626051410","1627051387","1628051364","1629051341","1630051318","1631051295","1632051272","1633051249",
        "1634051226","1635051203","1636051180","1637051157","1638051134","1639051111","1640051088","1641051065","1642051042","1643051019",
        "1644050996","1645050973","1646050950","1647050927","1648050904","1649050881","1650050858","1651050835","1652050812","1653050789",
        "1654050766","1655050743","1656050720","1657050697","1658050674","1659050651","1660050628","1661050605","1662050582","1663050559",
        "1664050536","1665050513","1666050490","1667050467","1668050444","1669050421","1670050398","1671050375","1672050352","1673050329",
        "1674050306","1675050283","1676050260","1677050237","1678050214","1679050191","1680050168","1681050145","1682050122","1683050099",
        "1684050076","1685050053","1686050030","1687050007","1688049984","1689049961","1690049938","1691049915","1692049892","1693049869",
    };
    EXPECT_EQ(result[1], expected_row);
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

TEST_F(ClickBench, Q32) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("SearchPhrase <> ''")
                      .GroupByAggregate("WatchID", "ClientIP", "COUNT(*), SUM(IsRefresh), AVG(ResolutionWidth)")
                      .Rename("COUNT(*)", "c")
                      .OrderBy("c DESC")
                      .Limit(10)
                      .Select("WatchID", "ClientIP", "c", "SUM(IsRefresh)", "AVG(ResolutionWidth)")
                      .Run();
    ASSERT_EQ(result[0], (std::vector<std::string>{"WatchID", "ClientIP", "c", "SUM(IsRefresh)", "AVG(ResolutionWidth)"}));
    ASSERT_LE(result.size(), 11u);
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

TEST_F(ClickBench, Q35) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Add("ClientIP - 1")
                      .Add("ClientIP - 2")
                      .Add("ClientIP - 3")
                      .GroupByAggregate("ClientIP", "ClientIP - 1", "ClientIP - 2", "ClientIP - 3", "COUNT(*)")
                      .Rename("COUNT(*)", "c")
                      .OrderBy("c DESC")
                      .Limit(10)
                      .Select("ClientIP", "ClientIP - 1", "ClientIP - 2", "ClientIP - 3", "c")
                      .Run();
    const std::vector<std::vector<std::string>> expected_rows = {
        {"-267589304", "-267589305", "-267589306", "-267589307", "1733"},
        {"-1064396353", "-1064396354", "-1064396355", "-1064396356", "1604"},
        {"2113746632", "2113746631", "2113746630", "2113746629", "1552"},
        {"-1071668921", "-1071668922", "-1071668923", "-1071668924", "1544"},
        {"2127211172", "2127211171", "2127211170", "2127211169", "1485"},
        {"1700560340", "1700560339", "1700560338", "1700560337", "1311"},
        {"657371700", "657371699", "657371698", "657371697", "1199"},
        {"1450638336", "1450638335", "1450638334", "1450638333", "1015"},
        {"1992394514", "1992394513", "1992394512", "1992394511", "1015"},
        {"1503108906", "1503108905", "1503108904", "1503108903", "990"},
    };
    ExpectResultMatches(result, {"ClientIP", "ClientIP - 1", "ClientIP - 2", "ClientIP - 3", "c"}, expected_rows);
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

TEST_F(ClickBench, Q38) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("CounterID = 62")
                      .Where("EventDate >= '2013-07-01'")
                      .Where("EventDate <= '2013-07-31'")
                      .Where("IsRefresh = 0")
                      .Where("IsLink <> 0")
                      .Where("IsDownload = 0")
                      .GroupByAggregate("URL", "COUNT(*)")
                      .Rename("COUNT(*)", "PageViews")
                      .OrderBy("PageViews DESC")
                      .Offset(1000)
                      .Limit(10)
                      .Select("URL", "PageViews")
                      .Run();
    ASSERT_EQ(result[0], (std::vector<std::string>{"URL", "PageViews"}));
    ASSERT_LE(result.size(), 11u);
}

TEST_F(ClickBench, Q40) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("CounterID = 62")
                      .Where("EventDate >= '2013-07-01'")
                      .Where("EventDate <= '2013-07-31'")
                      .Where("IsRefresh = 0")
                      .Where("TraficSourceID IN (-1, 6)")
                      .Where("RefererHash = 3594120000172545465")
                      .GroupByAggregate("URLHash", "EventDate", "COUNT(*)")
                      .Rename("COUNT(*)", "PageViews")
                      .OrderBy("PageViews DESC")
                      .Offset(100)
                      .Limit(10)
                      .Select("URLHash", "EventDate", "PageViews")
                      .Run();
    ASSERT_EQ(result[0], (std::vector<std::string>{"URLHash", "EventDate", "PageViews"}));
    ASSERT_LE(result.size(), 11u);
}

TEST_F(ClickBench, Q41) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("CounterID = 62")
                      .Where("EventDate >= '2013-07-01'")
                      .Where("EventDate <= '2013-07-31'")
                      .Where("IsRefresh = 0")
                      .Where("TraficSourceID IN (-1, 6)")
                      .Where("RefererHash = 3594120000172545465")
                      .GroupByAggregate("URLHash", "EventDate", "COUNT(*)")
                      .Rename("COUNT(*)", "PageViews")
                      .OrderBy("PageViews DESC")
                      .Offset(100)
                      .Limit(10)
                      .Select("URLHash", "EventDate", "PageViews")
                      .Run();
    ASSERT_EQ(result[0], (std::vector<std::string>{"URLHash", "EventDate", "PageViews"}));
    ASSERT_LE(result.size(), 11u);
}


TEST_F(ClickBench, Q42) {
    column_engine::Engine engine("col.col");
    auto result = engine.Api()
                      .Where("CounterID = 62")
                      .Where("EventDate >= '2013-07-14'")
                      .Where("EventDate <= '2013-07-15'")
                      .Where("IsRefresh = 0")
                      .Where("DontCountHits = 0")
                      .Add("strftime('%M', EventTime)")
                      .GroupByAggregate("strftime('%M', EventTime)", "COUNT(*)")
                      .Rename("COUNT(*)", "PageViews")
                      .OrderBy("strftime('%M', EventTime) ASC")
                      .Limit(10)
                      .Select("strftime('%M', EventTime)", "PageViews")
                      .Run();
    const std::vector<std::vector<std::string>> expected_rows = {
        {"00", "6277"},
        {"01", "6176"},
        {"02", "6422"},
        {"03", "6292"},
        {"04", "6138"},
        {"05", "6407"},
        {"06", "6379"},
        {"07", "6453"},
        {"08", "6277"},
        {"09", "6180"},
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
