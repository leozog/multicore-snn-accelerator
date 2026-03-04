template<u16 N, u16 M>
void ODIN_regmem<N, M>::set_syn_sign(u32 neur_id, bool val)
{
    syn_sign[neur_id / 32] =
        (syn_sign[neur_id / 32] & ~(1u << (neur_id % 32))) | ((static_cast<u32>(val) & 0x1u) << (neur_id % 32));
}

template<u16 N, u16 M>
bool ODIN_regmem<N, M>::get_syn_sign(u32 neur_id) const
{
    return (syn_sign[neur_id / 32] & (1u << (neur_id % 32))) != 0;
}

template<u16 N, u16 M>
void ODIN_neuram<N, M>::set_neuron(u32 neur_id, const ODIN_neuron& val)
{
    for (u32 i = 0; i < 4; i++) {
        neurons[i * N + neur_id] = val.words[i];
    }
}

template<u16 N, u16 M>
void ODIN_neuram<N, M>::set_neuron(u32 neur_id, const ODIN_neuron& val) volatile
{
    for (u32 i = 0; i < 4; i++) {
        neurons[i * N + neur_id] = val.words[i];
    }
}

template<u16 N, u16 M>
ODIN_neuron ODIN_neuram<N, M>::get_neuron(u32 neur_id) const
{
    ODIN_neuron neuron{};
    for (u32 i = 0; i < 4; i++) {
        neuron.words[i] = neurons[i * N + neur_id];
    }
    return neuron;
}

template<u16 N, u16 M>
ODIN_neuron ODIN_neuram<N, M>::get_neuron(u32 neur_id) const volatile
{
    ODIN_neuron neuron{};
    for (u32 i = 0; i < 4; i++) {
        neuron.words[i] = neurons[i * N + neur_id];
    }
    return neuron;
}

template<u16 N, u16 M>
void ODIN_synram<N, M>::set_synapse(u32 pre_neur_id, u32 post_neur_id, u8 val)
{
    constexpr u32 neur_mask = powof2(M) - 1;
    u32 addr = ((pre_neur_id & neur_mask) << (M - 3)) | ((post_neur_id & neur_mask) >> 3);
    u32 shift = (post_neur_id & 0x7) << 2;
    synapses[addr] = (synapses[addr] & ~(0xFu << shift)) | ((static_cast<u32>(val) & 0xFu) << shift);
}

template<u16 N, u16 M>
void ODIN_synram<N, M>::set_synapse(u32 pre_neur_id, u32 post_neur_id, u8 val) volatile
{
    constexpr u32 neur_mask = powof2(M) - 1;
    u32 addr = ((pre_neur_id & neur_mask) << (M - 3)) | ((post_neur_id & neur_mask) >> 3);
    u32 shift = (post_neur_id & 0x7) << 2;
    synapses[addr] = (synapses[addr] & ~(0xFu << shift)) | ((static_cast<u32>(val) & 0xFu) << shift);
}

template<u16 N, u16 M>
u8 ODIN_synram<N, M>::get_synapse(u32 pre_neur_id, u32 post_neur_id) const
{
    constexpr u32 neur_mask = powof2(M) - 1;
    u32 addr = ((pre_neur_id & neur_mask) << (M - 3)) | ((post_neur_id & neur_mask) >> 3);
    u32 shift = (post_neur_id & 0x7) << 2;
    return static_cast<u8>((synapses[addr] >> shift) & 0xFu);
}

template<u16 N, u16 M>
u8 ODIN_synram<N, M>::get_synapse(u32 pre_neur_id, u32 post_neur_id) const volatile
{
    constexpr u32 neur_mask = powof2(M) - 1;
    u32 addr = ((pre_neur_id & neur_mask) << (M - 3)) | ((post_neur_id & neur_mask) >> 3);
    u32 shift = (post_neur_id & 0x7) << 2;
    return static_cast<u8>((synapses[addr] >> shift) & 0xFu);
}
