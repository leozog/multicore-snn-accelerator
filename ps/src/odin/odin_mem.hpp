#pragma once
extern "C"
{
#include "xil_types.h"
}
#include "../intmath.hpp"
#include "odin_neuron.hpp"

template<u16 N, u16 M>
struct __attribute__((packed)) __attribute__((aligned(4))) ODIN_regmem
{
    static_assert((N % 32) == 0, "N must be a multiple of 32");
    static_assert(ceildiv(N, 32) <= 16, "REGMEM_SYN_SIGN supports up to 16 words");

    u32 gate_activity;
    u32 open_loop;
    u32 syn_sign[ceildiv(N, 32)];
    u32 reserved_syn_sign[16 - ceildiv(N, 32)];
    u32 burst_timeref;
    u32 aer_src_ctrl_nneur;
    u32 out_aer_monitor_en;
    u32 monitor_neur_addr;
    u32 monitor_syn_addr;
    u32 update_unmapped_syn;
    u32 propagate_unmapped_syn;
    u32 sdsp_on_syn_stim;
    u32 sw_rst;
    u32 busy;

    void set_syn_sign(u32 neur_id, bool val);
    bool get_syn_sign(u32 neur_id) const;
};

template<u16 N, u16 M>
struct __attribute__((packed)) __attribute__((aligned(4))) ODIN_neuram
{
    u32 neurons[N * 4];

    void set_neuron(u32 neur_id, const ODIN_neuron& val);
    void set_neuron(u32 neur_id, const ODIN_neuron& val) volatile;
    ODIN_neuron get_neuron(u32 neur_id) const;
    ODIN_neuron get_neuron(u32 neur_id) const volatile;
};

template<u16 N, u16 M>
struct __attribute__((packed)) __attribute__((aligned(4))) ODIN_synram
{
    static_assert(M >= 3, "M must be at least 3 for 4-bit synapse packing");

    u32 synapses[powof2(M) * N / 8];

    void set_synapse(u32 pre_neur_id, u32 post_neur_id, u8 val);
    void set_synapse(u32 pre_neur_id, u32 post_neur_id, u8 val) volatile;
    u8 get_synapse(u32 pre_neur_id, u32 post_neur_id) const;
    u8 get_synapse(u32 pre_neur_id, u32 post_neur_id) const volatile;
};

#include "odin_mem.tpp"