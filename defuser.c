#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;

    uint32_t low = (uint32_t)challenge;
    uint32_t mid = (uint32_t)(challenge >> 16);
    uint32_t high = (uint32_t)(challenge >> 32);

    uint32_t a = (5 * low) ^ (high + 0xC1EF);
    uint32_t b = 3 * (high - mid) + 0xF950;
    uint32_t c = (b + 8 * low) ^ 0x9D6E;

    values[0] = c ^ a;

    return write_answer(out, cap, values, count);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 6;

    values[0] = ((uint32_t)challenge >> 24) ^ 12;

    for (uint32_t i = 1; i < 6; ++i) {
        unsigned shift = 20 + 5 * (i - 1);
        uint32_t t = (uint32_t)((challenge >> shift) & 31);
        values[i] = 2 * values[i - 1] + (t ^ i);
    }

    return write_answer(out, cap, values, count);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;

    uint32_t x = (uint32_t)challenge & 7;
    uint32_t y = (uint32_t)(challenge >> 32)
               ^ (uint32_t)(challenge >> 3);

    values[0] = x;

    switch (x) {
    case 0:
        values[1] = (y ^ 0x4E9D) + (y >> 2);
        break;
    case 1:
        values[1] = 5 * y + 0xEE44;
        break;
    case 2:
        values[1] = (4 * y) ^ 0xDECA;
        break;
    case 3:
        values[1] = 5 * y + 0x6E37;
        break;
    case 4:
        values[1] = 3 * y + 0x34C3;
        break;
    case 5:
        values[1] = (y ^ 0xDB36) + (y >> 2);
        break;
    case 6:
        values[1] = (4 * y) ^ 0xFD06;
        break;
    case 7:
        values[1] = 5 * y + 0x4F001;
        break;
    }

    return write_answer(out, cap, values, count);
}

static uint32_t p4_walk(uint32_t target, uint32_t low,
                        uint32_t high, uint32_t extra) {
    uint32_t mid = low + ((high - low) >> 1);

    if (target == mid) {
        return (mid + 0x27) ^ extra;
    }

    if (target < mid) {
        uint32_t result = p4_walk(target, low, mid - 1, extra);
        return (5 * result) ^ (mid + extra) ^ 0x12;
    }

    uint32_t result = p4_walk(target, mid + 1, high, extra);
    return (5 * result) ^ (mid + extra) ^ 0x14;
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;

    uint32_t target = (uint32_t)challenge & 0x7F;

    uint32_t extra = (uint32_t)(challenge >> 7)
                   ^ (uint32_t)(challenge >> 32);

    values[0] = p4_walk(target, 0, 127, extra);

    return write_answer(out, cap, values, count);
}

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;

    const uint32_t node_values[12] = {
        0xFAAC, 0x5F25, 0x7432, 0xF549,
        0x8271, 0x6CEF, 0x2876, 0x1A7F,
        0xE640, 0x4CC6, 0x910D, 0xE9D7
    };

    const uint32_t node_tags[12] = {
        0x0A4A, 0x0B54, 0x0820, 0x0959,
        0x0411, 0x0547, 0x006F, 0x0235,
        0x0684, 0x0109, 0x037B, 0x077D
    };

    uint32_t index = (uint32_t)challenge & 7;
    uint32_t steps = (uint32_t)(challenge >> 3) & 3;

    uint32_t seed = (uint32_t)(challenge >> 6)
                  ^ (uint32_t)(challenge >> 32)
                  ^ 0x8939;

    uint32_t result = 3 * seed + node_values[index];
    result ^= node_tags[index];

    for (uint32_t i = 0; i < steps; i++) {
        index++;
        result = 3 * result + node_values[index];
        result ^= node_tags[index];
    }

    values[0] = result;
    values[1] = node_tags[index];

    return write_answer(out, cap, values, count);
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;

    const uint32_t ops[8] = {
        1, 1, 5, 3, 2, 4, 0, 3
    };

    uint32_t x = (uint32_t)(challenge >> 6)
               ^ (uint32_t)(challenge >> 32);

    uint32_t index1 = (uint32_t)challenge & 7;
    uint32_t index2 = (uint32_t)(challenge >> 3) & 7;

    uint32_t indices[2] = {index1, index2};

    for (int i = 0; i < 2; i++) {
        if (i == 1) {
            x ^= 0xCCD;
        }

        switch (ops[indices[i]]) {
            case 0:
                x = (x >> 1) | (x << 31);
                break;

            case 1:
                x = ((x << 5) + 0x432F) ^ (7 * x);
                break;

            case 2:
                x = ((x << 2) | (x >> 30)) ^ 0xB0A1;
                break;

            case 3:
                x = (x + 0x37F) ^ (x >> 3);
                break;

            case 4:
                x = ((x >> 1) ^ x) + 0x9AFC;
                break;

            case 5:
                x = (x ^ 0x3147) + 2 * x;
                break;
        }
    }

    values[0] = x + 0xF5A8;

    return write_answer(out, cap, values, count);
}

int solve_secret(uint64_t challenge, uint64_t context,
                 char *out, size_t cap) {
    uint32_t a = (uint32_t)context & 3;

    uint32_t challenge_fold =
        (uint32_t)(challenge >> 32) ^ (uint32_t)challenge;

    uint32_t context_fold =
        (uint32_t)(context >> 32) ^ (uint32_t)context;

    uint32_t x = challenge_fold ^ context_fold;

    uint32_t mixed = (x << 7) | (x >> 25);
    mixed = (mixed + 0x6D2B) ^ 0x2AD8;

    uint32_t t = a * 0x6D2B;
    t ^= (uint32_t)(context >> 16);

    uint32_t b = mixed ^ t;

    uint32_t c = b ^ (context_fold + a + 0x2AD8
                    + (challenge_fold >> 7));

    int n = snprintf(out, cap,
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOPEN%u %u %u",
        (unsigned)a, (unsigned)b, (unsigned)c);

    return n < 0 || (size_t)n >= cap;

}
