# UART Telemetry Protocol Lab

A dependency free C++17 reference implementation of a compact binary telemetry frame, incremental UART stream decoder, and corruption recovery. Designed as a software first starting point for ESP32 sensor reporting, robotics, and embedded firmware work.

## Frame format

| Offset | Bytes | Field | Format |
| --- | --- | --- | --- |
| 0 | 2 | Sync | `A5 5A` |
| 2 | 1 | Version | `01` |
| 3 | 1 | Message type | Application defined |
| 4 | 2 | Sequence | Unsigned 16 bit, little endian |
| 6 | 2 | Payload size | Unsigned 16 bit, little endian, 0 to 256 |
| 8 | N | Payload | Opaque bytes |
| 8 + N | 2 | CRC | CRC16 CCITT FALSE over bytes from version through payload, little endian |

CRC parameters: polynomial `0x1021`, initial value `0xFFFF`, no reflection, no final XOR. A known test vector is `123456789` mapping to `0x29B1`.

The decoder accepts arbitrary byte fragments, waits for complete frames, validates version and length, checks CRC, and scans for the next valid synchronization sequence when bytes are malformed. Invalid frames are skipped; the decoder returns only complete CRC validated packets. `sequence` is delivered to the application for loss accounting; retransmission is **not** implemented.

## Build and run

Prerequisites: CMake 3.16 or newer and a C++17 compiler.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/telemetry_demo
```

Windows executable path may differ (`build/Release/telemetry_demo.exe`). The test suite is portable C++ and requires no serial device.

## API

```cpp
telemetry::Packet p{1, 7, {0x10, 0x20}};
auto bytes = telemetry::encode(p);
telemetry::StreamDecoder decoder;
auto packets = decoder.feed(bytes);
```

In an MCU integration, pass received bytes from a UART ring buffer to `StreamDecoder::feed`. Keep the actual UART ISR small and perform parsing in task context. Consider replacing dynamic allocation with fixed buffers for constrained targets.

## Verification and limitations

The test suite covers a public CRC reference vector, binary payload round trip, one byte UART fragments, empty payload, noise and CRC error recovery, invalid versions and lengths, back to back frames, the maximum payload, oversize rejection, and decoder reset. Results are only confirmed when the tests are actually run in your environment.

This is **software only**. No ESP32 hardware, UART timing, interrupt latency, dynamic memory pressure, concurrent producers, protocol authentication, flow control, ACK or retransmission, long duration fuzzing, or production robustness has been validated. The CRC detects accidental corruption and **is not a security feature**. The example is a learning and review artifact, not a product firmware claim.

## Next steps

1. Add UART DMA or buffered serial glue on the chosen board.
2. Replace the dynamic decoder buffer with a bounded fixed capacity ring buffer.
3. Add packet types for real sensor readings and timestamp fields.
4. Stress test corrupted and dropped byte streams, including sequence gap tracking.

Review and understand this code, run its tests yourself, and extend and validate it before listing the project on a resume as personal engineering work.
