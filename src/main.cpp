#include "telemetry/protocol.hpp"

#include <iomanip>
#include <iostream>

int main() {
    const telemetry::Packet sensor{0x01, 42, {0x34, 0x12, 0xC8, 0x00}};
    const auto bytes = telemetry::encode(sensor);
    std::cout << "Encoded " << bytes.size() << " bytes: ";
    for (const auto byte : bytes) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<unsigned>(byte) << ' ';
    }
    std::cout << std::dec << '\n';

    telemetry::StreamDecoder decoder;
    for (const auto byte : bytes) {
        const auto decoded = decoder.feed(&byte, 1);
        for (const auto& packet : decoded) {
            std::cout << "Decoded seq=" << packet.sequence
                      << " type=" << static_cast<unsigned>(packet.type)
                      << " payload_bytes=" << packet.payload.size() << '\n';
        }
    }
}
