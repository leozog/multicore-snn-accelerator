#include "../logger.hpp"
#include "odin_test.hpp"
#include "xil_types.h"

namespace {
constexpr u32 REG_LAST_ADDR = 27;
constexpr u32 NEURAM_NEURONS = 256;
constexpr u32 N = 256;
constexpr u32 M = 8;
constexpr u32 SYNRAM_WORDS = (1u << M) * N / 8;
constexpr u32 ODIN_NEURAM_OFFSET = (0x1u << 15);
constexpr u32 ODIN_SYNRAM_OFFSET = (0x2u << 15);

u32 neuram_word_index(u32 neuron_idx, u32 chunk)
{
    return (chunk * NEURAM_NEURONS) + neuron_idx;
}

u32 reg_wmask(u32 addr)
{
    switch (addr) {
        case 0:
            return 0x00000000u;
        case 1:
            return 0x00000001u;
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            return 0xFFFFFFFFu;
        case 18:
            return 0x000FFFFFu;
        case 19:
        case 20:
            return 0x00000001u;
        case 21:
        case 22:
            return 0x000000FFu;
        case 23:
        case 24:
        case 25:
            return 0x00000001u;
        case 26:
        case 27:
        default:
            return 0x00000000u;
    }
}

u32 reg_test_pattern(u32 addr)
{
    (void)addr;
    return 0xFFFFFFFFu;
}

u32 mem_test_pattern(u32 addr, u32 mask)
{
    return mask ^ (addr * 0xDEADBEEFu);
}
}

namespace odin_test {
bool run_full_rw_test(UINTPTR odin_baseaddr)
{
    auto* regmem_words = reinterpret_cast<volatile u32*>(odin_baseaddr);
    auto* neuram_words = reinterpret_cast<volatile u32*>(odin_baseaddr + ODIN_NEURAM_OFFSET);
    auto* synram_words = reinterpret_cast<volatile u32*>(odin_baseaddr + ODIN_SYNRAM_OFFSET);

    Logger::info_force_uart("ODIN %08X mem_test: REGMEM write/read start", odin_baseaddr);
    for (u32 idx = 0; idx <= REG_LAST_ADDR; ++idx) {
        const u32 mask = reg_wmask(idx);
        const u32 expected = reg_test_pattern(idx) & mask;

        if (idx != 27u) {
            regmem_words[idx] = expected;
        }

        const u32 got = regmem_words[idx];
        if (idx != 27u && got != expected) {
            Logger::error_force_uart("ODIN %08X mem_test: REGMEM mismatch addr=%u exp=0x%08X got=0x%08X",
                                     odin_baseaddr,
                                     static_cast<unsigned>(idx),
                                     static_cast<unsigned>(expected),
                                     static_cast<unsigned>(got));
            return false;
        }
    }

    regmem_words[0] = 1u;

    Logger::info_force_uart("ODIN %08X mem_test: NEURAM write/read start", odin_baseaddr);
    for (u32 idx = 0; idx < NEURAM_NEURONS; ++idx) {
        neuram_words[neuram_word_index(idx, 0u)] = mem_test_pattern((idx << 2) + 0u, 0x13572468u);
        neuram_words[neuram_word_index(idx, 1u)] = mem_test_pattern((idx << 2) + 1u, 0x24681357u);
        neuram_words[neuram_word_index(idx, 2u)] = mem_test_pattern((idx << 2) + 2u, 0x55AA33CCu);
        neuram_words[neuram_word_index(idx, 3u)] = mem_test_pattern((idx << 2) + 3u, 0xAA55CC33u);
    }

    for (u32 idx = 0; idx < NEURAM_NEURONS; ++idx) {
        const u32 exp0 = mem_test_pattern((idx << 2) + 0u, 0x13572468u);
        const u32 exp1 = mem_test_pattern((idx << 2) + 1u, 0x24681357u);
        const u32 exp2 = mem_test_pattern((idx << 2) + 2u, 0x55AA33CCu);
        const u32 exp3 = mem_test_pattern((idx << 2) + 3u, 0xAA55CC33u);
        const u32 got0 = neuram_words[neuram_word_index(idx, 0u)];
        const u32 got1 = neuram_words[neuram_word_index(idx, 1u)];
        const u32 got2 = neuram_words[neuram_word_index(idx, 2u)];
        const u32 got3 = neuram_words[neuram_word_index(idx, 3u)];

        if ((got0 != exp0) || (got1 != exp1) || (got2 != exp2) || (got3 != exp3)) {
            Logger::error_force_uart(
                "ODIN %08X mem_test: NEURAM mismatch neur=%u exp=[%08X %08X %08X %08X] got=[%08X %08X %08X %08X]",
                odin_baseaddr,
                static_cast<unsigned>(idx),
                static_cast<unsigned>(exp0),
                static_cast<unsigned>(exp1),
                static_cast<unsigned>(exp2),
                static_cast<unsigned>(exp3),
                static_cast<unsigned>(got0),
                static_cast<unsigned>(got1),
                static_cast<unsigned>(got2),
                static_cast<unsigned>(got3));
            return false;
        }
    }

    Logger::info_force_uart("ODIN %08X mem_test: SYNRAM write/read start", odin_baseaddr);
    for (u32 idx = 0; idx < SYNRAM_WORDS; ++idx) {
        synram_words[idx] = mem_test_pattern(idx, 0x89ABCDEFu);
    }

    for (u32 idx = 0; idx < SYNRAM_WORDS; ++idx) {
        const u32 expected = mem_test_pattern(idx, 0x89ABCDEFu);
        const u32 got = synram_words[idx];
        if (got != expected) {
            Logger::error_force_uart("ODIN %08X mem_test: SYNRAM mismatch addr=%u exp=0x%08X got=0x%08X",
                                     odin_baseaddr,
                                     static_cast<unsigned>(idx),
                                     static_cast<unsigned>(expected),
                                     static_cast<unsigned>(got));
            return false;
        }
    }

    regmem_words[0] = 0u;
    Logger::info_force_uart("ODIN %08X mem_test: all checks passed", odin_baseaddr);
    return true;
}
}
