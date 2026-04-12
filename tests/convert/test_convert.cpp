// Этот файл был создан с частичным использованием генеративных моделей
#include <gtest/gtest.h>
#include <format/meta_reader.h>
#include "convert.h"
#include "file_writer.h"
#include "file_reader.h"
#include <format/schema_reader.h>
#include <glog/logging.h>
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <random>

void Write(const std::string& path, const std::string& data) {
    column_engine::FileWriter writer(path);
    writer.Write(data.data(), data.size());
}

bool Equal(const column_engine::Schema& a, const column_engine::Schema& b) {
    if (a.columns.size() != b.columns.size()) {
        return false;
    }
    for (size_t i = 0; i < a.columns.size(); ++i) {
        if (a.columns[i].name != b.columns[i].name ||
            a.columns[i].type->GetTypeName() != b.columns[i].type->GetTypeName()) {
            return false;
        }
    }
    return true;
}

std::string ReadFileToString(const std::string& path) {
    column_engine::FileReader reader(path);
    std::string result;
    char c;
    while (reader.Read(&c, 1)) {
        result += c;
    }
    return result;
}

std::string GenerateLargeCSV(int num_rows, int num_cols) {
    std::string csv;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int64_t> dist(-1000000, 1000000);
    for (int row = 0; row < num_rows; ++row) {
        for (int col = 0; col < num_cols; ++col) {
            if (col > 0) {
                csv += ",";
            }
            csv += std::to_string(dist(gen));
        }
        csv += "\n";
    }
    return csv;
}

std::string GenerateSchema(int num_cols, const std::string& type) {
    std::string schema;
    for (int col = 0; col < num_cols; ++col) {
        schema += "col" + std::to_string(col) + "," + type + "\n";
    }
    return schema;
}

static const std::vector<int64_t>& AsInt64(const column_engine::ColumnData& col) {
    return std::get<std::vector<int64_t>>(col);
}

static const std::vector<std::string>& AsString(const column_engine::ColumnData& col) {
    return std::get<std::vector<std::string>>(col);
}
*/
TEST(ConvertTest, NegativeNumbers) {
    Write("schema.csv", "a,int64\n");
    Write("input.csv", "-123\n-456\n");
    column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col");
    column_engine::FileReader r("out.col");
    EXPECT_EQ(-123, r.Read<int64_t>());
    EXPECT_EQ(-456, r.Read<int64_t>());
}

TEST(ConvertTest, LargeNumbers) {
    Write("schema.csv", "a,int64\n");
    Write("input.csv", "9223372036854775807\n-9223372036854775808\n");
    column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col");
    column_engine::FileReader r("out.col");
    EXPECT_EQ(9223372036854775807LL, r.Read<int64_t>());
    EXPECT_EQ(static_cast<int64_t>(-9223372036854775807LL - 1), r.Read<int64_t>());
}

// TODO
/*
TEST(ConvertTest, OverflowNumbers) {
    Write("schema.csv", "a,int64\n");
    Write("input.csv", "9223372036854775808\n");  // Overflow для int64
    EXPECT_THROW({
        column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col");
    }, std::overflow_error);
}*/

TEST(ConvertTest, EmptyInput) {
    Write("schema.csv", "a,int64\n");
    Write("input.csv", "");
    column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col");
    column_engine::FileReader r("out.col");
}

TEST(ConvertTest, LargeFileStress) {
    const int num_rows = 10000;
    const int num_cols = 5;
    Write("schema.csv", GenerateSchema(num_cols, "int64"));
    std::string large_csv = GenerateLargeCSV(num_rows, num_cols);
    Write("input.csv", large_csv);

// -------------------- Тесты --------------------
TEST(SchemaReaderTest, Basic) {
    Write("schema.csv", " a,int64\nb,string\n");
    auto schema = column_engine::ReadSchema("schema.csv");
    ASSERT_EQ(2, schema.columns.size());
    EXPECT_EQ("a", schema.columns[0].name);
    EXPECT_EQ("int64", schema.columns[0].type->GetTypeName());
    EXPECT_EQ("b", schema.columns[1].name);
    EXPECT_EQ("string", schema.columns[1].type->GetTypeName());
    Write("schema.csv",
        " aaaa , int64 \n"
        "bbb123,string\n\n\n\n\n"
        "cc,string\n"
        "dd ,string\n"
        "ddd ,int64\n"
        "dddd , string");
    schema = column_engine::ReadSchema("schema.csv");
    ASSERT_EQ(6, schema.columns.size());
    EXPECT_EQ("aaaa", schema.columns[0].name);
    EXPECT_EQ("int64", schema.columns[0].type->GetTypeName());
    EXPECT_EQ("bbb123", schema.columns[1].name);
    EXPECT_EQ("string", schema.columns[1].type->GetTypeName());
    EXPECT_EQ("cc", schema.columns[2].name);
    EXPECT_EQ("string", schema.columns[2].type->GetTypeName());
    EXPECT_EQ("dd", schema.columns[3].name);
    EXPECT_EQ("string", schema.columns[3].type->GetTypeName());
    EXPECT_EQ("ddd", schema.columns[4].name);
    EXPECT_EQ("int64", schema.columns[4].type->GetTypeName());
    EXPECT_EQ("dddd", schema.columns[5].name);
    EXPECT_EQ("string", schema.columns[5].type->GetTypeName());
}

// -------------------- ConvertTest --------------------
TEST(ConvertTest, SimpleConvert) {
    Write("schema.csv", "a,int64\nb,int64\n");
    Write("input.csv", "1,2\n3,4\n");
    column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col");
    auto [batch_meta, schema] = column_engine::GetMeta("out.col");
    column_engine::FileReader reader("out.col");
    std::vector<column_engine::ColumnData> columns(schema.columns.size());
    size_t batch_index = 0;
    for (size_t col = 0; col < schema.columns.size(); ++col) {
        columns[col] = schema.columns[col].type->GetBatch(batch_meta[batch_index].size, reader);
        ++batch_index;
    }
    EXPECT_EQ(1, AsInt64(columns[0])[0]);
    EXPECT_EQ(3, AsInt64(columns[0])[1]);
    EXPECT_EQ(2, AsInt64(columns[1])[0]);
    EXPECT_EQ(4, AsInt64(columns[1])[1]);
    column_engine::ConvertToCsv("out.col", "schema_out.csv", "out.csv");
    std::string out_content = ReadFileToString("out.csv");
    EXPECT_EQ("1,2\n3,4\n", out_content);
}

TEST(ConvertTest, NegativeNumbers) {
    Write("schema.csv", "a,int64\n");
    Write("input.csv", "-123\n-456\n");
    column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col");
    auto [batch_meta, schema] = column_engine::GetMeta("out.col");
    column_engine::FileReader reader("out.col");
    auto batch = AsInt64(schema.columns[0].type->GetBatch(batch_meta[0].size, reader));
    EXPECT_EQ(-123, batch[0]);
    EXPECT_EQ(-456, batch[1]);
}

TEST(ConvertTest, LargeNumbers) {
    Write("schema.csv", "a,int64\n");
    Write("input.csv", "9223372036854775807\n-9223372036854775808\n");
    column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col");
    auto [batch_meta, schema] = column_engine::GetMeta("out.col");
    column_engine::FileReader reader("out.col");
    auto batch = AsInt64(schema.columns[0].type->GetBatch(batch_meta[0].size, reader));
    EXPECT_EQ(9223372036854775807LL, batch[0]);
    EXPECT_EQ(static_cast<int64_t>(-9223372036854775807LL - 1), batch[1]);
}

TEST(ConvertTest, SmallBatchSize) {
    Write("schema.csv", "a,int64\nb,int64\n");
    Write("input.csv", "1,2\n3,4\n5,6\n");
    column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col", 1);
    auto [batch_meta, schema] = column_engine::GetMeta("out.col");
    column_engine::FileReader reader("out.col");
    size_t num_cols = schema.columns.size();
    size_t num_batches = batch_meta.size();
    std::vector<std::vector<int64_t>> columns(num_cols);
    size_t batch = 0;
    while (batch < num_batches) {
        for (size_t col = 0; col < num_cols; ++col) {
            auto vals = AsInt64(schema.columns[col].type->GetBatch(batch_meta[batch].size, reader));
            for (auto v : vals) {
                columns[col].push_back(v);
            }
            ++batch;
        }
    }
    std::vector<int64_t> expected_a = {1, 3, 5};
    std::vector<int64_t> expected_b = {2, 4, 6};
    for (size_t i = 0; i < expected_a.size(); ++i) {
        EXPECT_EQ(columns[0][i], expected_a[i]);
        EXPECT_EQ(columns[1][i], expected_b[i]);
    }
}

TEST(ConvertTest, MixedTypesLarge) {
    Write("schema.csv", "id,int64\nname,string\nvalue,int64\n");
    std::string csv;
    for (int i = 0; i < 1000; ++i) {
        csv += std::to_string(i) + ",\"name " + std::to_string(i) + "\"," + std::to_string(i * 2) + "\n";
    }
    Write("input.csv", csv);
    column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col", 500);
    auto [batch_meta, schema] = column_engine::GetMeta("out.col");
    column_engine::FileReader reader("out.col");
    auto batch_id0    = AsInt64(schema.columns[0].type->GetBatch(batch_meta[0].size, reader));
    schema.columns[1].type->GetBatch(batch_meta[1].size, reader); // name_0
    auto batch_value0 = AsInt64(schema.columns[2].type->GetBatch(batch_meta[2].size, reader));
    auto batch_id1    = AsInt64(schema.columns[0].type->GetBatch(batch_meta[3].size, reader));
    schema.columns[1].type->GetBatch(batch_meta[4].size, reader); // name_1
    auto batch_value1 = AsInt64(schema.columns[2].type->GetBatch(batch_meta[5].size, reader));
    for (int i = 0; i < 500; ++i) {
        EXPECT_EQ(i,       batch_id0[i]);
        EXPECT_EQ(i * 2,   batch_value0[i]);
    }
    for (int i = 0; i < 500; ++i) {
        EXPECT_EQ(i + 500,       batch_id1[i]);
        EXPECT_EQ((i + 500) * 2, batch_value1[i]);
    }
}

TEST(ConvertTest, NewlinesInQuotes) {
    Write("schema.csv", "a,string\nb,string\n");
    Write("input.csv", "\"line1\nline2\",\"foo\nbar\"\n");
    column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col");
    column_engine::ConvertToCsv("out.col", "schema_out.csv", "out.csv");
    std::string out_content = ReadFileToString("out.csv");
    EXPECT_EQ("\"line1\nline2\",\"foo\nbar\"\n", out_content);
}

TEST(ConvertTest, LargeFileStress) {
    const int num_rows = 10000;
    const int num_cols = 5;
    Write("schema.csv", GenerateSchema(num_cols, "int64"));
    std::string large_csv = GenerateLargeCSV(num_rows, num_cols);
    Write("input.csv", large_csv);
    column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col", 1000);
    column_engine::ConvertToCsv("out.col", "schema_out.csv", "out.csv");
    std::string out_content = ReadFileToString("out.csv");
    EXPECT_EQ(large_csv, out_content);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;
    int result = RUN_ALL_TESTS();
    google::ShutdownGoogleLogging();
    return result;
}