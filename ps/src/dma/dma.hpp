#pragma once
extern "C"
{
#include "FreeRTOS.h"
#include "xaxidma.h"
#include "xil_types.h"
}
#include <functional>
#include <memory>
#include <vector>

template<size_t BD_RING_SIZE = 0xFFFF, typename TX_WORD = u32, typename RX_WORD = u8>
class Dma
{
public:
    struct Cfg
    {
        const UINTPTR baseaddr;
        const u32 tx_intr_id;
        const u32 rx_intr_id;
        u32 max_rx_buffer_len;
    };

private:
    Cfg cfg;
    XAxiDma axi_dma;

    XAxiDma_BdRing* tx_ring_ptr;
    XAxiDma_BdRing* rx_ring_ptr;

    u8* tx_bd;
    u8* rx_bd;
    std::unique_ptr<u8[]> tx_bd_base;
    std::unique_ptr<u8[]> rx_bd_base;

    u32 max_tx_buffers;
    u32 max_rx_buffers;

public:
    Dma(const Cfg& dma_cfg);
    Dma(const Dma&) = delete;
    Dma& operator=(const Dma&) = delete;
    Dma(Dma&&) = delete;
    Dma& operator=(Dma&&) = delete;

    void reset();
    void update();

    u32 send_packet(std::unique_ptr<std::vector<TX_WORD>>&& buffer);
    u32 send_packet(std::vector<std::unique_ptr<std::vector<TX_WORD>>>&& buffers);

    u32 get_free_tx_buffers() const;
    u32 get_max_tx_buffers() const;
    u32 get_max_tx_buffer_len() const;
    u32 get_max_rx_buffers() const;
    u32 get_max_rx_buffer_len() const;
    void set_max_rx_buffer_len(u32 len);
    u32 get_rx_alloc_pending() const;

    using tx_callback_t = std::function<void(bool last)>;
    using rx_callback_t = std::function<void(std::unique_ptr<std::vector<RX_WORD>>&& buffer, bool last)>;
    tx_callback_t tx_callback;
    rx_callback_t rx_callback;
    static void do_nothing_tx_callback(bool) {}
    static void do_nothing_rx_callback(std::unique_ptr<std::vector<RX_WORD>>&&, bool) {}

private:
    u32 send_buffer(std::unique_ptr<std::vector<TX_WORD>>& buffer_obj, bool first, bool last);
    using intr_handler_t = void (*)(void*);
    void intr_setup(intr_handler_t intr_handler, u32 intr_id);
    void tx_setup();
    void tx_free();
    void tx_unalloc();
    static void tx_intr_handler(void* p);
    void rx_setup();
    void rx_alloc();
    static void rx_intr_handler(void* p);
    [[noreturn]] void dma_error(const char* msg) const;
};

#include "dma.tpp"
#include "dma_rx.tpp"
#include "dma_send.tpp"
#include "dma_tx.tpp"
