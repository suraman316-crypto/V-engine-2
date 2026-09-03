#pragma once

// V Engine 2.0 — Asset streaming (Phase 19): background priority-based
// loading/unloading of assets to fit memory budgets on low-end devices.
//
// Assets are loaded asynchronously via a worker queue with priorities, so
// gameplay never blocks on disk I/O. An LRU cache evicts unused assets when
// the memory budget is exceeded.

#include <vengine/Common.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace vengine::stream {

enum class Priority { Low, Normal, High, Critical };

struct AssetHandle {
    std::string id;
    std::uint64_t size_bytes{0};
    std::atomic<bool> loaded{false};
    std::atomic<bool> failed{false};
};

/// LRU cache of loaded assets with a memory budget.
class AssetCache {
public:
    explicit AssetCache(std::uint64_t budget_bytes) : budget_(budget_bytes) {}

    std::shared_ptr<AssetHandle> acquire(const std::string& id, std::uint64_t size) {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = entries_.find(id);
        if (it != entries_.end()) {
            lru_.touch(id);
            return it->second;
        }
        while (used_ + size > budget_ && !lru_.empty()) {
            auto victim = lru_.evict();
            auto vit = entries_.find(victim);
            if (vit != entries_.end()) {
                used_ -= vit->second->size_bytes;
                entries_.erase(vit);
            }
        }
        auto h = std::make_shared<AssetHandle>();
        h->id = id; h->size_bytes = size;
        entries_[id] = h;
        lru_.touch(id);
        used_ += size;
        return h;
    }
    void release(const std::string& id) {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = entries_.find(id);
        if (it != entries_.end()) { used_ -= it->second->size_bytes; entries_.erase(it); }
    }
    std::uint64_t used() const { return used_; }
    std::uint64_t budget() const { return budget_; }
    std::size_t cached_count() const { return entries_.size(); }

private:
    struct LRU {
        std::deque<std::string> order;
        std::unordered_map<std::string, std::size_t> pos;
        void touch(const std::string& id) {
            auto it = pos.find(id);
            if (it != pos.end()) order.erase(order.begin() + it->second);
            pos[id] = order.size();
            order.push_back(id);
        }
        std::string evict() {
            std::string v = order.front(); order.pop_front();
            pos.erase(v);
            return v;
        }
        bool empty() const { return order.empty(); }
    };
    std::uint64_t budget_;
    std::uint64_t used_{0};
    std::unordered_map<std::string, std::shared_ptr<AssetHandle>> entries_;
    LRU lru_;
    mutable std::mutex mtx_;
};

/// Async loader: a worker thread processes a priority queue of load jobs.
class AsyncLoader {
public:
    using LoadFn = std::function<bool(const std::string&)>;

    explicit AsyncLoader(LoadFn fn) : load_fn_(std::move(fn)) {
        worker_ = std::thread([this]{ run(); });
    }
    ~AsyncLoader() { stop(); }

    void enqueue(const std::string& id, Priority p) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            queue_.push_back({id, p, false});
        }
        cv_.notify_one();
    }
    void stop() {
        if (!running_) return;
        running_ = false;
        cv_.notify_all();
        if (worker_.joinable()) worker_.join();
    }
    std::size_t pending() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return queue_.size();
    }

private:
    struct Job { std::string id; Priority p; bool done; };
    void run() {
        while (running_) {
            Job job;
            {
                std::unique_lock<std::mutex> lk(mtx_);
                cv_.wait(lk, [this]{ return !queue_.empty() || !running_; });
                if (!running_) return;
                // pick highest priority
                auto it = std::min_element(queue_.begin(), queue_.end(),
                    [](const Job& a, const Job& b){ return a.p > b.p; });
                job = *it; queue_.erase(it);
            }
            bool ok = load_fn_(job.id);
            (void)ok;
        }
    }
    LoadFn load_fn_;
    std::thread worker_;
    std::atomic<bool> running_{true};
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::deque<Job> queue_;
};

} // namespace vengine::stream
