#pragma once

#include <variant>
#include <exception>
#include <utility>
#include <optional>

#include <lines/std/atomic.hpp>
#include <lines/std/condvar.hpp>
#include <lines/std/mutex.hpp>

namespace future {

using Error = std::exception_ptr;

namespace detail {

template <class T>
class SharedState {
private:
    struct Container {
        std::mutex mutex;
        lines::Condvar condvar;
        std::optional<std::variant<T, Error>> result;
    };

public:
    SharedState() : container_(std::make_shared<Container>()) {}

    void SetValue(T value) {
        std::lock_guard<std::mutex> lock(container_->mutex);
        if (!container_->result.has_value()) {
            container_->result = std::move(value);
            container_->condvar.NotifyAll();
        }
    }

    void SetError(Error error) {
        std::lock_guard<std::mutex> lock(container_->mutex);
        if (!container_->result.has_value()) {
            container_->result = std::move(error);
            container_->condvar.NotifyAll();
        }
    }

    T Get() {
        std::unique_lock<std::mutex> lock(container_->mutex);
        while (!container_->result.has_value()) {
            container_->condvar.Wait(lock);
        }
        
        auto& result = container_->result.value();
        if (std::holds_alternative<Error>(result)) {
            std::rethrow_exception(std::get<Error>(result));
        }
        
        return std::get<T>(result);
    }

private:
    std::shared_ptr<Container> container_;
};

}  // namespace detail

}  // namespace future