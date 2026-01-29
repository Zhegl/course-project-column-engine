#include <gtest/gtest.h>
#include "convert.h"
#include "file_writer.h"
#include "file_reader.h"
#include <format/scheme_reader.h>
#include <glog/logging.h>
#include <cstdint>
#include <string>

void Write(std::string path, std::string data) {
    column_engine::FileWriter writer(path);
    writer.Write(data.data(), data.size());
}

bool Equal(column_engine::Scheme a, column_engine::Scheme b) {
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

TEST(SchemeReaderTest, Basic) {
    Write("scheme.csv", " a,int64\nb,string\n");

    auto scheme = column_engine::ReadScheme("scheme.csv");

    ASSERT_EQ(2, scheme.columns.size());
    EXPECT_EQ("a", scheme.columns[0].name);
    EXPECT_EQ("int64", scheme.columns[0].type->GetTypeName());
    EXPECT_EQ("b", scheme.columns[1].name);
    EXPECT_EQ("string", scheme.columns[1].type->GetTypeName());

    Write("scheme.csv",
          " aaaa ,    int64  \n"
          "bbb123,string\n\n\n\n\n"
          "cc,string\n"
          "dd ,string\n"
          "ddd ,int64\n"
          "dddd ,  string");

    scheme = column_engine::ReadScheme("scheme.csv");

    ASSERT_EQ(6, scheme.columns.size());
    EXPECT_EQ("aaaa", scheme.columns[0].name);
    EXPECT_EQ("int64", scheme.columns[0].type->GetTypeName());
    EXPECT_EQ("bbb123", scheme.columns[1].name);
    EXPECT_EQ("string", scheme.columns[1].type->GetTypeName());
    EXPECT_EQ("cc", scheme.columns[2].name);
    EXPECT_EQ("string", scheme.columns[2].type->GetTypeName());
    EXPECT_EQ("dd", scheme.columns[3].name);
    EXPECT_EQ("string", scheme.columns[3].type->GetTypeName());
    EXPECT_EQ("ddd", scheme.columns[4].name);
    EXPECT_EQ("int64", scheme.columns[4].type->GetTypeName());
    EXPECT_EQ("dddd", scheme.columns[5].name);
    EXPECT_EQ("string", scheme.columns[5].type->GetTypeName());
}

TEST(ConvertTest, SimpleConvert) {
    Write("scheme.csv", "a,int64\nb,int64\n");
    Write("input.csv", "1   , 2\n   3,4\n");

    column_engine::ConvertToColumnar("input.csv", "scheme.csv", "out.col");

    column_engine::FileReader reader("out.col");
    EXPECT_EQ(1, reader.Read<int64_t>());
    EXPECT_EQ(3, reader.Read<int64_t>());
    EXPECT_EQ(2, reader.Read<int64_t>());
    EXPECT_EQ(4, reader.Read<int64_t>());

    column_engine::ConvertToCsv("out.col", "scheme_out.csv", "out.csv");
    column_engine::Scheme old_scheme = column_engine::ReadScheme("scheme.csv");
    column_engine::Scheme new_scheme = column_engine::ReadScheme("scheme_out.csv");
    EXPECT_TRUE(Equal(old_scheme, new_scheme));
    column_engine::FileReader reader2("out.csv");
    char a[9];
    reader2.Read(a, 8);
    EXPECT_EQ(std::string("1,2\n3,4\n"), std::string(a));
}

TEST(ConvertTest, QuotedStringsWithSpaces) {
    Write("scheme.csv",
          "id,int64\n"
          "name,string\n"
          "comment,string\n");

    Write("input.csv",
          "1,\"hello world\",\"foo bar\"\n"
          "2,bar,\"baz qux\"\n"
          "3,\" spaced\n   text \",qux\n");

    column_engine::ConvertToColumnar(
        "input.csv",
        "scheme.csv",
        "out.col",
        2
    );

    column_engine::FileReader r("out.col");
    EXPECT_EQ(1, r.Read<int64_t>());
    EXPECT_EQ(2, r.Read<int64_t>());
    EXPECT_NE(3, r.Read<int64_t>());

    column_engine::ConvertToCsv("out.col", "scheme_out.csv", "out.csv");

    column_engine::Scheme old_scheme = column_engine::ReadScheme("scheme.csv");
    column_engine::Scheme new_scheme = column_engine::ReadScheme("scheme_out.csv");
    EXPECT_TRUE(Equal(old_scheme, new_scheme));

    column_engine::FileReader reader2("out.csv");
    char buf[256];
    reader2.Read(buf, sizeof(buf) - 1);

    EXPECT_EQ(
        std::string(
            "1,\"hello world\",\"foo bar\"\n"
            "2,bar,\"baz qux\"\n"
            "3,\" spaced\n   text \",qux\n"
        ),
        std::string(buf)
    );
}



int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;
    int result = RUN_ALL_TESTS();

    google::ShutdownGoogleLogging();
    return result;
}
