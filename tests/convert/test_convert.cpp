#include <gtest/gtest.h>
#include "convert.h"
#include "file_writer.h"
#include "file_reader.h"
#include "scheme_reader.h"
#include <glog/logging.h>
#include <cstdint>

TEST(ConvertTest, SchemeReader) {
    {
        FileWriter writer("simple_scheme.csv");
        writer.Write("a,int64\nb,int64\nname123,string\nd,int64");
    }
    Scheme result = ReadScheme("simple_scheme.csv");
    Scheme correct;
    EXPECT_EQ("a", result.columns[0].name);
    EXPECT_EQ(ColumnTypeName::Int64, result.columns[0].type);
    EXPECT_EQ("b", result.columns[1].name);
    EXPECT_EQ(ColumnTypeName::Int64, result.columns[1].type);
    EXPECT_EQ("name123", result.columns[2].name);
    EXPECT_EQ(ColumnTypeName::String, result.columns[2].type);
    EXPECT_EQ("d", result.columns[3].name);
    EXPECT_EQ(ColumnTypeName::Int64, result.columns[3].type);
}

TEST(ConvertTest, SimpleConvert) {
    {
        FileWriter writer("simple_scheme.csv");
        writer.Write("a,int64\nb,int64\nname123,string\nd,int64\n");
        FileWriter writer2("simple_input.csv");
        writer2.Write("1,2,first,4\n5,1,second,2\n8,17,third,2");
    }
    {
        Convert("simple_input.csv", "simple_scheme.csv", "simple_output.columnar");
    }
    FileReader reader("simple_output.columnar");
    EXPECT_EQ(1, reader.Read<uint64_t>());

}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;
    int result = RUN_ALL_TESTS();

    google::ShutdownGoogleLogging();
    return result;
}
