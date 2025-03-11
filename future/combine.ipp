#ifndef COMBINE_IMPL
#include "combine.hpp"
#error Do not include this file directly
#endif

// TODO: Your solution

namespace future {

namespace detail {

template <size_t Ind, class Func, class Head>
void ForEach(Func&& func, Head&& head) {
    func(std::integral_constant<size_t, Ind>{}, head);
}

template <size_t Ind, class Func, class Head, class... Tail>
void ForEach(Func&& func, Head&& head, Tail&&... tail) {
    func(std::integral_constant<size_t, Ind>{}, head);
    ForEach<Ind + 1>(func, tail...);
}

// You may find this function useful.
// Usage:
//
// ForEach([&](auto index, auto&& future) {
//     ...
//     std::get<index.value>(tuple) = ...;
//     ...
// }, fs...);
template <class V, class... Fs>
void ForEach(V&& func, Fs&&... fs) {
    ForEach<0>(static_cast<V&&>(func), static_cast<Fs&&>(fs)...);
}

}  // namespace detail

// TODO: Your solution

}  // namespace future
