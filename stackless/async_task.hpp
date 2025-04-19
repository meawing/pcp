#pragma once

#include <coroutine>
#include <future>
#include <utility>
#include <variant>
#include <optional>

namespace coro {

////////////////////////////////////////////////////////////////////////////////
// Exceptions
////////////////////////////////////////////////////////////////////////////////

class AsyncTaskInvalid : public std::exception {
public:
    const char* what() const noexcept override;
};

////////////////////////////////////////////////////////////////////////////////
// Class declarations
////////////////////////////////////////////////////////////////////////////////

// Don't bother with void types, just use Unit.
using Unit = std::monostate;

template <class T>
class AsyncTask;

namespace detail {

template <class T>
class AsyncTaskAwaiter;

template <class T>
class AsyncTaskPromise;

// Forward declare FinalSuspendAwaitable with complete definition
class FinalSuspendAwaitable {
public:
    explicit FinalSuspendAwaitable(std::coroutine_handle<> continuation) noexcept
        : continuation_(continuation) {
    }
    
    bool await_ready() noexcept {
        return false;
    }
    
    std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept {
        // If we have a continuation, transfer control to it
        if (continuation_) {
            return continuation_;
        }
        
        // Otherwise, return noop_coroutine which effectively destroys the coroutine
        return std::noop_coroutine();
    }
    
    void await_resume() noexcept {
        std::unreachable();  // This should never be called
    }

private:
    std::coroutine_handle<> continuation_;
};

}  // namespace detail

////////////////////////////////////////////////////////////////////////////////
// Methods declarations
////////////////////////////////////////////////////////////////////////////////

/// Represents an allocated, but not-started coroutine.
template <class T>
class AsyncTask {
public:
    /// Define promise type for compiler.
    using promise_type = detail::AsyncTaskPromise<T>;

    /// Default-constructable.
    ///
    /// Postconditions:
    /// - `IsValid() == false`
    AsyncTask() noexcept = default;

    /// Not copyable.
    AsyncTask(const AsyncTask&) = delete;
    AsyncTask& operator=(const AsyncTask&) = delete;

    /// Movable.
    AsyncTask(AsyncTask<T>&& other) noexcept
        : handle_(std::exchange(other.handle_, {})) {
    }
    
    AsyncTask& operator=(AsyncTask<T>&& other) noexcept {
        if (this != &other) {
            if (handle_) {
                handle_.destroy();
            }
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }

    /// Checks whether this task holds coroutine.
    bool IsValid() const noexcept {
        return handle_ != nullptr;
    }

    /// Starts the coroutine, invalidates the current task and returns
    /// the future that is set when the coroutine is completed.
    ///
    /// Preconditions:
    /// - `IsValid() == true` (else throws AsyncTaskInvalid)
    ///
    /// Postconditions:
    /// - `IsValid() == false`
    std::future<T> Run() && {
        if (!IsValid()) {
            throw AsyncTaskInvalid();
        }
        
        auto future = handle_.promise().GetFuture();
        
        // Resume the coroutine
        auto h = std::exchange(handle_, {});
        h.resume();
        
        return future;
    }

    /// This method allows awaiting the current task.
    ///
    /// Preconditions:
    /// - `IsValid() == true` (else throws AsyncTaskInvalid)
    ///
    /// Postconditions:
    /// - `IsValid() == false`
    detail::AsyncTaskAwaiter<T> operator co_await() && {
        if (!IsValid()) {
            throw AsyncTaskInvalid();
        }
        
        return detail::AsyncTaskAwaiter<T>(std::exchange(handle_, {}));
    }

    // If the coroutine is not started, it should be destroyed.
    ~AsyncTask() noexcept {
        if (handle_) {
            handle_.destroy();
        }
    }

private:
    // Users should not be able to call this constructor explicitly,
    // it is the details of implementation.
    AsyncTask(std::coroutine_handle<detail::AsyncTaskPromise<T>> handle) noexcept
        : handle_(handle) {
    }

    // Friend declarations
    friend detail::AsyncTaskPromise<T>;
    
    std::coroutine_handle<detail::AsyncTaskPromise<T>> handle_{};
};

////////////////////////////////////////////////////////////////////////////////

namespace detail {

/// Promise type for AsyncTask coroutines.
template <class T>
class AsyncTaskPromise {
public:
    AsyncTaskPromise() noexcept = default;
    
    // Called after the coroutine and promise is created to obtain the return object
    AsyncTask<T> get_return_object() noexcept {
        return AsyncTask<T>(std::coroutine_handle<AsyncTaskPromise<T>>::from_promise(*this));
    }
    
    // Defines the behavior of just started coroutine
    // Suspend immediately as we implement lazy-started coroutines
    std::suspend_always initial_suspend() noexcept {
        return {};
    }
    
    // Called when `co_return expr` is called
    void return_value(T value) noexcept {
        result_.emplace(std::move(value));
        if (promise_) {
            promise_->set_value(std::move(*result_));
            result_.reset();
        }
    }
    
    // Called if the coroutine ends with an uncaught exception
    void unhandled_exception() noexcept {
        exception_ = std::current_exception();
        if (promise_) {
            promise_->set_exception(exception_);
        }
    }
    
    // Defines the behavior of coroutine that has just finished its execution
    FinalSuspendAwaitable final_suspend() noexcept {
        return FinalSuspendAwaitable(continuation_);
    }
    
    // Get future for the result
    std::future<T> GetFuture() {
        promise_ = std::promise<T>();
        return promise_->get_future();
    }
    
    // Set continuation for symmetric transfer
    void SetContinuation(std::coroutine_handle<> continuation) noexcept {
        continuation_ = continuation;
    }
    
    // Complete the promise with the result or exception (used for co_await)
    void Complete() noexcept {
        // If we already completed the promise in return_value or unhandled_exception,
        // there's nothing more to do
    }
    
    // Check if we have a result
    bool HasResult() const noexcept {
        return result_.has_value();
    }
    
    // Get the result (caller must ensure HasResult() is true)
    T GetResult() {
        if (!result_) {
            throw std::runtime_error("No result available in coroutine");
        }
        T temp_result = std::move(*result_);
        result_.reset();  // Clear the optional to release any resources
        return temp_result;
    }
    
    // Get the exception (if any)
    std::exception_ptr GetException() const noexcept {
        return exception_;
    }

private:
    std::optional<T> result_{};  // Use optional to avoid default construction
    std::exception_ptr exception_{};
    std::optional<std::promise<T>> promise_{};
    std::coroutine_handle<> continuation_{};
    
    // Friend declarations
    friend AsyncTaskAwaiter<T>;
};

////////////////////////////////////////////////////////////////////////////////

/// This awaiter is used to await the completion of the AsyncTask.
template <class T>
class AsyncTaskAwaiter {
public:
    explicit AsyncTaskAwaiter(std::coroutine_handle<AsyncTaskPromise<T>> handle) noexcept
        : handle_(handle) {
    }
    
    bool await_ready() noexcept {
        return false;  // Always suspend to set up continuation
    }
    
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> continuation) noexcept {
        // Set continuation for symmetric transfer
        handle_.promise().SetContinuation(continuation);
        
        // Start the awaited coroutine
        return handle_;
    }
    
    T await_resume() {
        try {
            // Check for exceptions
            if (auto exception = handle_.promise().GetException()) {
                std::rethrow_exception(exception);
            }
            
            // Get the result
            if (handle_.promise().HasResult()) {
                return handle_.promise().GetResult();
            }
            
            throw std::runtime_error("No result available in coroutine");
        } catch (...) {
            // Make sure we don't leak the coroutine frame
            handle_.destroy();
            handle_ = nullptr;
            throw;  // Rethrow the exception
        }
        
        // This point should never be reached due to exceptions or returns above
        std::unreachable();
    }

    ~AsyncTaskAwaiter() {
        if (handle_) {
            handle_.destroy();
        }
    }

private:
    std::coroutine_handle<AsyncTaskPromise<T>> handle_{};
};

////////////////////////////////////////////////////////////////////////////////

}  // namespace detail

}  // namespace coro
