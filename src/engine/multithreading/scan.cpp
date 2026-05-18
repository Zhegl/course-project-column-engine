#include "scan.h"
#include "column_utils.h"

namespace column_engine::internal {

ScanWorkersPool::ScanWorkersPool(size_t num_workers, const std::string& path, Schema schema,
                                 std::vector<BatchMetaData> batch_meta,
                                 std::vector<size_t> columns)
    : path_(path),
      reader_(path),
      schema_(std::move(schema)),
      batch_meta_(std::move(batch_meta)),
      columns_(std::move(columns)),
      num_row_groups_(0),
      num_workers_(num_workers),
      workers_(num_workers) {
    num_row_groups_ = batch_meta_.size() / schema_.columns.size();
    threads_.reserve(num_workers);
    for (size_t i = 0; i < num_workers; ++i) {
        threads_.emplace_back(&ScanWorkersPool::WorkerLoop, this, i);
    }
}

ScanWorkersPool::~ScanWorkersPool() {
    while (consume_rg_ < num_row_groups_) {
        auto& worker = workers_[consume_rg_ % num_workers_];
        std::unique_lock lock(worker.mu);
        worker.cv.wait(lock, [&] { return worker.ready; });
        worker.ready = false;
        worker.batch.reset();
        worker.cv.notify_one();
        ++consume_rg_;
    }
    for (auto& t : threads_) {
        t.join();
    }
}

void ScanWorkersPool::WorkerLoop(size_t worker_id) {
    FileReaderView reader(reader_);
    size_t cols = schema_.columns.size();

    while (true) {
        size_t rg = next_rg_.fetch_add(1, std::memory_order_relaxed);
        if (rg >= num_row_groups_) {
            break;
        }
        EngineBatch result;
        for (size_t col : columns_) {
            const auto& meta = batch_meta_[rg * cols + col];
            reader.Jump(static_cast<int64_t>(meta.offset) -
                        static_cast<int64_t>(reader.GetPos()));
            result.names.push_back(schema_.columns[col].name);
            ColumnData col_data = schema_.columns[col].type->GetBatch(meta.size, reader);
            if (auto* sv = std::get_if<std::vector<std::string_view>>(&col_data)) {
                std::vector<std::string> owned;
                owned.reserve(sv->size());
                for (auto s : *sv) owned.emplace_back(s);
                result.columns.emplace_back(std::move(owned));
            } else {
                result.columns.emplace_back(std::move(col_data));
            }
        }
        size_t num_rows = GetColumnSize(result.columns[0]);
        result.selection.resize(num_rows);
        for (RowIndex i = 0; i < static_cast<RowIndex>(num_rows); ++i) {
            result.selection[i] = i;
        }

        auto& worker = workers_[rg % num_workers_];
        std::unique_lock lock(worker.mu);
        worker.cv.wait(lock, [&] { return !worker.ready; });

        worker.batch = std::move(result);
        worker.ready = true;
        worker.cv.notify_one();
    }

}

std::optional<EngineBatch> ScanWorkersPool::GetNext() {
    if (consume_rg_ >= num_row_groups_) {
        return std::nullopt;
    }

    auto& worker = workers_[consume_rg_ % num_workers_];
    std::unique_lock lock(worker.mu);
    worker.cv.wait(lock, [&] { return worker.ready; });

    std::optional<EngineBatch> batch = std::move(worker.batch);
    worker.batch.reset();
    worker.ready = false;
    worker.cv.notify_one();

    ++consume_rg_;
    return batch;
}

}  // namespace column_engine::internal
