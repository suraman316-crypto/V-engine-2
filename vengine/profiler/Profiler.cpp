#include <vengine/profiler/Profiler.hpp>

namespace vengine::profiler {

Profiler& Profiler::instance() {
    static Profiler p;
    return p;
}

void Profiler::reset() noexcept {
    active_.clear();
    scopes_.clear();
}

std::size_t Profiler::begin(std::string name) {
    active_.push_back({std::move(name), std::chrono::steady_clock::now()});
    return active_.size() - 1;
}

void Profiler::end(std::size_t handle) {
    if (handle >= active_.size()) return;
    auto& a = active_[handle];
    double secs = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - a.start).count();
    for (auto& s : scopes_) {
        if (s.name == a.name) { s.seconds += secs; s.calls += 1; a.name.clear(); return; }
    }
    scopes_.push_back({std::move(a.name), secs, 1});
}

} // namespace vengine::profiler
