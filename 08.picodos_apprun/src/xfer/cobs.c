#include "xfer/cobs.h"

size_t cobs_decode(const uint8_t* in, size_t in_len, uint8_t* out, size_t out_cap) {
    if (!in || !out || in_len == 0) return 0;

    size_t ri = 0, wi = 0;
    while (ri < in_len) {
        uint8_t code = in[ri++];
        if (code == 0) return 0;
        size_t copy = (size_t)code - 1;
        if (ri + copy > in_len) return 0;

        for (size_t i = 0; i < copy; i++) {
            if (wi >= out_cap) return 0;
            out[wi++] = in[ri++];
        }
        if (code != 0xFF && ri < in_len) {
            if (wi >= out_cap) return 0;
            out[wi++] = 0x00;
        }
    }
    return wi;
}
