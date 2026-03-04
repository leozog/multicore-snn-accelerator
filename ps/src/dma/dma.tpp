#include <memory>
extern "C"
{
#include "sys/_intsup.h"
#include "xaxidma_bd.h"
#include "xdebug.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xil_util.h"
#include "xinterrupt_wrap.h"
#include "xparameters.h"
#include "xscugic.h"
}
#include "logger.hpp"

#define XAXIDMA_RESET_TIMEOUT 50000000u

template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::Dma(const Cfg& dma_cfg)
    : cfg(dma_cfg)
    , tx_ring_ptr(nullptr)
    , rx_ring_ptr(nullptr)
    , tx_bd(nullptr)
    , rx_bd(nullptr)
    , tx_bd_base(nullptr)
    , rx_bd_base(nullptr)
    , max_tx_buffers(0)
    , max_rx_buffers(0)
    , tx_callback(Dma::do_nothing_tx_callback)
    , rx_callback(Dma::do_nothing_rx_callback)
{
    Logger::debug_force_uart("Initializing DMA %08X", cfg.baseaddr);

    tx_bd_base = std::make_unique<u8[]>(BD_RING_SIZE + XAXIDMA_BD_MINIMUM_ALIGNMENT);
    tx_bd = reinterpret_cast<u8*>((reinterpret_cast<uintptr_t>(tx_bd_base.get()) + XAXIDMA_BD_MINIMUM_ALIGNMENT - 1) &
                                  ~(XAXIDMA_BD_MINIMUM_ALIGNMENT - 1));

    rx_bd_base = std::make_unique<u8[]>(BD_RING_SIZE + XAXIDMA_BD_MINIMUM_ALIGNMENT);
    rx_bd = reinterpret_cast<u8*>((reinterpret_cast<uintptr_t>(rx_bd_base.get()) + XAXIDMA_BD_MINIMUM_ALIGNMENT - 1) &
                                  ~(XAXIDMA_BD_MINIMUM_ALIGNMENT - 1));

    XAxiDma_Config* config = XAxiDma_LookupConfigBaseAddr(cfg.baseaddr);
    if (!config) {
        dma_error("No config found");
    }

    if (XAxiDma_CfgInitialize(&axi_dma, config)) {
        dma_error("Couldn't initialize config");
    }

    if (!XAxiDma_HasSg(&axi_dma)) {
        dma_error("Device configured as simple mode");
    }

    tx_setup();
    rx_setup();

    intr_setup(Dma::tx_intr_handler, cfg.tx_intr_id);
    intr_setup(Dma::rx_intr_handler, cfg.rx_intr_id);

    XAxiDma_BdRingAckIrq(tx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);
    XAxiDma_BdRingAckIrq(rx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);

    XAxiDma_BdRingIntEnable(tx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);
    XAxiDma_BdRingIntEnable(rx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);

    Logger::info_force_uart("DMA %08X initialized", cfg.baseaddr);
}

template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
void Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::intr_setup(intr_handler_t intr_handler, u32 intr_id)
{
    XScuGic_SetPriTrigTypeByDistAddr(XPAR_SCUGIC_0_DIST_BASEADDR, intr_id, 0xA0U, 0x3U);

    XScuGic_RegisterHandler(XPAR_SCUGIC_0_CPU_BASEADDR,
                            static_cast<s32>(intr_id),
                            reinterpret_cast<Xil_InterruptHandler>(intr_handler),
                            static_cast<void*>(this));

    XScuGic_EnableIntr(XPAR_SCUGIC_0_DIST_BASEADDR, intr_id);
}

template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
void Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::reset()
{
    XAxiDma_BdRingIntDisable(tx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);
    XAxiDma_BdRingIntDisable(rx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);

    Logger::debug_only_uart("Unallocating unsent tx packets");
    tx_unalloc();

    XAxiDma_BdRingIntEnable(tx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);
    XAxiDma_BdRingIntEnable(rx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);
}

template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
void Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::update()
{
    if (rx_ring_ptr->PostCnt != 0U) {
        XAxiDma_BdRingIntDisable(rx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);
        rx_alloc();
        XAxiDma_BdRingIntEnable(rx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);
    }
    if (tx_ring_ptr->PostCnt != 0U) {
        XAxiDma_BdRingIntDisable(tx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);
        tx_free();
        XAxiDma_BdRingIntEnable(tx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);
    }
}

template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
u32 Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::get_free_tx_buffers() const
{
    return XAxiDma_BdRingGetFreeCnt(tx_ring_ptr);
}

template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
u32 Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::get_max_tx_buffers() const
{
    return max_tx_buffers;
}

template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
u32 Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::get_max_tx_buffer_len() const
{
    return tx_ring_ptr->MaxTransferLen / sizeof(TX_WORD);
}

template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
u32 Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::get_max_rx_buffers() const
{
    return max_rx_buffers;
}

template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
u32 Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::get_max_rx_buffer_len() const
{
    return cfg.max_rx_buffer_len;
}

template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
void Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::set_max_rx_buffer_len(u32 len)
{
    const u32 len_bytes = static_cast<u32>(len * sizeof(RX_WORD));
    if (len_bytes > rx_ring_ptr->MaxTransferLen) {
        dma_error("Max rx buffer len too big");
    }
    cfg.max_rx_buffer_len = len;
}

template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
u32 Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::get_rx_alloc_pending() const
{
    return rx_ring_ptr->PostCnt;
}

template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
void Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::dma_error(const char* msg) const
{
    Logger::error_only_uart("DMA %08X error: %s", cfg.baseaddr, msg);
    error_stop();
}
