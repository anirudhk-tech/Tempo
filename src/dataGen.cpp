#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <random>
#include <cstring>
#include <arpa/inet.h>

void write_bytes(FILE* f, const void* data, size_t n) {
    if (std::fwrite(data, 1, n, f) != n) {
        std::perror("fwrite");
        std::exit(1);
    }
}

void put_u16_be(uint8_t* dst, uint16_t v) {
    uint16_t be = htons(v);
    std::memcpy(dst, &be, 2);
}

void put_u32_be(uint8_t* dst, uint32_t v) {
    uint32_t be = htonl(v);
    std::memcpy(dst, &be, 4);
}

void put_u64_be(uint8_t* dst, uint64_t v) {
    uint32_t hi = htonl(static_cast<uint32_t>(v >> 32));
    uint32_t lo = htonl(static_cast<uint32_t>(v & 0xFFFFFFFFu));
    std::memcpy(dst, &hi, 4);
    std::memcpy(dst + 4, &lo, 4);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <num_adds>\n", argv[0]);
        return 1;
    }

    const char* out_path = "./data/itch_sample";
    const int num_adds = std::atoi(argv[1]);

    FILE* f = std::fopen(out_path, "wb");
    if (!f) {
        std::perror("fopen");
        return 1;
    }

    std::mt19937_64 rng(123456);
    std::uniform_int_distribution<uint32_t> shares_dist(100, 10000);
    std::uniform_int_distribution<uint32_t> price_dist(1000, 200000);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> action_dist(0, 1);
    std::uniform_real_distribution<double> frac_dist(0.0, 1.0);

    struct OrderState {
        uint64_t ref;
        uint32_t remaining;
    };
    std::vector<OrderState> orders;
    orders.reserve(num_adds);

    uint64_t next_ref = 1;
    uint32_t tracking = 1;
    uint32_t ts = 0;

    for (int i = 0; i < num_adds; ++i) {
        uint8_t msg[35] = {};
        msg[0] = 'A';
        put_u16_be(msg + 1, 1);
        put_u16_be(msg + 3, tracking++);
        put_u32_be(msg + 5, ts += 100);
        uint64_t ref = next_ref++;
        put_u64_be(msg + 11, ref);
        msg[19] = side_dist(rng) ? 'B' : 'S';
        uint32_t sz = shares_dist(rng);
        put_u32_be(msg + 20, sz);
        msg[24] = 'A'; msg[25] = 'A'; msg[26] = 'P'; msg[27] = 'L';
        msg[28] = ' '; msg[29] = ' '; msg[30] = ' '; msg[31] = ' ';
        uint32_t px = price_dist(rng);
        put_u32_be(msg + 32, px);
        write_bytes(f, msg, sizeof(msg));
        orders.push_back(OrderState{ref, sz});
    }

    for (auto& o : orders) {
        int num_events = 1 + (rng() % 3);
        for (int e = 0; e < num_events && o.remaining > 0; ++e) {
            bool do_exec = (action_dist(rng) == 0);
            uint32_t max_qty = o.remaining;
            uint32_t qty = static_cast<uint32_t>(max_qty * frac_dist(rng));
            if (qty == 0) qty = 1;
            if (qty > o.remaining) qty = o.remaining;

            if (do_exec) {
                uint8_t msg[31] = {};
                msg[0] = 'E';
                put_u16_be(msg + 1, 1);
                put_u16_be(msg + 3, tracking++);
                put_u32_be(msg + 5, ts += 50);
                put_u64_be(msg + 11, o.ref);
                put_u32_be(msg + 19, qty);
                put_u64_be(msg + 23, rng());
                write_bytes(f, msg, sizeof(msg));
            } else {
                uint8_t msg[23] = {};
                msg[0] = 'X';
                put_u16_be(msg + 1, 1);
                put_u16_be(msg + 3, tracking++);
                put_u32_be(msg + 5, ts += 50);
                put_u64_be(msg + 11, o.ref);
                put_u32_be(msg + 19, qty);
                write_bytes(f, msg, sizeof(msg));
            }

            o.remaining -= qty;
        }
    }

    std::fclose(f);
    return 0;
}


