#pragma once

// V Engine 2.0 — Networking: client/server transport abstraction, state
// replication, and snapshot interpolation for smooth remote-entity motion.
//
// We don't ship a specific socket impl here (the Android backend provides
// raw UDP/TCP). Instead this module defines the message framing, the
// reliable-ordered channel (sequence numbers + ack), entity replication
// (authoritative server, interpolated clients), and lag compensation hooks.

#include <vengine/Common.hpp>
#include <vengine/math/Vec2.hpp>

#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace vengine::net {

using ClientId = std::uint32_t;
constexpr ClientId kInvalidClient = 0;

enum class Channel : u8 {
    Unreliable,       ///< fire-and-forget UDP (movement snapshots)
    ReliableOrdered,  ///< ack + reorder (chat, events)
    ReliableUnordered,///< ack only (large state)
};

struct Message {
    std::uint16_t type{0};
    std::uint32_t sequence{0};
    Channel channel{Channel::Unreliable};
    std::vector<u8> payload;
};

/// Serialize/deserialize helpers (little-endian).
inline void write_u8(std::vector<u8>& out, u8 v) { out.push_back(v); }
inline void write_u16(std::vector<u8>& out, std::uint16_t v) { out.push_back(v & 0xFF); out.push_back((v >> 8) & 0xFF); }
inline void write_u32(std::vector<u8>& out, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) { out.push_back((v >> (i * 8)) & 0xFF); }
}
inline void write_float(std::vector<u8>& out, float v) { std::uint32_t u; std::memcpy(&u, &v, 4); write_u32(out, u); }

inline u8 read_u8(const std::vector<u8>& in, std::size_t& off) { return in[off++]; }
inline std::uint16_t read_u16(const std::vector<u8>& in, std::size_t& off) {
    return std::uint16_t(in[off]) | (std::uint16_t(in[off + 1]) << 8); off += 2;
}
inline std::uint32_t read_u32(const std::vector<u8>& in, std::size_t& off) {
    std::uint32_t v = 0; for (int i = 0; i < 4; ++i) v |= std::uint32_t(in[off + i]) << (i * 8); off += 4; return v;
}
inline float read_float(const std::vector<u8>& in, std::size_t& off) {
    std::uint32_t u = read_u32(in, off); float v; std::memcpy(&v, &u, 4); return v;
}

/// Reliable-ordered channel: tracks the next expected sequence and acks.
class ReliableChannel {
public:
    void send(Message m) {
        m.sequence = next_seq_++;
        m.channel = Channel::ReliableOrdered;
        outbox_.push_back(std::move(m));
    }
    /// Returns messages in order; buffers out-of-order ones.
    std::vector<Message> receive(const std::vector<Message>& incoming) {
        std::vector<Message> delivered;
        for (const auto& m : incoming) {
            if (m.channel != Channel::ReliableOrdered) continue;
            if (m.sequence == next_expected_) {
                delivered.push_back(m);
                ++next_expected_;
                // flush buffered
                while (buffered_.count(next_expected_)) {
                    delivered.push_back(buffered_[next_expected_]);
                    buffered_.erase(next_expected_);
                    ++next_expected_;
                }
            } else if (m.sequence > next_expected_) {
                buffered_[m.sequence] = m;
            }
        }
        return delivered;
    }
    std::vector<Message>& outbox() noexcept { return outbox_; }
    void clear_outbox(std::size_t n) { outbox_.erase(outbox_.begin(), outbox_.begin() + std::min(n, outbox_.size())); }
private:
    std::uint32_t next_seq_{1};
    std::uint32_t next_expected_{1};
    std::unordered_map<std::uint32_t, Message> buffered_;
    std::vector<Message> outbox_;
};

struct Snapshot {
    struct Entity {
        std::uint32_t id{0};
        math::Vec2f pos{};
        float rot{0.0f};
    };
    std::uint32_t frame{0};
    double timestamp{0.0};
    std::vector<Entity> entities;
};

/// Interpolates between two snapshots to render remote entities smoothly.
class SnapshotInterpolator {
public:
    void add_snapshot(Snapshot s) {
        history_.push_back(std::move(s));
        if (history_.size() > max_history_) history_.pop_front();
    }
    /// Sample entity state at `render_time` (server time - interpolation delay).
    Snapshot sample(double render_time, float delay = 0.1f) const {
        if (history_.empty()) return {};
        const double t = render_time - delay;
        if (history_.size() == 1 || t <= history_.front().timestamp) return history_.front();
        if (t >= history_.back().timestamp) return history_.back();
        for (std::size_t i = 1; i < history_.size(); ++i) {
            if (history_[i].timestamp >= t) {
                const Snapshot& a = history_[i - 1];
                const Snapshot& b = history_[i];
                const float alpha = static_cast<float>((t - a.timestamp) / (b.timestamp - a.timestamp));
                Snapshot out; out.frame = a.frame; out.timestamp = t;
                for (const auto& ea : a.entities) {
                    for (const auto& eb : b.entities) {
                        if (ea.id == eb.id) {
                            Snapshot::Entity e;
                            e.id = ea.id;
                            e.pos = ea.pos + (eb.pos - ea.pos) * alpha;
                            e.rot = ea.rot + (eb.rot - ea.rot) * alpha;
                            out.entities.push_back(e);
                            break;
                        }
                    }
                }
                return out;
            }
        }
        return history_.back();
    }
    void set_max_history(std::size_t n) noexcept { max_history_ = n; }
private:
    std::deque<Snapshot> history_;
    std::size_t max_history_{20};
};

/// Connection-level interface a transport (UDP socket) implements.
class Transport {
public:
    virtual ~Transport() = default;
    virtual void send_to(ClientId id, const std::vector<u8>& bytes) = 0;
    virtual std::vector<std::pair<ClientId, std::vector<u8>>> poll() = 0;
};

} // namespace vengine::net
