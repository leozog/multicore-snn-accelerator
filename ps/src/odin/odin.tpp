#include "../event_router/router_input.hpp"
#include "../rtos_mutex_guard.hpp"
#include "FreeRTOS.h"
#include <memory>

template<u16 N, u16 M>
ODIN<N, M>::ODIN(const typename ODIN<N, M>::Cfg& odin_cfg)
    : cfg(odin_cfg)
    , regmem(reinterpret_cast<volatile ODIN_regmem<N, M>*>(cfg.baseaddr))
    , neuram(reinterpret_cast<volatile ODIN_neuram<N, M>*>(cfg.baseaddr + (0x1u << 15)))
    , synram(reinterpret_cast<volatile ODIN_synram<N, M>*>(cfg.baseaddr + (0x2u << 15)))
    , dma(cfg.dma)
    , packets_in_flight(0)
    , dropped_buffers(0)
    , dma_update_task_handle(nullptr)
{
    dma.tx_callback = [this](bool last) { this->tx_callback(last); };
    dma.rx_callback = [this](std::unique_ptr<std::vector<u8>>&& buffer, bool last) {
        this->rx_callback(std::move(buffer), last);
    };

    mutex = xSemaphoreCreateMutex();
    if (mutex == nullptr) {
        core_error("Failed to create mutex");
    }

    if (xTaskCreate([](void* p) { static_cast<ODIN*>(p)->dma_update_task(); },
                    "ODIN_DMA_UPDATE",
                    1024,
                    this,
                    DEFAULT_THREAD_PRIO,
                    &dma_update_task_handle) != pdPASS) {
        core_error("Failed to create ODIN_DMA_UPDATE task");
    }

    reset();

    Logger::info_force_uart("ODIN core %08X initialized", cfg.baseaddr);
}

template<u16 N, u16 M>
typename ODIN<N, M>::State ODIN<N, M>::get_state() const
{
    // Logger::debug_only_uart("tx_ring_ptr->PreCnt: %u", static_cast<unsigned>(dma.tx_ring_ptr->PreCnt));
    // Logger::debug_only_uart("tx_ring_ptr->HwCnt: %u", static_cast<unsigned>(dma.tx_ring_ptr->HwCnt));
    // Logger::debug_only_uart("tx_ring_ptr->PostCnt: %u", static_cast<unsigned>(dma.tx_ring_ptr->PostCnt));
    // Logger::debug_only_uart("rx_ring_ptr->PreCnt: %u", static_cast<unsigned>(dma.rx_ring_ptr->PreCnt));
    // Logger::debug_only_uart("rx_ring_ptr->HwCnt: %u", static_cast<unsigned>(dma.rx_ring_ptr->HwCnt));
    // Logger::debug_only_uart("rx_ring_ptr->PostCnt: %u", static_cast<unsigned>(dma.rx_ring_ptr->PostCnt));

    return regmem->busy ? State::BUSY : State::IDLE;
}

template<u16 N, u16 M>
void ODIN<N, M>::send_packet(std::unique_ptr<std::vector<u32>>&& buffer)
{
    RtosMutexGuard guard(mutex);

    if (!buffer) {
        core_error("Send packet null buffer");
    }

    u32 total_words = buffer->size();

    u32 sent_buffers_count = dma.send_packet(std::move(buffer));
    if (sent_buffers_count > 0U) {
        packets_in_flight += 1;
    }
    dropped_buffers += 1 - sent_buffers_count;

    Logger::debug("ODIN core %08X sent %u words", cfg.baseaddr, total_words);
}

template<u16 N, u16 M>
void ODIN<N, M>::send_packet(std::vector<std::unique_ptr<std::vector<u32>>>&& buffers)
{
    RtosMutexGuard guard(mutex);

    if (buffers.empty()) {
        core_error("Send packet empty buffers");
    }

    u32 total_words = 0;
    for (const auto& buffer : buffers) {
        if (!buffer) {
            core_error("Send packet null buffer");
        }
        total_words += buffer->size();
    }

    u32 sent_buffers_count = dma.send_packet(std::move(buffers));
    if (sent_buffers_count > 0U) {
        packets_in_flight += 1;
    }
    dropped_buffers += buffers.size() - sent_buffers_count;

    Logger::debug("ODIN core %08X sent %u words, packets in flight: %u", cfg.baseaddr, total_words, packets_in_flight);
}

template<u16 N, u16 M>
void ODIN<N, M>::dma_update_task()
{
    Logger::debug_force_uart("ODIN core %08X ODIN_DMA_UPDATE task started", cfg.baseaddr);
    while (true) {
        if (ulTaskNotifyTake(pdTRUE, 1) > 0) {
            RtosMutexGuard guard(mutex);
            dma.update();
            Logger::debug("ODIN core %08X DMA update done", cfg.baseaddr);
        }
        if (dropped_buffers > 0) {
            Logger::warning_force_uart(
                "ODIN core %08X dropped %u buffers due to full queues", cfg.baseaddr, dropped_buffers);
            dropped_buffers = 0;
        }
        vTaskDelay(1);
    }
}

template<u16 N, u16 M>
void ODIN<N, M>::reset()
{
    zero_memory();
    {
        RtosMutexGuard guard(mutex);

        Logger::debug("ODIN core %08X reset", cfg.baseaddr);
        start_gate_activity();
        regmem->sw_rst = 1;
        dma.reset();
        regmem->sw_rst = 1;
        stop_gate_activity();

        while (regmem->busy == 1) {
            taskYIELD();
        }
        packets_in_flight = 0;
    }
    Logger::debug("ODIN core %08X reset done", cfg.baseaddr);
}

template<u16 N, u16 M>
u32 ODIN<N, M>::get_max_tx_buffer_len() const
{
    // return dma.get_max_tx_buffer_len();
    return 512;
}

template<u16 N, u16 M>
void ODIN<N, M>::tx_callback(bool last)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    vTaskNotifyGiveFromISR(dma_update_task_handle, &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

template<u16 N, u16 M>
void ODIN<N, M>::rx_callback(std::unique_ptr<std::vector<u8>>&& buffer, bool last)
{
    if (last) {
        packets_in_flight -= 1;
    }

    BaseType_t higher_priority_task_woken = pdFALSE;

    RouterInput router_input;
    router_input.buffer_ptr = buffer.release();
    router_input.core_id = cfg.core_id;
    router_input.last = last;

    if (!cfg.output_queue.push_from_isr(router_input, &higher_priority_task_woken)) {
        // output queue full, dropping packet and memory leak :<
        // output queue shuold be so big it would never realistically fill up
        dropped_buffers += 1;
    }

    if (dma_update_task_handle != nullptr) {
        vTaskNotifyGiveFromISR(dma_update_task_handle, &higher_priority_task_woken);
    }
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

template<u16 N, u16 M>
[[noreturn]] void ODIN<N, M>::core_error(const char* msg) const
{
    Logger::error_force_uart("ODIN core %08X error: %s", cfg.baseaddr, msg);
    error_stop();
}
