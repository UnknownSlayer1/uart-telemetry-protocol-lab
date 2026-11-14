#include "telemetry/protocol.hpp"

#include <stdexcept>

namespace telemetry {
namespace {
std::uint16_t read_u16_le(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(p[0]) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[1]) << 8U));
}
void append_u16_le(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8U));
}
}  // namespace

std::uint16_t crc16(const std::uint8_t* data, std::size_t size) {
    std::uint16_t crc = 0xFFFFU;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= static_cast<std::uint16_t>(static_cast<std::uint16_t>(data[i]) << 8U);
        for (int bit = 0; bit < 8; ++bit) {
            crc = static_cast<std::uint16_t>(
                (crc & 0x8000U) ? ((crc << 1U) ^ 0x1021U) : (crc << 1U));
        }
    }
    return crc;
}

std::vector<std::uint8_t> encode(const Packet& packet) {
    if (packet.payload.size() > kMaxPayload) {
        throw std::length_error("telemetry payload too large");
    }
    std::vector<std::uint8_t> out;
    out.reserve(kFrameOverhead + packet.payload.size());
    out.push_back(kMagic0);
    out.push_back(kMagic1);
    out.push_back(kVersion);
    out.push_back(packet.type);
    append_u16_le(out, packet.sequence);
    append_u16_le(out, static_cast<std::uint16_t>(packet.payload.size()));
    out.insert(out.end(), packet.payload.begin(), packet.payload.end());
    append_u16_le(out, crc16(out.data() + 2, out.size() - 2));
    return out;
}

void StreamDecoder::discard_first_byte() {
    buffer_.erase(buffer_.begin());
    ++stats_.noise_bytes;
}

std::vector<Packet> StreamDecoder::feed(const std::uint8_t* data, std::size_t size) {
    if (size != 0 && data == nullptr) {
        throw std::invalid_argument("null data for nonempty input");
    }
    std::vector<Packet> packets;
    for (std::size_t i = 0; i < size; ++i) {
        buffer_.push_back(data[i]);
        for (;;) {
            if (buffer_.empty()) break;
            if (buffer_[0] != kMagic0) {
                discard_first_byte();
                continue;
            }
            if (buffer_.size() < 2) break;
            if (buffer_[1] != kMagic1) {
                discard_first_byte();
                continue;
            }
            if (buffer_.size() < kHeaderSize) break;
            if (buffer_[2] != kVersion) {
                ++stats_.version_errors;
                discard_first_byte();
                continue;
            }
            const std::size_t length = read_u16_le(buffer_.data() + 6);
            if (length > kMaxPayload) {
                ++stats_.length_errors;
                discard_first_byte();
                continue;
            }
            const std::size_t frame_length = kFrameOverhead + length;
            if (buffer_.size() < frame_length) break;
            const auto expected = read_u16_le(buffer_.data() + 8 + length);
            const auto actual = crc16(buffer_.data() + 2, 6 + length);
            if (expected != actual) {
                ++stats_.crc_errors;
                discard_first_byte();
                continue;
            }
            Packet packet;
            packet.type = buffer_[3];
            packet.sequence = read_u16_le(buffer_.data() + 4);
            packet.payload.assign(buffer_.begin() + 8,
                                  buffer_.begin() + static_cast<std::ptrdiff_t>(8 + length));
            packets.push_back(std::move(packet));
            ++stats_.received;
            buffer_.erase(buffer_.begin(),
                          buffer_.begin() + static_cast<std::ptrdiff_t>(frame_length));
        }
    }
    return packets;
}

void StreamDecoder::reset() noexcept {
    buffer_.clear();
    stats_ = {};
}

}  // namespace telemetry
