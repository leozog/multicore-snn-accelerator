#pragma once
#include "arch/sys_arch.h"
extern "C"
{
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "xil_types.h"
}
#include "../dma/dma.hpp"
#include "../event_router/router_input.hpp"
#include "../logger.hpp"
#include "../rtos_queue.hpp"
#include "odin_mem.hpp"
#include <memory>
#include <vector>

template<u16 N, u16 M>
class ODIN
{
public:
    struct Cfg
    {
        const u8 core_id;
        const UINTPTR baseaddr;
        const Dma<0xFFFF, u32, u8>::Cfg dma;
        RtosQueue<RouterInput>& output_queue;
    };

private:
    const Cfg cfg;

    volatile ODIN_regmem<N, M>* regmem;
    volatile ODIN_neuram<N, M>* neuram;
    volatile ODIN_synram<N, M>* synram;

    Dma<0xFFFF, u32, u8> dma;

    TaskHandle_t dma_update_task_handle;
    SemaphoreHandle_t mutex;

    volatile int packets_in_flight;
    volatile int dropped_buffers;

public:
    ODIN(const Cfg& cfg);
    ODIN(const ODIN&) = delete;
    ODIN& operator=(const ODIN&) = delete;
    ODIN(ODIN&&) = delete;
    ODIN& operator=(ODIN&&) = delete;

    enum class State
    {
        IDLE,
        BUSY
    };
    State get_state() const;

    void send_packet(std::unique_ptr<std::vector<u32>>&& buffer);
    void send_packet(std::vector<std::unique_ptr<std::vector<u32>>>&& buffers);

    void reset();
    void zero_memory();

    void set_regmem(const ODIN_regmem<N, M>& val);
    void set_neuram(const ODIN_neuram<N, M>& val);
    void set_synram(const ODIN_synram<N, M>& val);
    ODIN_regmem<N, M> get_regmem();
    ODIN_neuram<N, M> get_neuram();
    ODIN_synram<N, M> get_synram();
    void set_neuron(u32 neur_id, const ODIN_neuron& val);
    ODIN_neuron get_neuron(u32 neur_id);
    void set_synapse(u32 pre_neur_id, u32 post_neur_id, u8 val);
    u8 get_synapse(u32 pre_neur_id, u32 post_neur_id);

    u32 get_max_tx_buffer_len() const;

private:
    void dma_update_task();

    void tx_callback(bool last);
    void rx_callback(std::unique_ptr<std::vector<u8>>&& buffer, bool last);

    void start_gate_activity();
    void stop_gate_activity();

    [[noreturn]] void core_error(const char* msg) const;
};

#include "odin.tpp"
#include "odin_set_get.tpp"
