#pragma once

#include <memory>

#include "state.hpp"

namespace future {

template <class T>
class Future;

template <class T>
class Promise {
public:
    Promise() : state_(std::make_shared<detail::SharedState<T>>()) {}
    Promise(const Promise&) = delete;
    Promise(Promise&& other) = default;

    void SetValue(T value) && {
        state_->SetValue(std::move(value));
    }

    void SetError(Error error) && {
        state_->SetError(std::move(error));
    }

private:
    friend class Future<T>;

    // Shared state
    std::shared_ptr<detail::SharedState<T>> state_;
};

}  // namespace future