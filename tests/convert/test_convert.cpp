#include <gtest/gtest.h>
#include "convert.h"
#include "file_writer.h"
#include "file_reader.h"
#include <format/scheme_reader.h>
#include <glog/logging.h>
#include <cstdint>
#include <string>

TEST(ConvertTest, SchemeReader) {
    {
        column_engine::FileWriter writer("simple_scheme.csv");
        writer.Write("a,int64\nb,int64\nname123,string\nd,int64");
    }
    column_engine::Scheme result = column_engine::ReadScheme("simple_scheme.csv");
    column_engine::Scheme correct;
    EXPECT_EQ("a", result.columns[0].name);
    EXPECT_EQ("int64", result.columns[0].type->GetTypeName());
    EXPECT_EQ("b", result.columns[1].name);
    EXPECT_EQ("int64", result.columns[1].type->GetTypeName());
    EXPECT_EQ("name123", result.columns[2].name);
    EXPECT_EQ("string", result.columns[2].type->GetTypeName());
    EXPECT_EQ("d", result.columns[3].name);
    EXPECT_EQ("int64", result.columns[3].type->GetTypeName());
}


TEST(ConvertTest, SimpleConvert) {
    {
        column_engine::FileWriter writer("simple_scheme.csv");
        writer.Write("a,int64\nb,int64\nname123,string\nd,int64\n");
        column_engine::FileWriter writer2("simple_input.csv");
        writer2.Write("1,2,first,4\n5,1,second,2\n8,17,third,2");
    }
    { column_engine::ConvertToColumnar("simple_input.csv", "simple_scheme.csv", "simple_output.columnar", 2); }
    {
        column_engine::FileReader reader("simple_output.columnar");
        EXPECT_EQ(1, reader.Read<uint64_t>());
    }
    column_engine::ConvertToCsv("simple_output.columnar", "simple_scheme.csv", "simple_csv.csv");

}

TEST(ConvertTest, SimpleConvert2) {
    {
        column_engine::FileWriter writer("simple_scheme.csv");
        std::string scheme = "b,int64\nname123,string\n";
        writer.Write(scheme.data(), scheme.size());

        std::string data = "1,aaaa\n2,bbbb\n3,abcd\n";
        column_engine::FileWriter writer2("simple_input.csv");
        writer2.Write(data.data(), data.size());
    }
    { column_engine::ConvertToColumnar("simple_input.csv", "simple_scheme.csv", "simple_output.columnar"); }
    column_engine::ConvertToCsv("simple_output.columnar", "simple_scheme_out.csv", "simple_csv_out.csv");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;
    int result = RUN_ALL_TESTS();

    google::ShutdownGoogleLogging();
    return result;
}
