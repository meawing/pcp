#pragma once

#include <utility>
#include <type_traits>

#include "promise.hpp"
#include "state.hpp"
#include "meta.hpp"
#include "lines/fibers/api.hpp"

namespace future {

////////////////////////////////////////////////////////////////////////////////
// Exceptions
////////////////////////////////////////////////////////////////////////////////

class FutureException : public std::exception {
public:
    // NOLINTNEXTLINE
    const char* what() const noexcept override;
};

class FutureInvalid : public FutureException {
public:
    // NOLINTNEXTLINE
    const char* what() const noexcept override;
};

class FutureNotReady : public FutureException {
public:
    // NOLINTNEXTLINE
    const char* what() const noexcept override;
};

////////////////////////////////////////////////////////////////////////////////
// Future
////////////////////////////////////////////////////////////////////////////////

template <class T>
class Promise;

// future.hpp implementation
template <class T>
class Future {
public:
    using ValueType = T;

    Future() : state_(nullptr) {}

    // Not copyable.
    Future(const Future&) = delete;
    Future& operator=(const Future&) = delete;

    // Movable.
    Future(Future&& other) noexcept : state_(other.state_) {
        other.state_ = nullptr;
    }
    Future& operator=(Future&& other) noexcept {
        if (this != &other) {
            Cleanup();
            state_ = other.state_;
            other.state_ = nullptr;
        }
        return *this;
    }

    ~Future() {
        Cleanup();
    }

    ////////////////////////////////////////////////////////////////////////////
    // GetResult
    ////////////////////////////////////////////////////////////////////////////

    // lvalue overload: returns a reference to the result stored in the shared state.
    Result<T>& GetResult() & {
        if (!IsValid())
            throw FutureInvalid();
        if (!state_->HasResult())
            throw FutureNotReady();
        return state_->GetResult();
    }

    // const lvalue overload
    const Result<T>& GetResult() const & {
        if (!IsValid())
            throw FutureInvalid();
        if (!state_->HasResult())
            throw FutureNotReady();
        return state_->GetResult();
    }

    // rvalue overload: moves out the stored Result and clears it from the shared state.
    Result<T> GetResult() && {
        if (!IsValid())
            throw FutureInvalid();
        if (!state_->HasResult())
            throw FutureNotReady();
        return state_->MoveResult();
    }

    ////////////////////////////////////////////////////////////////////////////
    // GetValue
    ////////////////////////////////////////////////////////////////////////////

    // lvalue: returns the value by reference; if the state holds an error, rethrows.
    T& GetValue() & {
        auto& res = GetResult();
        if (!res)
            std::rethrow_exception(res.error());
        return *res;
    }

    const T& GetValue() const & {
        auto& res = GetResult();
        if (!res)
            std::rethrow_exception(res.error());
        return *res;
    }

    // rvalue: moves out the value by utilizing the rvalue GetResult().
    T GetValue() && {
        auto res = std::move(*this).GetResult();  // calls our MoveResult version
        if (!res)
            std::rethrow_exception(res.error());
        return std::move(*res);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Wait
    ////////////////////////////////////////////////////////////////////////////

    Future<T>& Wait() & {
        if (!IsValid())
            throw FutureInvalid();
        state_->Wait();
        return *this;
    }
    Future<T>&& Wait() && {
        if (!IsValid())
            throw FutureInvalid();
        state_->Wait();
        return std::move(*this);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Status
    ////////////////////////////////////////////////////////////////////////////

    bool IsValid() const noexcept {
        return state_ != nullptr;
    }

    bool IsReady() const {
        if (!IsValid())
            throw FutureInvalid();
        return state_->HasResult();
    }

    bool HasException() const {
        if (!IsValid())
            throw FutureInvalid();
        if (!state_->HasResult())
            throw FutureNotReady();
        return !state_->GetResult();
    }

    ////////////////////////////////////////////////////////////////////////////
    // Subscribe
    ////////////////////////////////////////////////////////////////////////////

    template <detail::SubscribeCallback<Result<T>> F>
    void Subscribe(F&& callback) && {
        if (!IsValid())
            throw FutureInvalid();
        state_->SetCallback(fu2::unique_function<void(Result<T>&&) noexcept>(std::forward<F>(callback)));
        state_->Detach();
        state_ = nullptr;
    }

private:
    template <class Q>
    friend std::pair<Future<Q>, Promise<Q>> GetTied();

    void Cleanup() {
        if (state_) {
            state_->Detach();
            state_ = nullptr;
        }
    }

private:
    detail::SharedState<T>* state_;
};

////////////////////////////////////////////////////////////////////////////////

template <class T>
[[nodiscard]] std::pair<Future<T>, Promise<T>> GetTied() {
    auto* state = new detail::SharedState<T>();
    Future<T> f;
    Promise<T> p;
    f.state_ = state;
    p.state_ = state;
    return {std::move(f), std::move(p)};
}



}  // namespace future
