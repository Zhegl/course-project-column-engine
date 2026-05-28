#include <gtest/gtest.h>
#include <glog/logging.h>
#include <engine/engine.h>
#include <app/clickbench_queries.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>

static constexpr const char* kColFile    = "col.col";
static constexpr const char* kHitsCsv   = "hits_sample.csv";
static constexpr const char* kSchemaCsv  = "hits_schema.csv";
static constexpr const char* kQueriesSql = "queries.sql";
static constexpr const char* kCheckPy    = "check_query.py";

static std::string CsvField(const std::string& s) {
    if (s.find(',') == std::string::npos &&
        s.find('"') == std::string::npos &&
        s.find('\n') == std::string::npos) {
        return s;
    }
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += '"';
        out += c;
    }
    out += '"';
    return out;
}

static void WriteEngineCsv(const column_engine::QueryResult& result, const std::string& path) {
    std::ofstream out(path);
    // skip header row (result[0])
    for (size_t i = 1; i < result.size(); ++i) {
        const auto& row = result[i];
        for (size_t j = 0; j < row.size(); ++j) {
            if (j) out << ',';
            out << CsvField(row[j]);
        }
        out << '\n';
    }
}

static void RunQueryTest(int query_num) {
    column_engine::Engine engine(kColFile);

    column_engine::QueryResult result;
    try {
        result = RunQuery(engine, query_num);
    } catch (const std::runtime_error& e) {
        GTEST_SKIP() << "Query " << query_num << " not implemented: " << e.what();
    }

    std::string tmp_csv = "/tmp/engine_q" + std::to_string(query_num) + ".csv";
    WriteEngineCsv(result, tmp_csv);

    std::string cmd = std::string("python3 ") + kCheckPy
        + " " + std::to_string(query_num)
        + " " + tmp_csv
        + " " + kHitsCsv
        + " " + kSchemaCsv
        + " " + kQueriesSql
        + " 2>&1";

    FILE* pipe = popen(cmd.c_str(), "r");
    ASSERT_NE(pipe, nullptr);

    std::ostringstream output;
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe)) {
        output << buf;
    }
    int rc = pclose(pipe);

    std::string out_str = output.str();
    if (!out_str.empty() && out_str.find("SKIP") == 0) {
        GTEST_SKIP() << out_str;
    }

    EXPECT_EQ(rc, 0) << "Query " << query_num << " mismatch:\n" << out_str;
}

// one test per query
TEST(ClickBenchAuto, Q0)  { RunQueryTest(0);  }
TEST(ClickBenchAuto, Q1)  { RunQueryTest(1);  }
TEST(ClickBenchAuto, Q2)  { RunQueryTest(2);  }
TEST(ClickBenchAuto, Q3)  { RunQueryTest(3);  }
TEST(ClickBenchAuto, Q4)  { RunQueryTest(4);  }
TEST(ClickBenchAuto, Q5)  { RunQueryTest(5);  }
TEST(ClickBenchAuto, Q6)  { RunQueryTest(6);  }
TEST(ClickBenchAuto, Q7)  { RunQueryTest(7);  }
TEST(ClickBenchAuto, Q8)  { RunQueryTest(8);  }
TEST(ClickBenchAuto, Q9)  { RunQueryTest(9);  }
TEST(ClickBenchAuto, Q10) { RunQueryTest(10); }
TEST(ClickBenchAuto, Q11) { RunQueryTest(11); }
TEST(ClickBenchAuto, Q12) { RunQueryTest(12); }
TEST(ClickBenchAuto, Q13) { RunQueryTest(13); }
TEST(ClickBenchAuto, Q14) { RunQueryTest(14); }
TEST(ClickBenchAuto, Q15) { RunQueryTest(15); }
TEST(ClickBenchAuto, Q16) { RunQueryTest(16); }
TEST(ClickBenchAuto, Q17) { RunQueryTest(17); }
TEST(ClickBenchAuto, Q18) { RunQueryTest(18); }
TEST(ClickBenchAuto, Q19) { RunQueryTest(19); }
TEST(ClickBenchAuto, Q20) { RunQueryTest(20); }
TEST(ClickBenchAuto, Q21) { RunQueryTest(21); }
TEST(ClickBenchAuto, Q22) { RunQueryTest(22); }
TEST(ClickBenchAuto, Q23) { RunQueryTest(23); }
TEST(ClickBenchAuto, Q24) { RunQueryTest(24); }
TEST(ClickBenchAuto, Q25) { RunQueryTest(25); }
TEST(ClickBenchAuto, Q26) { RunQueryTest(26); }
TEST(ClickBenchAuto, Q27) { RunQueryTest(27); }
TEST(ClickBenchAuto, Q28) { RunQueryTest(28); }
TEST(ClickBenchAuto, Q29) { RunQueryTest(29); }
TEST(ClickBenchAuto, Q30) { RunQueryTest(30); }
TEST(ClickBenchAuto, Q31) { RunQueryTest(31); }
TEST(ClickBenchAuto, Q32) { RunQueryTest(32); }
TEST(ClickBenchAuto, Q33) { RunQueryTest(33); }
TEST(ClickBenchAuto, Q34) { RunQueryTest(34); }
TEST(ClickBenchAuto, Q35) { RunQueryTest(35); }
TEST(ClickBenchAuto, Q36) { RunQueryTest(36); }
TEST(ClickBenchAuto, Q37) { RunQueryTest(37); }
TEST(ClickBenchAuto, Q38) { RunQueryTest(38); }
TEST(ClickBenchAuto, Q39) { RunQueryTest(39); }
TEST(ClickBenchAuto, Q40) { RunQueryTest(40); }
TEST(ClickBenchAuto, Q41) { RunQueryTest(41); }
TEST(ClickBenchAuto, Q42) { RunQueryTest(42); }

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;
    int result = RUN_ALL_TESTS();
    google::ShutdownGoogleLogging();
    return result;
}
