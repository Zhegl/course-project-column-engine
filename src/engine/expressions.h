#pragma once
#include <regex>
#include <string>
#include "engine/batch.h"
#include "types/types.h"

namespace column_engine::internal {

class AddColFun {
public:
    virtual ColumnValue Get(EngineBatch& batch, RowIndex i) = 0;
    virtual std::string GetName() = 0;
    virtual ~AddColFun() = default;
};

class IntLiteral : public AddColFun {
public:
    explicit IntLiteral(int64_t val) : val_(val) {
    }
    ColumnValue Get(EngineBatch&, RowIndex) override {
        return val_;
    }
    std::string GetName() override {
        return std::to_string(val_);
    }

private:
    int64_t val_;
};

class StrLiteral : public AddColFun {
public:
    explicit StrLiteral(std::string val) : val_(std::move(val)) {
    }
    ColumnValue Get(EngineBatch&, RowIndex) override {
        return val_;
    }
    std::string GetName() override {
        return "'" + val_ + "'";
    }

private:
    std::string val_;
};

class IntColRef : public AddColFun {
public:
    IntColRef(size_t col_idx, std::string col_name)
        : col_idx_(col_idx), col_name_(std::move(col_name)) {
    }
    ColumnValue Get(EngineBatch& batch, RowIndex i) override {
        return std::get<std::vector<int64_t>>(batch.columns[col_idx_])[i];
    }
    std::string GetName() override {
        return col_name_;
    }

private:
    size_t col_idx_;
    std::string col_name_;
};

class StrColRef : public AddColFun {
public:
    StrColRef(size_t col_idx, std::string col_name)
        : col_idx_(col_idx), col_name_(std::move(col_name)) {
    }
    ColumnValue Get(EngineBatch& batch, RowIndex i) override {
        return std::get<std::vector<std::string>>(batch.columns[col_idx_])[i];
    }
    std::string GetName() override {
        return col_name_;
    }

private:
    size_t col_idx_;
    std::string col_name_;
};

class IntOffset : public AddColFun {
public:
    IntOffset(size_t col_idx, std::string col_name, int64_t delta)
        : col_idx_(col_idx), col_name_(std::move(col_name)), delta_(delta) {
    }
    ColumnValue Get(EngineBatch& batch, RowIndex i) override {
        return std::get<std::vector<int64_t>>(batch.columns[col_idx_])[i] + delta_;
    }
    std::string GetName() override {
        if (delta_ >= 0) {
            return col_name_ + " + " + std::to_string(delta_);
        }
        return col_name_ + " - " + std::to_string(-delta_);
    }

private:
    size_t col_idx_;
    std::string col_name_;
    int64_t delta_;
};

class StrLen : public AddColFun {
public:
    StrLen(size_t col_idx, std::string col_name)
        : col_idx_(col_idx), col_name_(std::move(col_name)) {
    }
    ColumnValue Get(EngineBatch& batch, RowIndex i) override {
        return static_cast<int64_t>(
            std::get<std::vector<std::string>>(batch.columns[col_idx_])[i].size());
    }
    std::string GetName() override {
        return "length(" + col_name_ + ")";
    }

private:
    size_t col_idx_;
    std::string col_name_;
};

class RegexpReplace : public AddColFun {
public:
    RegexpReplace(size_t col_idx, std::string col_name, std::string pattern,
                  std::string replacement)
        : col_idx_(col_idx),
          col_name_(std::move(col_name)),
          pattern_(pattern),
          re_(pattern),
          replacement_(std::move(replacement)) {
    }
    ColumnValue Get(EngineBatch& batch, RowIndex i) override {
        const std::string& s = std::get<std::vector<std::string>>(batch.columns[col_idx_])[i];
        return std::regex_replace(s, re_, replacement_);
    }
    std::string GetName() override {
        return "regexp_replace(" + col_name_ + ", '" + pattern_ + "', '" + replacement_ + "')";
    }

private:
    size_t col_idx_;
    std::string col_name_;
    std::string pattern_;
    std::regex re_;
    std::string replacement_;
};

class Strftime : public AddColFun {
public:
    Strftime(size_t col_idx, std::string col_name, std::string fmt)
        : col_idx_(col_idx), col_name_(std::move(col_name)), fmt_(std::move(fmt)) {
    }
    ColumnValue Get(EngineBatch& batch, RowIndex i) override;
    std::string GetName() override {
        return "strftime('" + fmt_ + "', " + col_name_ + ")";
    }

private:
    size_t col_idx_;
    std::string col_name_;
    std::string fmt_;
};

}  // namespace column_engine::internal
