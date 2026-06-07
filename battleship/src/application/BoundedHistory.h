#pragma once

#include <cstddef>
#include <vector>

namespace battleship::application {

template <typename T>
class BoundedHistory final {
public:
    explicit BoundedHistory(std::size_t capacity)
        : capacity_(capacity) {
    }

    void push(T value) {
        values_.push_back(std::move(value));
        while (values_.size() > capacity_) {
            values_.erase(values_.begin());
        }
    }

    void assign(std::vector<T> values) {
        values_ = std::move(values);
        while (values_.size() > capacity_) {
            values_.erase(values_.begin());
        }
    }

    void clear() {
        values_.clear();
    }

    const std::vector<T>& values() const {
        return values_;
    }

private:
    std::size_t capacity_;
    std::vector<T> values_;
};

}
