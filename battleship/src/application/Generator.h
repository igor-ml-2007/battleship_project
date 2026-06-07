#pragma once

#include <coroutine>
#include <exception>
#include <iterator>
#include <utility>

namespace battleship::application {

template <typename T>
class Generator final {
public:
    struct promise_type {
        T current{};

        Generator get_return_object() {
            return Generator(std::coroutine_handle<promise_type>::from_promise(*this));
        }

        std::suspend_always initial_suspend() noexcept {
            return {};
        }

        std::suspend_always final_suspend() noexcept {
            return {};
        }

        std::suspend_always yield_value(T value) noexcept {
            current = std::move(value);
            return {};
        }

        void return_void() noexcept {
        }

        void unhandled_exception() {
            std::terminate();
        }
    };

    class iterator final {
    public:
        explicit iterator(std::coroutine_handle<promise_type> handle)
            : handle_(handle) {
            ++(*this);
        }

        iterator& operator++() {
            handle_.resume();
            done_ = handle_.done();
            return *this;
        }

        const T& operator*() const {
            return handle_.promise().current;
        }

        bool operator==(std::default_sentinel_t) const {
            return done_;
        }

    private:
        std::coroutine_handle<promise_type> handle_;
        bool done_ = false;
    };

    explicit Generator(std::coroutine_handle<promise_type> handle)
        : handle_(handle) {
    }

    Generator(Generator&& other) noexcept
        : handle_(std::exchange(other.handle_, {})) {
    }

    Generator& operator=(Generator&& other) noexcept {
        if (this != &other) {
            if (handle_) {
                handle_.destroy();
            }
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }

    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;

    ~Generator() {
        if (handle_) {
            handle_.destroy();
        }
    }

    iterator begin() {
        return iterator(handle_);
    }

    std::default_sentinel_t end() const noexcept {
        return {};
    }

private:
    std::coroutine_handle<promise_type> handle_;
};

}  // namespace battleship::application
