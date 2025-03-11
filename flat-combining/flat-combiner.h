#pragma once

#include "spinlock.h"

#include <atomic>
#include <vector>

////////////////////////////////////////////////////////////////////////////////

template <typename Variant, typename Dispatcher>
class FlatCombinerBase {
public:
    FlatCombinerBase(int concurrency);

    ~FlatCombinerBase() = default;
    FlatCombinerBase(const FlatCombinerBase& other) = delete;
    FlatCombinerBase& operator=(const FlatCombinerBase& other) = delete;
    FlatCombinerBase(FlatCombinerBase&& other) = delete;
    FlatCombinerBase& operator=(FlatCombinerBase&& other) = delete;

    struct Message {
        std::atomic<bool> request_sent = false;
        Variant body;
        char padding[64];
    };

    void* CheckIn();

protected:
    void Run(Message* message);

private:
    void Combiner();

    // TODO: Your solution
};

////////////////////////////////////////////////////////////////////////////////

template <typename Variant, typename Dispatcher>
FlatCombinerBase<Variant, Dispatcher>::FlatCombinerBase(int concurrency)
    : publication_list_(concurrency) {
}

template <typename Variant, typename Dispatcher>
void* FlatCombinerBase<Variant, Dispatcher>::CheckIn() {
    // TODO: Your solution
}

template <typename Variant, typename Dispatcher>
void FlatCombinerBase<Variant, Dispatcher>::Run(FlatCombinerBase::Message* message) {
    // TODO: Your solution
}

template <typename Variant, typename Dispatcher>
void FlatCombinerBase<Variant, Dispatcher>::Combiner() {
    // TODO: Your solution
}

////////////////////////////////////////////////////////////////////////////////
