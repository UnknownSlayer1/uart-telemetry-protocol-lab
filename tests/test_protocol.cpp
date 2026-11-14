#include "telemetry/protocol.hpp"

// Keep assertions active in Release builds as well as Debug builds.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

using telemetry::Packet;
using telemetry::StreamDecoder;

void test_crc_reference() {
    const std::vector<std::uint8_t> v{'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    assert(telemetry::crc16(v.data(), v.size()) == 0x29B1U);
}
void test_round_trip() {
    StreamDecoder decoder;
    Packet input{0x7F, 65535, {0x00, 0xA5, 0x5A, 0xFF}};
    auto frames = decoder.feed(telemetry::encode(input));
    assert(frames.size() == 1);
    assert(frames[0].type == input.type);
    assert(frames[0].sequence == input.sequence);
    assert(frames[0].payload == input.payload);
    assert(decoder.stats().received == 1);
}
void test_one_byte_fragments_and_empty_payload() {
    StreamDecoder decoder;
    const auto frame = telemetry::encode(Packet{0, 0x1234, {}});
    std::size_t count = 0;
    for (const auto byte : frame) {
        const auto frames = decoder.feed(&byte, 1);
        if (!frames.empty()) {
            assert(frames.size() == 1);
            assert(frames[0].sequence == 0x1234);
            assert(frames[0].payload.empty());
            ++count;
        }
    }
    assert(count == 1);
}
void test_crc_recovery_and_noise() {
    auto corrupt = telemetry::encode(Packet{1, 1, {4, 5, 6}});
    corrupt[8] ^= 0x80U;
    const auto good = telemetry::encode(Packet{2, 2, {9, 8}});
    std::vector<std::uint8_t> stream{0x0, 0xFF, 0xA5, 0x00};
    stream.insert(stream.end(), corrupt.begin(), corrupt.end());
    stream.insert(stream.end(), good.begin(), good.end());
    StreamDecoder decoder;
    const auto packets = decoder.feed(stream);
    assert(packets.size() == 1);
    assert(packets[0].sequence == 2);
    assert(decoder.stats().crc_errors == 1);
    assert(decoder.stats().noise_bytes >= 4);
}
void test_bad_version_and_oversized_length() {
    auto invalid_version = telemetry::encode(Packet{1, 1, {7}});
    invalid_version[2] = 2;
    auto invalid_length = telemetry::encode(Packet{1, 1, {7}});
    invalid_length[6] = 0x01;
    invalid_length[7] = 0x01;  // 257 bytes is too large
    auto good = telemetry::encode(Packet{1, 3, {3}});
    std::vector<std::uint8_t> stream;
    stream.insert(stream.end(), invalid_version.begin(), invalid_version.end());
    stream.insert(stream.end(), invalid_length.begin(), invalid_length.end());
    stream.insert(stream.end(), good.begin(), good.end());
    StreamDecoder decoder;
    auto result = decoder.feed(stream);
    assert(result.size() == 1);
    assert(result[0].sequence == 3);
    assert(decoder.stats().version_errors >= 1);
    assert(decoder.stats().length_errors >= 1);
}
void test_back_to_back_and_max_payload() {
    const Packet max_packet{2, 0, std::vector<std::uint8_t>(telemetry::kMaxPayload, 0xA5)};
    const auto a = telemetry::encode(max_packet);
    const auto b = telemetry::encode(Packet{3, 1, {42}});
    std::vector<std::uint8_t> stream = a;
    stream.insert(stream.end(), b.begin(), b.end());
    StreamDecoder decoder;
    const auto decoded = decoder.feed(stream);
    assert(decoded.size() == 2);
    assert(decoded[0].payload == max_packet.payload);
    assert(decoded[1].sequence == 1);
    bool did_throw = false;
    try {
        telemetry::encode(Packet{0, 0, std::vector<std::uint8_t>(257)});
    } catch (const std::length_error&) {
        did_throw = true;
    }
    assert(did_throw);
}
void test_empty_and_null() {
    StreamDecoder decoder;
    assert(decoder.feed(nullptr, 0).empty());
    bool did_throw = false;
    try {
        decoder.feed(nullptr, 1);
    } catch (const std::invalid_argument&) {
        did_throw = true;
    }
    assert(did_throw);
    decoder.feed(telemetry::encode(Packet{1, 1, {1}}));
    decoder.reset();
    assert(decoder.stats().received == 0);
}
int main() {
    test_crc_reference();
    test_round_trip();
    test_one_byte_fragments_and_empty_payload();
    test_crc_recovery_and_noise();
    test_bad_version_and_oversized_length();
    test_back_to_back_and_max_payload();
    test_empty_and_null();
    std::cout << "7 protocol test groups passed\n";
}
