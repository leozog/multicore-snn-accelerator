#pragma once
extern "C"
{
#include "xil_types.h"
}
#include <array>

#if defined(__GNUC__) || defined(__clang__)
using bf128_t = unsigned __int128;
#else
#error "ODIN_neuron requires compiler support for unsigned __int128 bitfields"
#endif

union __attribute__((packed)) __attribute__((aligned(4))) ODIN_neuron
{
    struct
    {
        bf128_t lif_izh_sel : 1;    // Bit 0 (1 = LIF, 0 = IZH)
        bf128_t leak_str : 7;       // Bits 1-7
        bf128_t leak_en : 1;        // Bit 8
        bf128_t thr : 8;            // Bits 9-16
        bf128_t ca_en : 1;          // Bit 17
        bf128_t thetamem : 8;       // Bits 18-25
        bf128_t ca_theta1 : 3;      // Bits 26-28
        bf128_t ca_theta2 : 3;      // Bits 29-31
        bf128_t ca_theta3 : 3;      // Bits 32-34
        bf128_t ca_leak : 5;        // Bits 35-39
        bf128_t lif_reserved : 30;  // Bits 40-69
        bf128_t core : 8;           // Bits 70-77
        bf128_t calcium : 3;        // Bits 78-80
        bf128_t caleak_cnt : 5;     // Bits 81-85
        bf128_t lif_reserved2 : 41; // Bits 86-126
        bf128_t neur_disable : 1;   // Bit 127
    } lif;

    struct
    {
        bf128_t lif_izh_sel : 1;     // Bit 0 (1 = LIF, 0 = IZH)
        bf128_t leak_str : 7;        // Bits 1-7
        bf128_t leak_en : 1;         // Bit 8
        bf128_t fi_sel : 3;          // Bits 9-11
        bf128_t spk_ref : 3;         // Bits 12-14
        bf128_t isi_ref : 3;         // Bits 15-17
        bf128_t reson_sharp_en : 1;  // Bit 18
        bf128_t thr : 3;             // Bits 19-21
        bf128_t rfr : 3;             // Bits 22-24
        bf128_t dapdel : 3;          // Bits 25-27
        bf128_t spklat_en : 1;       // Bit 28
        bf128_t dap_en : 1;          // Bit 29
        bf128_t stim_thr : 3;        // Bits 30-32
        bf128_t phasic_en : 1;       // Bit 33
        bf128_t mixed_en : 1;        // Bit 34
        bf128_t class2_en : 1;       // Bit 35
        bf128_t neg_en : 1;          // Bit 36
        bf128_t rebound_en : 1;      // Bit 37
        bf128_t inhin_en : 1;        // Bit 38
        bf128_t bist_en : 1;         // Bit 39
        bf128_t reson_en : 1;        // Bit 40
        bf128_t thrvar_en : 1;       // Bit 41
        bf128_t thr_sel_of : 1;      // Bit 42
        bf128_t thrleak : 4;         // Bits 43-46
        bf128_t acc_en : 1;          // Bit 47
        bf128_t ca_en : 1;           // Bit 48
        bf128_t thetamem : 3;        // Bits 49-51
        bf128_t ca_theta1 : 3;       // Bits 52-54
        bf128_t ca_theta2 : 3;       // Bits 55-57
        bf128_t ca_theta3 : 3;       // Bits 58-60
        bf128_t ca_leak : 5;         // Bits 61-65
        bf128_t burst_incr : 1;      // Bit 66
        bf128_t reson_sharp_amt : 3; // Bits 67-69
        bf128_t inacc : 11;          // Bits 70-80
        bf128_t refrac : 1;          // Bit 81
        bf128_t core : 4;            // Bits 82-85
        bf128_t dapdel_cnt : 3;      // Bits 86-88
        bf128_t stim_str : 4;        // Bits 89-92
        bf128_t stim_str_tmp : 4;    // Bits 93-96
        bf128_t phasic_lock : 1;     // Bit 97
        bf128_t mixed_lock : 1;      // Bit 98
        bf128_t spkout_done : 1;     // Bit 99
        bf128_t stim0_prev : 2;      // Bits 100-101
        bf128_t inhexc_prev : 2;     // Bits 102-103
        bf128_t bist_lock : 1;       // Bit 104
        bf128_t inhin_lock : 1;      // Bit 105
        bf128_t reson_sign : 2;      // Bits 106-107
        bf128_t thrmod : 4;          // Bits 108-111
        bf128_t thrleak_cnt : 4;     // Bits 112-115
        bf128_t calcium : 3;         // Bits 116-118
        bf128_t caleak_cnt : 5;      // Bits 119-123
        bf128_t burst_lock : 1;      // Bit 124
        bf128_t izh_reserved : 2;    // Bits 125-126
        bf128_t neur_disable : 1;    // Bit 127
    } izh;

    std::array<u32, 4> words;
};

static_assert(sizeof(ODIN_neuron) == 16, "ODIN_neuron must be 128 bits");