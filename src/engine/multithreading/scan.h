#pragma once
#include "../batch.h"
#include <types/types.h>
#include <io/file_reader.h>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace column_engine::internal {

class ScanWorkersPool {
public:
    ScanWorkersPool(size_t num_workers, const std::string& path, Schema schema,
                    std::vector<BatchMetaData> batch_meta, std::vector<size_t> columns);
    ~ScanWorkersPool();

    std::optional<EngineBatch> GetNext();

private:
    struct alignas(64) Worker {
        std::mutex mu;
        std::condition_variable cv;
        std::optional<EngineBatch> batch;
        bool ready = false;
    };

    void WorkerLoop(size_t worker_id);

    std::string path_;
    FileReader reader_;
    Schema schema_;
    std::vector<BatchMetaData> batch_meta_;
    std::vector<size_t> columns_;
    size_t num_row_groups_;
    size_t num_workers_;

    std::atomic<size_t> next_rg_{0};
    size_t consume_rg_{0};

    std::vector<Worker> workers_;
    std::vector<std::thread> threads_;
};

}  // namespace column_engine::internal
