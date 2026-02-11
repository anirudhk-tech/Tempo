# Tempo

A lightweight binary parser for ITCH-style market data message streams (Add Order, Executed, Cancel) with correct network byte-order handling.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [API Reference](#api-reference)
- [Performance](#performance)
- [Design](#design)
- [Limitations](#limitations)
- [Platform Support](#platform-support)
- [License](#license)

## Overview

Tempo parses a contiguous stream of binary messages (e.g. NASDAQ TotalView-ITCH style): each message starts with a type byte (`A`, `E`, `X`, …), has a fixed length, and uses big-endian (network) byte order. Instead of raw `memcpy` into structs, Tempo converts multi-byte fields with `ntohs` / `ntohl` / `ntohll` so numeric values are correct on host machines.

This is aimed at:

- **Market data pipelines** — parsing ITCH or similar binary feeds
- **Backtesting** — replaying order/exec/cancel streams
- **Real-time ingest** — callback-driven parsing with minimal allocation

## Features

- **Correct endianness** — network to host conversion for all multi-byte fields
- **Packed structs** — `AddOrder`, `Executed`, `Cancel` with fixed layouts
- **Stream API** — `parse_stream(data, len, handlers)` with optional callbacks for A/E/X
- **Zero dependencies** — C++17, only standard library and `arpa/inet.h` for ntoh
- **Modern CMake** — sanitizer options, compile commands export

## Installation

### CMake (FetchContent)

```cmake
include(FetchContent)
FetchContent_Declare(
  tempo
  GIT_REPOSITORY https://github.com/youruser/tempo.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(tempo)

target_link_libraries(your_target PRIVATE tempo_lib)
```

### Manual

```bash
git clone https://github.com/youruser/tempo.git
cd tempo
cmake -S . -B build
cmake --build build -j
```

Link against `tempo_lib` and add `include/` to your include path.

## Quick Start

```cpp
#include "Tempo/tempo.h"
#include <vector>

int main() {
    std::vector<uint8_t> buffer = load_itch_stream();  // your loader

    TempoHandlers h{};
    h.on_add  = [](const AddOrder& a) { /* use a */ };
    h.on_exec = [](const Executed& e) { /* use e */ };
    h.on_cancel = [](const Cancel& c) { /* use c */ };

    parse_stream(buffer.data(), buffer.size(), h);
}
```

## API Reference

### Message structs

All message types are `__attribute__((packed))` and parsed via a static `from_bytes(const uint8_t* buf)`.

| Type | Struct   | Size | Description        |
|------|----------|------|--------------------|
| `A`, `F` | `AddOrder`  | 35 B | Add order (price in fixed-point, use `price_to_double(raw)` for float) |
| `E`      | `Executed`  | 31 B | Execution          |
| `X`      | `Cancel`    | 23 B | Cancel             |

Other types (`D`, `R`, `S`) are skipped by `parse_stream` (fixed lengths 19, 37, 12).

### TempoHandlers

```cpp
struct TempoHandlers {
    void (*on_add)  (const AddOrder&);
    void (*on_exec) (const Executed&);
    void (*on_cancel)(const Cancel&);
};
```

Set to `nullptr` any callback you don’t need.

### parse_stream

```cpp
void parse_stream(const uint8_t* data, size_t len, const TempoHandlers& h);
```

Sequentially interprets `data` as type-byte + fixed-length messages and invokes the corresponding handler for `A`, `E`, and `X`. Stops on unknown type or if the remaining length is shorter than the message length.

### price_to_double

```cpp
double price_to_double(uint32_t raw);
```

Converts ITCH fixed-point price (network byte order) to a double (e.g. raw / 10000.0).

## Performance

The included benchmark (`tempo_bench`) runs 1000 passes over a binary message file, comparing a **normal** parser (raw `memcpy` into structs, no endian conversion) with the **Tempo** parser (correct `from_bytes` with ntoh). Example output:

```
File size: 89011969 bytes
Normal parser (A/E/X): 5.428984 s
Tempo   parser (A/E/X): 5.336783 s
```

On this ~89 MB stream, Tempo is slightly faster than the raw-memcpy baseline while producing correct host-order values. Build with `-O3` (default in the project) for meaningful numbers.

Run the benchmark:

```bash
./build/tempo_bench
```

(Expects `data/itch_sample` to exist; use a binary ITCH-style capture or generate test data; see below.)

### Generating test data

The project includes a data generator in `src/dataGen.cpp`. Build the `tempo_data` executable and run:

```bash
./build/tempo_data <num_of_adds>
```

This writes a binary stream of Add Order messages; Executed and Cancel messages are inserted randomly as well.

## Design

- **Type byte + switch** — first byte selects message length; no extra branching per field.
- **Packed structs + ntoh** — one `memcpy` per message into a packed struct, then field-wise conversion for multi-byte integers.
- **Callback-driven** — no allocation inside the parser; handlers receive const references.

Message layout (high level):

| Message | Layout (bytes) |
|---------|----------------|
| AddOrder  | type(1), stock_loc(2), tracking(2), timestamp_ns(4), order_ref(8), side(1), shares(4), stock(8), price(4) |
| Executed  | type(1), stock_loc(2), tracking(2), timestamp_ns(4), order_ref(8), exec_shares(4), match_num(8) |
| Cancel    | type(1), stock_loc(2), tracking(2), timestamp_ns(4), order_ref(8), cancel_shares(4) |

All multi-byte integer fields are big-endian in the stream.

## Limitations

| Limitation | Explanation |
|------------|-------------|
| **ITCH-style only** | Fixed message set and layouts; not a generic binary parser |
| **No validation** | Does not verify message type or checksums beyond length |
| **Single-threaded** | No internal locking; synchronize externally if parsing concurrently |
| **ntohll** | 64-bit conversion uses `ntohll`; may require a compatibility shim on some platforms |

## Platform Support

| Platform   | Status |
|-----------|--------|
| macOS     | Supported (Clang, `arpa/inet.h`) |
| Linux     | Supported (GCC/Clang, `arpa/inet.h`) |
| Windows   | Not supported (no `arpa/inet.h`; would need Winsock or manual ntoh) |

Requirements: C++17, CMake 3.20+.

### Building

```bash
# Standard build
cmake -S . -B build
cmake --build build -j

# With AddressSanitizer
cmake -S . -B build-asan -DLANE_ASAN=ON
cmake --build build-asan -j

# With ThreadSanitizer
cmake -S . -B build-tsan -DLANE_TSAN=ON
cmake --build build-tsan -j

# Build benchmark and data generator
cmake --build build -j

# Generate test data (num_of_adds Add Orders; Exec/Cancel added randomly; file created automatically in data/itch_sample)
./build/tempo_data 1000000

# Run benchmark 
./build/tempo_bench
```

The CMake configuration uses `-O3 -Wall -Wextra -Wpedantic` by default.

## License

MIT License. See LICENSE for details.

---

**Tempo** — Correct, fast parsing of ITCH-style binary market data streams.
