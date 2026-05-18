#include <api/columnar_engine.h>
#include <gtest/gtest.h>
#include <convert/convert.h>
#include <glog/logging.h>
#include <io/file_writer.h>

namespace {

void Write(const std::string& path, const std::string& data) {
    column_engine::FileWriter writer(path);
    writer.Write(data.data(), data.size());
}

// Same dataset as EngineTest: 100 rows, 4 columns, batch_size=8 → many row groups
class MtScanTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        Write("mt_schema.csv", "id,int64\nscore,int64\nname,string\ngroup,string\n");
        std::string csv;
        for (int i = 1; i <= 100; ++i) {
            csv += std::to_string(i) + ","
                 + std::to_string(i * 10) + ","
                 + "item_" + std::to_string(i) + ","
                 + (i <= 50 ? "A" : "B") + "\n";
        }
        Write("mt_input.csv", csv);
        // small batch_size so we get many row groups and actually exercise parallelism
        column_engine::ConvertToColumnar("mt_input.csv", "mt_schema.csv", "mt_test.col", 8);
    }
};

// Helper: run query single-threaded and multi-threaded, compare results
static void ExpectSameAsSingleThread(
    std::function<column_engine::QueryResult(column_engine::Engine&)> query,
    size_t n_workers) {

    column_engine::Engine st("mt_test.col");
    column_engine::Engine mt("mt_test.col", 4);

    auto expected = query(st);
    auto actual   = query(mt);

    ASSERT_EQ(actual.size(), expected.size()) << "row count mismatch";
    // header must match exactly
    EXPECT_EQ(actual[0], expected[0]);

    // for data rows order may differ between single/multi — compare as sorted sets
    std::vector<std::vector<std::string>> exp_rows(expected.begin() + 1, expected.end());
    std::vector<std::vector<std::string>> act_rows(actual.begin() + 1, actual.end());
    std::sort(exp_rows.begin(), exp_rows.end());
    std::sort(act_rows.begin(), act_rows.end());
    EXPECT_EQ(act_rows, exp_rows);
}

TEST_F(MtScanTest, SelectAll) {
    column_engine::Engine st("mt_test.col");
    column_engine::Engine mt("mt_test.col", 4);
    auto expected = st.Api().Select("id", "score", "name", "group").Run();
    auto actual   = mt.Api().Select("id", "score", "name", "group").Run();
    ASSERT_EQ(actual.size(), expected.size());
    EXPECT_EQ(actual[0], expected[0]);
    std::vector<std::vector<std::string>> er(expected.begin()+1, expected.end());
    std::vector<std::vector<std::string>> ar(actual.begin()+1, actual.end());
    std::sort(er.begin(), er.end());
    std::sort(ar.begin(), ar.end());
    EXPECT_EQ(ar, er);
}

TEST_F(MtScanTest, SelectSubset) {
    column_engine::Engine st("mt_test.col");
    column_engine::Engine mt("mt_test.col", 4);
    auto expected = st.Api().Select("name", "score").Run();
    auto actual   = mt.Api().Select("name", "score").Run();
    ASSERT_EQ(actual.size(), expected.size());
    std::vector<std::vector<std::string>> er(expected.begin()+1, expected.end());
    std::vector<std::vector<std::string>> ar(actual.begin()+1, actual.end());
    std::sort(er.begin(), er.end());
    std::sort(ar.begin(), ar.end());
    EXPECT_EQ(ar, er);
}

TEST_F(MtScanTest, CountStar) {
    column_engine::Engine st("mt_test.col");
    column_engine::Engine mt("mt_test.col", 4);
    auto expected = st.Api().Count("*").Run();
    auto actual   = mt.Api().Count("*").Run();
    ASSERT_EQ(actual.size(), 2u);
    EXPECT_EQ(actual[1][0], "100");
    EXPECT_EQ(actual[1][0], expected[1][0]);
}

TEST_F(MtScanTest, SumScore) {
    column_engine::Engine st("mt_test.col");
    column_engine::Engine mt("mt_test.col", 4);
    auto expected = st.Api().Aggregate("SUM(score)").Select("SUM(score)").Run();
    auto actual   = mt.Api().Aggregate("SUM(score)").Select("SUM(score)").Run();
    // SUM(10+20+...+1000) = 10*5050 = 50500
    ASSERT_EQ(actual.size(), 2u);
    EXPECT_EQ(actual[1][0], "50500");
    EXPECT_EQ(actual[1][0], expected[1][0]);
}

TEST_F(MtScanTest, WhereFilter) {
    column_engine::Engine st("mt_test.col");
    column_engine::Engine mt("mt_test.col", 4);
    auto expected = st.Api().Where("group = 'A'").Select("id").Run();
    auto actual   = mt.Api().Where("group = 'A'").Select("id").Run();
    ASSERT_EQ(actual.size(), expected.size());  // 51 rows (header + 50)
    std::vector<std::vector<std::string>> er(expected.begin()+1, expected.end());
    std::vector<std::vector<std::string>> ar(actual.begin()+1, actual.end());
    std::sort(er.begin(), er.end());
    std::sort(ar.begin(), ar.end());
    EXPECT_EQ(ar, er);
}

TEST_F(MtScanTest, GroupBySum) {
    column_engine::Engine st("mt_test.col");
    column_engine::Engine mt("mt_test.col", 4);
    auto expected = st.Api().GroupByAggregate("group", "SUM(score)").Select("group", "SUM(score)").Run();
    auto actual   = mt.Api().GroupByAggregate("group", "SUM(score)").Select("group", "SUM(score)").Run();
    ASSERT_EQ(actual.size(), 3u);
    std::map<std::string, std::string> sums;
    for (size_t i = 1; i < actual.size(); ++i) sums[actual[i][0]] = actual[i][1];
    // A: SUM(10..500) = 10*(1+2+...+50) = 10*1275 = 12750
    // B: SUM(510..1000) = 10*(51+...+100) = 10*3775 = 37750
    EXPECT_EQ(sums["A"], "12750");
    EXPECT_EQ(sums["B"], "37750");
    std::map<std::string, std::string> exp_sums;
    for (size_t i = 1; i < expected.size(); ++i) exp_sums[expected[i][0]] = expected[i][1];
    EXPECT_EQ(sums, exp_sums);
}

TEST_F(MtScanTest, WorkerCountOne) {
    column_engine::Engine st("mt_test.col");
    column_engine::Engine mt("mt_test.col", 1);
    auto expected = st.Api().Aggregate("SUM(score)").Select("SUM(score)").Run();
    auto actual   = mt.Api().Aggregate("SUM(score)").Select("SUM(score)").Run();
    EXPECT_EQ(actual[1][0], expected[1][0]);
}

TEST_F(MtScanTest, WorkerCountMatchesCPU) {
    column_engine::Engine st("mt_test.col");
    size_t nw = std::max(1u, std::thread::hardware_concurrency());
    column_engine::Engine mt("mt_test.col", nw);
    auto expected = st.Api().Aggregate("COUNT(*)").Select("COUNT(*)").Run();
    auto actual   = mt.Api().Aggregate("COUNT(*)").Select("COUNT(*)").Run();
    EXPECT_EQ(actual[1][0], "100");
    EXPECT_EQ(actual[1][0], expected[1][0]);
}

}  // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;
    return RUN_ALL_TESTS();
}
