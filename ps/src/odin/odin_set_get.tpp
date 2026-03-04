template<u16 N, u16 M>
void ODIN<N, M>::start_gate_activity()
{
    regmem->gate_activity = 1;
    while (regmem->gate_activity == 0) {
        taskYIELD();
    }
}

template<u16 N, u16 M>
void ODIN<N, M>::stop_gate_activity()
{
    regmem->gate_activity = 0;
    while (regmem->gate_activity == 1) {
        taskYIELD();
    }
}

template<u16 N, u16 M>
void ODIN<N, M>::set_regmem(const ODIN_regmem<N, M>& val)
{
    RtosMutexGuard guard(mutex);

    regmem->open_loop = val.open_loop;
    for (u32 i = 0; i < (N / 32); i++) {
        regmem->syn_sign[i] = val.syn_sign[i];
    }
    for (u32 i = 0; i < (16 - ceildiv(N, 32)); i++) {
        regmem->reserved_syn_sign[i] = val.reserved_syn_sign[i];
    }
    regmem->burst_timeref = val.burst_timeref;
    regmem->aer_src_ctrl_nneur = val.aer_src_ctrl_nneur;
    regmem->out_aer_monitor_en = val.out_aer_monitor_en;
    regmem->monitor_neur_addr = val.monitor_neur_addr;
    regmem->monitor_syn_addr = val.monitor_syn_addr;
    regmem->update_unmapped_syn = val.update_unmapped_syn;
    regmem->propagate_unmapped_syn = val.propagate_unmapped_syn;
    regmem->sdsp_on_syn_stim = val.sdsp_on_syn_stim;
}

template<u16 N, u16 M>
void ODIN<N, M>::set_neuram(const ODIN_neuram<N, M>& val)
{
    RtosMutexGuard guard(mutex);
    start_gate_activity();
    for (u32 i = 0; i < (N * 4); i++) {
        neuram->neurons[i] = val.neurons[i];
    }
    stop_gate_activity();
}

template<u16 N, u16 M>
void ODIN<N, M>::set_synram(const ODIN_synram<N, M>& val)
{
    RtosMutexGuard guard(mutex);
    start_gate_activity();
    constexpr u32 syn_words = powof2(M) * N / 8;
    for (u32 i = 0; i < syn_words; i++) {
        synram->synapses[i] = val.synapses[i];
    }
    stop_gate_activity();
}

template<u16 N, u16 M>
ODIN_regmem<N, M> ODIN<N, M>::get_regmem()
{
    RtosMutexGuard guard(mutex);
    ODIN_regmem<N, M> val{};

    val.open_loop = regmem->open_loop;
    for (u32 i = 0; i < (N / 32); i++) {
        val.syn_sign[i] = regmem->syn_sign[i];
    }
    for (u32 i = 0; i < (16 - ceildiv(N, 32)); i++) {
        val.reserved_syn_sign[i] = regmem->reserved_syn_sign[i];
    }
    val.burst_timeref = regmem->burst_timeref;
    val.aer_src_ctrl_nneur = regmem->aer_src_ctrl_nneur;
    val.out_aer_monitor_en = regmem->out_aer_monitor_en;
    val.monitor_neur_addr = regmem->monitor_neur_addr;
    val.monitor_syn_addr = regmem->monitor_syn_addr;
    val.update_unmapped_syn = regmem->update_unmapped_syn;
    val.propagate_unmapped_syn = regmem->propagate_unmapped_syn;
    val.sdsp_on_syn_stim = regmem->sdsp_on_syn_stim;

    return val;
}

template<u16 N, u16 M>
ODIN_neuram<N, M> ODIN<N, M>::get_neuram()
{
    RtosMutexGuard guard(mutex);
    ODIN_neuram<N, M> val{};
    start_gate_activity();
    for (u32 i = 0; i < (N * 4); i++) {
        val.neurons[i] = neuram->neurons[i];
    }
    stop_gate_activity();
    return val;
}

template<u16 N, u16 M>
ODIN_synram<N, M> ODIN<N, M>::get_synram()
{
    RtosMutexGuard guard(mutex);
    ODIN_synram<N, M> val{};
    constexpr u32 syn_words = powof2(M) * N / 8;
    start_gate_activity();
    for (u32 i = 0; i < syn_words; i++) {
        val.synapses[i] = synram->synapses[i];
    }
    stop_gate_activity();
    return val;
}

template<u16 N, u16 M>
void ODIN<N, M>::set_neuron(u32 neur_id, const ODIN_neuron& val)
{
    RtosMutexGuard guard(mutex);
    start_gate_activity();
    neuram->set_neuron(neur_id, val);
    stop_gate_activity();
}

template<u16 N, u16 M>
ODIN_neuron ODIN<N, M>::get_neuron(u32 neur_id)
{
    RtosMutexGuard guard(mutex);
    start_gate_activity();
    ODIN_neuron val = neuram->get_neuron(neur_id);
    stop_gate_activity();
    return val;
}

template<u16 N, u16 M>
void ODIN<N, M>::set_synapse(u32 pre_neur_id, u32 post_neur_id, u8 val)
{
    RtosMutexGuard guard(mutex);
    start_gate_activity();
    synram->set_synapse(pre_neur_id, post_neur_id, val);
    stop_gate_activity();
}

template<u16 N, u16 M>
u8 ODIN<N, M>::get_synapse(u32 pre_neur_id, u32 post_neur_id)
{
    RtosMutexGuard guard(mutex);
    start_gate_activity();
    u8 val = synram->get_synapse(pre_neur_id, post_neur_id);
    stop_gate_activity();
    return val;
}

template<u16 N, u16 M>
void ODIN<N, M>::zero_memory()
{
    ODIN_regmem<N, M> reg_defaults{};
    set_regmem(reg_defaults);

    start_gate_activity();

    ODIN_neuron neuron_defaults{};
    neuron_defaults.lif.lif_izh_sel = 1;
    neuron_defaults.lif.neur_disable = 1;

    for (u32 neur_id = 0; neur_id < N; ++neur_id) {
        neuram->set_neuron(neur_id, neuron_defaults);
    }

    {
        constexpr u32 syn_words = powof2(M) * N / 8;
        RtosMutexGuard guard(mutex);
        for (u32 i = 0; i < syn_words; ++i) {
            synram->synapses[i] = 0;
        }
    }

    stop_gate_activity();
}