#include <gtest/gtest.h>
#include "convert.h"
#include <glog/logging.h>

TEST(ConvertTest, TestTest) {
    EXPECT_TRUE(true);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;

    int result = RUN_ALL_TESTS();

    google::ShutdownGoogleLogging();
    return result;
}
