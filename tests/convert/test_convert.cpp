#include <gtest/gtest.h>
#include "convert.h"
#include "file_writer.h"
#include "file_reader.h"
#include <format/schema_reader.h>
#include <glog/logging.h>
#include <cstdint>
#include <string>

void Write(std::string path, std::string data) {
    column_engine::FileWriter writer(path);
    writer.Write(data.data(), data.size());
}

bool Equal(column_engine::Schema a, column_engine::Schema b) {
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

TEST(SchemaReaderTest, Basic) {
    Write("schema.csv", " a,int64\nb,string\n");

    auto schema = column_engine::ReadSchema("schema.csv");

    ASSERT_EQ(2, schema.columns.size());
    EXPECT_EQ("a", schema.columns[0].name);
    EXPECT_EQ("int64", schema.columns[0].type->GetTypeName());
    EXPECT_EQ("b", schema.columns[1].name);
    EXPECT_EQ("string", schema.columns[1].type->GetTypeName());

    Write("schema.csv",
          " aaaa ,    int64  \n"
          "bbb123,string\n\n\n\n\n"
          "cc,string\n"
          "dd ,string\n"
          "ddd ,int64\n"
          "dddd ,  string");

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

TEST(ConvertTest, SimpleConvert) {
    Write("schema.csv", "a,int64\nb,int64\n");
    Write("input.csv", "1   , 2\n   3,4\n");

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
    column_engine::FileReader reader2("out.csv");
    char a[9];
    reader2.Read(a, 8);
    EXPECT_EQ(std::string("1,2\n3,4\n"), std::string(a));
}

TEST(ConvertTest, QuotedStringsWithSpaces) {
    Write("schema.csv",
          "id,int64\n"
          "name,string\n"
          "comment,string\n");

    Write("input.csv",
          "1,\"hello world\",\"foo bar\"\n"
          "2,bar,\"baz qux\"\n"
          "3,\" spaced\n   text \",qux\n");

    column_engine::ConvertToColumnar("input.csv", "schema.csv", "out.col", 2);

    column_engine::FileReader r("out.col");
    EXPECT_EQ(1, r.Read<int64_t>());
    EXPECT_EQ(2, r.Read<int64_t>());
    EXPECT_NE(3, r.Read<int64_t>());

    column_engine::ConvertToCsv("out.col", "schema_out.csv", "out.csv");

    column_engine::Schema old_schema = column_engine::ReadSchema("schema.csv");
    column_engine::Schema new_schema = column_engine::ReadSchema("schema_out.csv");
    EXPECT_TRUE(Equal(old_schema, new_schema));

    column_engine::FileReader reader2("out.csv");
    char buf[256];
    reader2.Read(buf, sizeof(buf) - 1);

    EXPECT_EQ(std::string("1,\"hello world\",\"foo bar\"\n"
                          "2,bar,\"baz qux\"\n"
                          "3,\" spaced\n   text \",qux\n"),
              std::string(buf));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;
    int result = RUN_ALL_TESTS();

    google::ShutdownGoogleLogging();
    return result;
}
