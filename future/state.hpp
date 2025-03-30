#pragma once

#include <expected>

#include <lines/std/atomic.hpp>
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

public:
    SharedState();
    ~SharedState();

    void SetResult(Result<T>&& result);
    void SetCallback(Callback&& callback);

    bool HasResult() const noexcept;
    bool HasCallback() const noexcept;

    /// Called by a destructing Future and Promise.
    /// Calls `delete this` if there are no more references to `this`.
    void Detach() noexcept;

    // Precondition: HasResult() == true;
    template <class Self>
    auto&& GetResult(this Self&& self) noexcept;

private:
    enum class State {
        Empty,
        OnlyResult,
        OnlyCallback,
        Done,
    };

private:
    // Try not to use mutex here. Use CAS-loop instead.

    // TODO: Your solution
};

}  // namespace detail
}  // namespace future
