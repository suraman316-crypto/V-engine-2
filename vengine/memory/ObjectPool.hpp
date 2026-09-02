#pragma once

#include <vengine/Common.hpp>
#include <vengine/core/Assert.hpp>
#include <vengine/core/Error.hpp>
#include <vengine/core/Types.hpp>

#include <algorithm>
#include <vector>

namespace vengine::memory {

/// A fixed-capacity object pool that reuses memory. Designed for the
/// frame loop: allocate once at startup, then acquire/release during frames
/// without touching the global heap.
///
/// Not thread-safe; intended to be used from a single worker (e.g. the
/// render thread). Acquire returns nullptr when the pool is exhausted rather
/// than throwing, so callers can degrade gracefully (README rule #4/#9).
template <typename T>
class ObjectPool {
public:
    explicit ObjectPool(usize capacity)
        : capacity_(capacity) {
        VENGINE_ASSERT(capacity > 0, "pool capacity must be > 0");
        storage_.resize(capacity);
        in_use_.assign(capacity, false);
        free_list_.reserve(capacity);
        for (usize i = 0; i < capacity; ++i) free_list_.push_back(i);
    }

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    template <typename... Args>
    T* acquire(Args&&... args) {
        if (free_list_.empty()) return nullptr;
        usize index = free_list_.back();
        free_list_.pop_back();
        storage_[index] = T(std::forward<Args>(args)...);
        in_use_[index] = true;
        return &storage_[index];
    }

    void release(T* obj) {
        if (!obj) return;
        if (obj < storage_.data() || obj >= storage_.data() + storage_.size()) return;
        usize index = static_cast<usize>(obj - storage_.data());
        if (!in_use_[index]) return;            // double-release guard
        in_use_[index] = false;
        free_list_.push_back(index);
    }

    usize capacity() const noexcept { return capacity_; }
    usize available() const noexcept { return free_list_.size(); }
    usize in_use() const noexcept { return capacity_ - free_list_.size(); }

    void clear() {
        std::fill(in_use_.begin(), in_use_.end(), false);
        free_list_.clear();
        for (usize i = 0; i < capacity_; ++i) free_list_.push_back(i);
    }

private:
    usize              capacity_;
    std::vector<T>     storage_;
    std::vector<char>  in_use_;
    std::vector<usize> free_list_;
};

} // namespace vengine::memory
