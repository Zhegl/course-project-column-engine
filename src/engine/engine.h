#include <types/types.h>
#include <memory>
#include <string>
#include <vector>

namespace column_engine {

struct EngineBatch {
    std::vector<std::string> names;
    std::vector<ColumnData> columns;
    std::vector<size_t> selection;
};

class Operator {
public:
    virtual void Process(std::shared_ptr<EngineBatch> batch) = 0;
    virtual void Finalize() = 0;
    void SetNext(std::shared_ptr<Operator> next);
    std::shared_ptr<Operator>  GetNext();

    private:
    std::shared_ptr<Operator> next_ = nullptr;
};

class Sink : public Operator {
public:
    Sink(std::shared_ptr<EngineBatch> batch);
    void Process(std::shared_ptr<EngineBatch> batch) override;
    void Finalize() override;
};

class Filter : public Operator {
public:
    void Process(std::shared_ptr<EngineBatch> batch) override;
    void Finalize() override;
};


class Engine {
public:
    explicit Engine(const std::string& path);
    std::shared_ptr<EngineBatch> Run(std::shared_ptr<Operator> root);  

private:
    std::shared_ptr<EngineBatch> GetEngineBatch(size_t row_group);
    Schema schema_;
    std::string path_;
    std::vector<BatchMetaData> batch_meta_;
};
}  // namespace column_engine