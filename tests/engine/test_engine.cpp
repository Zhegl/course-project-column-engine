#include <gtest/gtest.h>
#include <convert/convert.h>
#include <engine/engine.h>
#include <glog/logging.h>

TEST(ClickBench1, MainTest) {
    column_engine::ConvertToColumnar("hits_sample.csv", "hits_schema.csv", "col.col", 10);
    
    column_engine::Engine engine("col.col");
    auto result = engine.Api().Where("AdvEngineID <> 0").Count("*").Run();
    
    for (const auto& col : result) {
        for (const auto& val : col) {
           std::cout << val << " ";
        }
        std::cout << "\n";
    }
    EXPECT_TRUE(true);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;
    int result = RUN_ALL_TESTS();
    google::ShutdownGoogleLogging();
    return result;
}