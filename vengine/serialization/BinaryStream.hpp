#pragma once

// V Engine 2.0 — Binary serialization stream (Phase 4 extension): compact
// little-endian byte serialization for save files and network snapshots.
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace vengine::serialization {

class BinaryWriter {
public:
    void write_u8(std::uint8_t v) { buf_.push_back(v); }
    void write_u16(std::uint16_t v) { buf_.push_back(v & 0xff); buf_.push_back((v >> 8) & 0xff); }
    void write_u32(std::uint32_t v) {
        for (int i = 0; i < 4; ++i) { buf_.push_back(v & 0xff); v >>= 8; }
    }
    void write_i32(std::int32_t v) { write_u32(static_cast<std::uint32_t>(v)); }
    void write_f32(float v) { std::uint32_t u; std::memcpy(&u, &v, 4); write_u32(u); }
    void write_string(const std::string& s) {
        write_u32(static_cast<std::uint32_t>(s.size()));
        buf_.insert(buf_.end(), s.begin(), s.end());
    }
    void write_bytes(const std::uint8_t* data, std::size_t n) {
        buf_.insert(buf_.end(), data, data + n);
    }
    const std::vector<std::uint8_t>& data() const { return buf_; }
    std::size_t size() const { return buf_.size(); }
private:
    std::vector<std::uint8_t> buf_;
};

class BinaryReader {
public:
    explicit BinaryReader(const std::vector<std::uint8_t>& d) : d_(d) {}
    std::uint8_t read_u8() { return d_[pos_++]; }
    std::uint16_t read_u16() {
        std::uint16_t v = d_[pos_] | (d_[pos_+1] << 8); pos_ += 2; return v;
    }
    std::uint32_t read_u32() {
        std::uint32_t v = 0; for (int i = 0; i < 4; ++i) v |= d_[pos_+i] << (i*8);
        pos_ += 4; return v;
    }
    std::int32_t read_i32() { return static_cast<std::int32_t>(read_u32()); }
    float read_f32() { std::uint32_t u = read_u32(); float f; std::memcpy(&f, &u, 4); return f; }
    std::string read_string() {
        auto n = read_u32();
        std::string s(reinterpret_cast<const char*>(d_.data() + pos_), n);
        pos_ += n; return s;
    }
    bool eof() const { return pos_ >= d_.size(); }
    std::size_t remaining() const { return d_.size() - pos_; }
private:
    const std::vector<std::uint8_t>& d_;
    std::size_t pos_{0};
};

} // namespace vengine::serialization
