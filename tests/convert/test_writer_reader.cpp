#include <gtest/gtest.h>
#include "file_writer.h"
#include "file_reader.h"
#include <glog/logging.h>
#include <cstdint>

TEST(WriterReaderTest, Simple) {
    uint64_t val = 42;

    {
        column_engine::FileWriter writer("test.columnar");
        writer.Write(val);
    }
    
    column_engine::FileReader reader("test.columnar");
    uint64_t val_read = reader.Read<uint64_t>();

    EXPECT_EQ(val, val_read);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;

    int result = RUN_ALL_TESTS();

    google::ShutdownGoogleLogging();
    return result;
}
