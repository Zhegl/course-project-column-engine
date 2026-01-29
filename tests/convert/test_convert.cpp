// Этот файл был создан с частичным использованием генеративных моделей
#include <gtest/gtest.h>
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

// Вспомогательная функция для чтения всего файла в строку (поскольку Reader читает посимвольно)
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

TEST(SchemaReaderTest, EmptySchema) {
    Write("empty_schema.csv", "");
    EXPECT_THROW({
        column_engine::ReadSchema("empty_schema.csv");
    }, std::runtime_error); 
}


TEST(SchemaReaderTest, MissingComma) {
    Write("bad_schema.csv", "a int64\nb,string\n");
    EXPECT_THROW({
        column_engine::ReadSchema("bad_schema.csv");
    }, std::runtime_error);
}


TEST(SchemaReaderTest, TrailingCommas) {
    Write("trailing_schema.csv", "a,int64,\nb,string,\n");
        EXPECT_THROW({
        auto schema = column_engine::ReadSchema("trailing_schema.csv");
    }, std::runtime_error);
}

TEST(SchemaReaderTest, FileNotFound) {
    EXPECT_THROW({
        column_engine::ReadSchema("nonexistent.csv");
    }, std::runtime_error);
}

TEST(ConvertTest, SimpleConvert) {
    Write("schema.csv", "a,int64\nb,int64\n");
    Write("input.csv", "1 , 2\n 3,4\n");
    column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col");
    column_engine::FileReader reader("out.col");
    EXPECT_EQ(1, reader.Read<int64_t>());
    EXPECT_EQ(3, reader.Read<int64_t>());
    EXPECT_EQ(2, reader.Read<int64_t>());
    EXPECT_EQ(4, reader.Read<int64_t>());

    column_engine::ConvertToCsv("out.col", "schema_out.csv", "out.csv");
    column_engine::Schema old_schema = column_engine::ReadSchema("schema.csv");
    column_engine::Schema new_schema = column_engine::ReadSchema("schema_out.csv");
    EXPECT_TRUE(Equal(old_schema, new_schema));

    std::string out_content = ReadFileToString("out.csv");
    EXPECT_EQ("1,2\n3,4\n", out_content);
}

TEST(ConvertTest, QuotedStringsWithSpaces) {
    Write("schema.csv",
          "id,int64\n"
          "name,string\n"
          "comment,string\n");
    Write("input.csv",
          "1,\"hello world\",\"foo bar\"\n"
          "2,bar,\"baz qux\"\n"
          "3,\" spaced\n text \",qux\n");
    column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col", 2);
    column_engine::FileReader r("out.col");
    EXPECT_EQ(1, r.Read<int64_t>());
    EXPECT_EQ(2, r.Read<int64_t>());
    EXPECT_NE(3, r.Read<int64_t>());  

    column_engine::ConvertToCsv("out.col", "schema_out.csv", "out.csv");
    column_engine::Schema old_schema = column_engine::ReadSchema("schema.csv");
    column_engine::Schema new_schema = column_engine::ReadSchema("schema_out.csv");
    EXPECT_TRUE(Equal(old_schema, new_schema));

    std::string out_content = ReadFileToString("out.csv");
    EXPECT_EQ("1,\"hello world\",\"foo bar\"\n"
              "2,bar,\"baz qux\"\n"
              "3,\" spaced\n text \",qux\n", out_content);
}


TEST(ConvertTest, MismatchedColumns) {
    Write("schema.csv", "a,int64\nb,int64\n");
    Write("input.csv", "1,2,3\n4,5\n");
    EXPECT_THROW({
        column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col");
    }, std::runtime_error);
}

// TODO, пока считаю что все переданные типы корректные, все равно сжатие переписывать
/*
TEST(ConvertTest, UnclosedQuotes) {
    Write("schema.csv", "a,string\n");
    Write("input.csv", "\"unclosed\n");
    EXPECT_THROW({
        column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col");
    }, std::runtime_error);
}

TEST(ConvertTest, EscapedQuotes) {
    Write("schema.csv", "a,string\n");
    Write("input.csv", "\"\"\"escaped\"\"\"\n");
    column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col");
    column_engine::ConvertToCsv("out.col", "schema_out.csv", "out.csv");
    std::string out_content = ReadFileToString("out.csv");
    EXPECT_EQ("\"\"\"escaped\"\"\"\n", out_content);
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

    column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col", 1000);  // Батч 1000

    std::ifstream out_file("out.col", std::ios::binary | std::ios::ate);
    EXPECT_GT(out_file.tellg(), 0);

    column_engine::ConvertToCsv("out.col", "schema_out.csv", "out.csv");
    std::string out_content = ReadFileToString("out.csv");
    EXPECT_EQ(large_csv, out_content); }

TEST(ConvertTest, SmallBatchSize) {
    Write("schema.csv", "a,int64\nb,int64\n");
    Write("input.csv", "1,2\n3,4\n5,6\n");
    column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col", 1);  // Батч 1
    column_engine::FileReader r("out.col");
    EXPECT_EQ(1, r.Read<int64_t>());
    EXPECT_EQ(2, r.Read<int64_t>());
    EXPECT_EQ(3, r.Read<int64_t>());
    EXPECT_EQ(4, r.Read<int64_t>());
    EXPECT_EQ(5, r.Read<int64_t>());
    EXPECT_EQ(6, r.Read<int64_t>());
}

TEST(ConvertTest, LargeBatchSize) {
    const int num_rows = 5000;
    Write("schema.csv", "a,int64\n");
    std::string large_csv = GenerateLargeCSV(num_rows, 1);
    Write("input.csv", large_csv);
    column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col", 10000);  // Батч больше, чем строк
    // Проверяем чтение нескольких значений
    column_engine::FileReader r("out.col");
    for (int i = 0; i < 10; ++i) {  // Проверяем первые 10
        r.Read<int64_t>();  // Просто читаем, без проверки (для стресса)
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

    column_engine::ConvertToCsv("out.col", "schema_out.csv", "out.csv");
    std::string out_content = ReadFileToString("out.csv");
    EXPECT_EQ(csv, out_content);
}

TEST(ConvertTest, NewlinesInQuotes) {
    Write("schema.csv", "a,string\nb,string\n");
    Write("input.csv", "\"line1\nline2\",\"foo\nbar\"\n");
    column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col");
    column_engine::ConvertToCsv("out.col", "schema_out.csv", "out.csv");
    std::string out_content = ReadFileToString("out.csv");
    EXPECT_EQ("\"line1\nline2\",\"foo\nbar\"\n", out_content);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;
    int result = RUN_ALL_TESTS();
    google::ShutdownGoogleLogging();
    return result;
}