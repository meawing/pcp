#pragma once

#include <optional>
#include <atomic>
#include <cassert>
#include <expected>

#include <lines/fibers/api.hpp>
#include <lines/fibers/queue.hpp>
#include <lines/sync/wait_queue.hpp>
#include <function2/function2.hpp>

namespace future {

template <class T>
using Result = std::expected<T, std::exception_ptr>;

struct Unit {};

namespace detail {

template <class T>
class SharedState {
public:
    using Callback = fu2::unique_function<void(Result<T>&& result) noexcept>;

    SharedState() 
      : ready_{false}
      , ref_count_{2} // one for Future and one for Promise
    {
    }

    ~SharedState() = default;

    // Called by the promise to set the result.
    void SetResult(Result<T>&& result) {
        bool expected = false;
        if (!ready_.compare_exchange_strong(expected, true)) {
            return;
        }
        result_ = std::move(result);

        Callback cb = std::move(callback_);
        if (cb) {
            cb(std::move(*result_));
        }
        wait_queue_.WakeAll();
    }

    // Called by Future::Subscribe to register a callback.
    void SetCallback(Callback&& cb) {
        if (ready_.load(std::memory_order_acquire)) {
            cb(std::move(*result_));
            return;
        }
        callback_ = std::move(cb);
        if (ready_.load(std::memory_order_acquire)) {
            Callback temp = std::move(callback_);
            if (temp) {
                temp(std::move(*result_));
            }
        }
    }

    bool HasResult() const noexcept {
        return ready_.load(std::memory_order_acquire);
    }

    // Returns the stored result by reference.
    Result<T>& GetResult() & {
        assert(HasResult());
        return *result_;
    }
    const Result<T>& GetResult() const & {
        assert(HasResult());
        return *result_;
    }

    // Moves out the result and clears it from the state.
    Result<T> MoveResult() {
        assert(HasResult());
        // Move from the stored result.
        Result<T> tmp = std::move(*result_);
        // Clear the stored result so subsequent accesses see a moved–from object.
        result_.reset();
        return tmp;
    }

    // Busy–wait until the result is ready.
    void Wait() {
        while (!ready_.load(std::memory_order_acquire)) {
            lines::Yield();
        }
    }

    // Called when a Future or Promise no longer needs the shared state.
    void Detach() noexcept {
        if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete this;
        }
    }

private:
    std::atomic<bool> ready_;
    std::optional<Result<T>> result_;
    Callback callback_;
    lines::WaitQueue wait_queue_;
    std::atomic<int> ref_count_;
};

}  // namespace detail
}  // namespace future
