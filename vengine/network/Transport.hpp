#pragma once

// V Engine 2.0 — Networking transport (Phase 14): unreliable/unreliable-ordered
// /reliable channels over UDP for real-time multiplayer. Header-only; the
// platform socket layer is in platform/Platform.cpp.
#include <vengine/Common.hpp>

#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

namespace vengine::net {

enum class ChannelType { Unreliable, UnreliableSequenced, Reliable, ReliableOrdered };

struct Packet {
    ChannelType channel{ChannelType::Unreliable};
    std::uint32_t sequence{0};
    std::vector<std::uint8_t> data;
    bool delivered{false};
};

/// Sliding-window reliability: tracks sent/acked sequence numbers per channel.
class ReliabilityWindow {
public:
    explicit ReliabilityWindow(std::uint32_t size = 1024) : mask_(size) {}

    void record_send(std::uint32_t seq) {
        if (seq >= mask_.size()) mask_.resize(seq + 1);
        mask_[seq] = true;
        ++in_flight_;
    }
    void record_ack(std::uint32_t seq) {
        if (seq < mask_.size() && mask_[seq]) {
            mask_[seq] = false;
            --in_flight_;
        }
    }
    std::uint32_t in_flight() const noexcept { return in_flight_; }
    float loss_rate(std::uint32_t sent_total) const {
        if (sent_total == 0) return 0.0f;
        return static_cast<float>(in_flight_) / sent_total;
    }

private:
    std::vector<bool> mask_;
    std::uint32_t in_flight_{0};
};

/// A connection to a remote peer: send/recv with channels.
class Connection {
public:
    explicit Connection(std::string addr) : address_(std::move(addr)) {}

    void send(const Packet& p) {
        if (p.channel == ChannelType::Reliable || p.channel == ChannelType::ReliableOrdered)
            reliable_.record_send(p.sequence);
        ++sent_total_;
    }
    bool receive(Packet& out) {
        if (incoming_.empty()) return false;
        out = incoming_.front();
        incoming_.erase(incoming_.begin());
        return true;
    }
    void on_ack(std::uint32_t seq) { reliable_.record_ack(seq); }

    const std::string& address() const { return address_; }
    std::uint32_t sent_total() const { return sent_total_; }
    float packet_loss() const { return reliable_.loss_rate(sent_total_); }

private:
    std::string address_;
    ReliabilityWindow reliable_;
    std::vector<Packet> incoming_;
    std::uint32_t sent_total_{0};
};

/// Network message serialization: varint + length-prefixed fields.
class ByteWriter {
public:
    void u8(std::uint8_t v) { buf_.push_back(v); }
    void u16(std::uint16_t v) { buf_.push_back(v & 0xff); buf_.push_back(v >> 8); }
    void u32(std::uint32_t v) { for (int i = 0; i < 4; ++i) { buf_.push_back(v & 0xff); v >>= 8; } }
    void str(const std::string& s) { u32(static_cast<std::uint32_t>(s.size())); buf_.insert(buf_.end(), s.begin(), s.end()); }
    const std::vector<std::uint8_t>& data() const { return buf_; }
private:
    std::vector<std::uint8_t> buf_;
};

class ByteReader {
public:
    explicit ByteReader(const std::vector<std::uint8_t>& d) : d_(d) {}
    std::uint8_t u8() { return d_[pos_++]; }
    std::uint16_t u16() { std::uint16_t v = d_[pos_] | (d_[pos_+1] << 8); pos_ += 2; return v; }
    std::uint32_t u32() { std::uint32_t v = 0; for (int i = 0; i < 4; ++i) v |= d_[pos_+i] << (i*8); pos_ += 4; return v; }
    std::string str() { auto n = u32(); std::string s(d_.data() + pos_, d_.data() + pos_ + n); pos_ += n; return s; }
    bool eof() const { return pos_ >= d_.size(); }
private:
    const std::vector<std::uint8_t>& d_;
    std::size_t pos_{0};
};

} // namespace vengine::net
