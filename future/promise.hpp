#pragma once

#include <exception>

#include "state.hpp"

namespace future {

////////////////////////////////////////////////////////////////////////////////
// Exceptions
////////////////////////////////////////////////////////////////////////////////

class PromiseException : public std::exception {
public:
    // NOLINTNEXTLINE
    const char* what() const noexcept override;
};

class PromiseInvalid : public PromiseException {
public:
    // NOLINTNEXTLINE
    const char* what() const noexcept override;
};

class PromiseAlreadySatisfied : public PromiseException {
public:
    // NOLINTNEXTLINE
    const char* what() const noexcept override;
};

class PromiseBroken : public PromiseException {
public:
    // NOLINTNEXTLINE
    const char* what() const noexcept override;
};

////////////////////////////////////////////////////////////////////////////////
// Promise
////////////////////////////////////////////////////////////////////////////////

template <class T>
class Future;

template <class T>
class Promise {
public:
    Promise() : state_(nullptr), fulfilled_(false) {}

    // Not copyable.
    Promise(const Promise&) = delete;
    Promise& operator=(const Promise&) = delete;

    // Movable.
    Promise(Promise&& other) noexcept : state_(other.state_), fulfilled_(other.fulfilled_) {
        other.state_ = nullptr;
        other.fulfilled_ = true;
    }
    Promise& operator=(Promise&& other) noexcept {
        if (this != &other) {
            Cleanup();
            state_ = other.state_;
            fulfilled_ = other.fulfilled_;
            other.state_ = nullptr;
            other.fulfilled_ = true;
        }
        return *this;
    }

    ~Promise() {
        // If promise is still valid and not fulfilled, then fulfill with a PromiseBroken error.
        if (state_ && !fulfilled_) {
            try {
                std::move(*this).SetError(std::make_exception_ptr(PromiseBroken()));
            } catch (...) {
                // Swallow exception during destruction.
            }
        }
        Cleanup();
    }

    ////////////////////////////////////////////////////////////////////////////
    // SetValue
    ////////////////////////////////////////////////////////////////////////////

    template <class M>
        requires std::is_constructible_v<T, M&&>
    void SetValue(M&& value) && {
        if (!IsValid())
            throw PromiseInvalid();
        if (fulfilled_)
            throw PromiseAlreadySatisfied();
        state_->SetResult(Result<T>{std::in_place, std::forward<M>(value)});
        fulfilled_ = true;
    }

    ////////////////////////////////////////////////////////////////////////////
    // SetError
    ////////////////////////////////////////////////////////////////////////////

    void SetError(std::exception_ptr exception) && {
        if (!IsValid())
            throw PromiseInvalid();
        if (fulfilled_)
            throw PromiseAlreadySatisfied();
        state_->SetResult(Result<T>{std::unexpected{exception}});
        fulfilled_ = true;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Status
    ////////////////////////////////////////////////////////////////////////////

    bool IsValid() const noexcept {
        return state_ != nullptr;
    }

    bool IsFulfilled() const noexcept {
        // If moved-from this is not valid OR has been fulfilled.
        return !IsValid() || fulfilled_;
    }

private:
    // allow GetTied to set the shared state pointer
    template <class Q>
    friend std::pair<Future<Q>, Promise<Q>> GetTied();

    void Cleanup() {
        if (state_) {
            state_->Detach();
            state_ = nullptr;
        }
    }

private:
    // pointer to the shared state
    detail::SharedState<T>* state_;
    bool fulfilled_;
};


}  // namespace future
