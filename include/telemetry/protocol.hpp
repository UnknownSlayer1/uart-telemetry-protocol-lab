#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace telemetry {

inline constexpr std::uint8_t kMagic0 = 0xA5;
inline constexpr std::uint8_t kMagic1 = 0x5A;
inline constexpr std::uint8_t kVersion = 1;
inline constexpr std::size_t kMaxPayload = 256;
inline constexpr std::size_t kHeaderSize = 8;
inline constexpr std::size_t kFrameOverhead = 10;

struct Packet {
    std::uint8_t type = 0;
    std::uint16_t sequence = 0;
    std::vector<std::uint8_t> payload;
};

struct DecoderStats {
    std::size_t received = 0;
    std::size_t crc_errors = 0;
    std::size_t version_errors = 0;
    std::size_t length_errors = 0;
    std::size_t noise_bytes = 0;
};

// CRC16 CCITT FALSE: polynomial 0x1021, initial value 0xFFFF,
// no reflection and no final XOR.
std::uint16_t crc16(const std::uint8_t* data, std::size_t size);

// Throws std::length_error when payload exceeds kMaxPayload.
std::vector<std::uint8_t> encode(const Packet& packet);

class StreamDecoder {
public:
    // Feed any fragment of a UART byte stream. Returns zero or more complete packets.
    std::vector<Packet> feed(const std::uint8_t* data, std::size_t size);
    std::vector<Packet> feed(const std::vector<std::uint8_t>& bytes) {
        return feed(bytes.data(), bytes.size());
    }
    const DecoderStats& stats() const noexcept { return stats_; }
    void reset() noexcept;

private:
    std::vector<std::uint8_t> buffer_;
    DecoderStats stats_{};
    void discard_first_byte();
};

}  // namespace telemetry
