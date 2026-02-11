#include "Tempo/tempo.h"

#include <cstdio>
#include <cstdint>
#include <vector>
#include <chrono>
#include <cstring>

int main() {
    FILE* f = std::fopen("data/itch_sample", "rb");
    if (!f) {
        std::perror("fopen");
        return 1;
    }

    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);

    std::vector<uint8_t> buf(size);
    if (std::fread(buf.data(), 1, size, f) != static_cast<size_t>(size)) {
        std::perror("fread");
        return 1;
    }
    std::fclose(f);

    std::printf("File size: %ld bytes\n", size);

    const int runs = 1000;

    auto start_normal = std::chrono::high_resolution_clock::now();
    for (int r = 0; r < runs; ++r) {
        size_t i = 0;
        while (i < buf.size()) {
            char type = static_cast<char>(buf[i]);
            size_t msg_len;
            switch (type) {
                case 'A': case 'F': msg_len = 35; break;
                case 'E':           msg_len = 31; break;
                case 'X':           msg_len = 23; break;
                default:            goto done_normal;
            }
            if (i + msg_len > buf.size()) break;

            const uint8_t* msg = buf.data() + i;

            if (type == 'A') {
                auto a = parse_add_normal(msg);
                (void)a;
            } else if (type == 'E') {
                auto e = parse_exec_normal(msg);
                (void)e;
            } else if (type == 'X') {
                auto x = parse_cancel_normal(msg);
                (void)x;
            }

            i += msg_len;
        }
    }
done_normal:
    auto end_normal = std::chrono::high_resolution_clock::now();

    auto start_tempo = std::chrono::high_resolution_clock::now();
    for (int r = 0; r < runs; ++r) {
        size_t i = 0;
        while (i < buf.size()) {
            char type = static_cast<char>(buf[i]);
            size_t msg_len;
            switch (type) {
                case 'A': case 'F': msg_len = 35; break;
                case 'E':           msg_len = 31; break;
                case 'X':           msg_len = 23; break;
                default:            goto done_tempo;
            }
            if (i + msg_len > buf.size()) break;

            const uint8_t* msg = buf.data() + i;

            if (type == 'A') {
                auto a = parse_add_tempo(msg);
                (void)a;
            } else if (type == 'E') {
                auto e = parse_exec_tempo(msg);
                (void)e;
            } else if (type == 'X') {
                auto x = parse_cancel_tempo(msg);
                (void)x;
            }

            i += msg_len;
        }
    }
done_tempo:
    auto end_tempo = std::chrono::high_resolution_clock::now();

    using sec = std::chrono::duration<double>;
    double normal_s = std::chrono::duration_cast<sec>(end_normal - start_normal).count();
    double tempo_s  = std::chrono::duration_cast<sec>(end_tempo  - start_tempo ).count();

    std::printf("Normal parser (A/E/X): %.6f s\n", normal_s);
    std::printf("Tempo   parser (A/E/X): %.6f s\n", tempo_s);
}

