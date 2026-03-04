#include <memory>
#include <vector>
template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
void Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::tx_setup()
{
    u32 status;

    tx_ring_ptr = XAxiDma_GetTxRing(&axi_dma);

    u32 bd_count = XAxiDma_BdRingCntCalc(XAXIDMA_BD_MINIMUM_ALIGNMENT, BD_RING_SIZE + 1);
    max_tx_buffers = bd_count;

    status = XAxiDma_BdRingCreate(tx_ring_ptr, (UINTPTR)tx_bd, (UINTPTR)tx_bd, XAXIDMA_BD_MINIMUM_ALIGNMENT, bd_count);
    if (status != XST_SUCCESS) {
        dma_error("Tx bd ring create failed");
    }

    XAxiDma_Bd bd_template;
    XAxiDma_BdClear(&bd_template);
    status = XAxiDma_BdRingClone(tx_ring_ptr, &bd_template);
    if (status != XST_SUCCESS) {
        dma_error("Tx clone bds failed");
    }

    status = XAxiDma_BdRingSetCoalesce(tx_ring_ptr, 1, 1);
    if (status != XST_SUCCESS) {
        dma_error("Tx set coalesce failed");
    }

    status = XAxiDma_BdRingStart(tx_ring_ptr);
    if (status != XST_SUCCESS) {
        dma_error("Tx start bd ring failed");
    }
}

template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
void Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::tx_free()
{
    u32 count = tx_ring_ptr->PostCnt;
    if (count == 0U) {
        return;
    }

    XAxiDma_Bd* cur_bd = tx_ring_ptr->PostHead;
    for (u32 i = 0; i < count; ++i) {
        const UINTPTR id = XAxiDma_BdGetId(cur_bd);
        if (id != 0U) {
            std::unique_ptr<std::vector<TX_WORD>> buffer_uptr(reinterpret_cast<std::vector<TX_WORD>*>(id));
            buffer_uptr.reset();
        }

        cur_bd = (XAxiDma_Bd*)XAxiDma_BdRingNext(tx_ring_ptr, cur_bd);
    }

    if (XAxiDma_BdRingFree(tx_ring_ptr, (int)count, tx_ring_ptr->PostHead) != XST_SUCCESS) {
        dma_error("Tx bd free failed");
    }

    Logger::debug("Freed %u tx buffers", static_cast<unsigned>(count));
}

template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
void Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::tx_unalloc()
{
    u32 count = tx_ring_ptr->PreCnt;
    if (count == 0U) {
        return;
    }

    XAxiDma_Bd* cur_bd = tx_ring_ptr->PreHead;
    for (u32 i = 0; i < count; ++i) {
        const UINTPTR id = XAxiDma_BdGetId(cur_bd);
        if (id != 0U) {
            std::unique_ptr<std::vector<TX_WORD>> buffer_uptr(reinterpret_cast<std::vector<TX_WORD>*>(id));
            buffer_uptr.reset();
        }

        cur_bd = (XAxiDma_Bd*)XAxiDma_BdRingNext(tx_ring_ptr, cur_bd);
    }

    if (XAxiDma_BdRingUnAlloc(tx_ring_ptr, (int)count, tx_ring_ptr->PreHead) != XST_SUCCESS) {
        dma_error("Tx bd unalloc failed");
    }

    Logger::debug("Unallocated %u tx buffers", static_cast<unsigned>(count));
}

template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
void Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::tx_intr_handler(void* p)
{
    Dma* dma = (Dma*)p;
    u32 irq_status = XAxiDma_BdRingGetIrq(dma->tx_ring_ptr);
    XAxiDma_BdRingAckIrq(dma->tx_ring_ptr, irq_status);

    UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();

    if ((irq_status & XAXIDMA_IRQ_ALL_MASK) == 0U) {
        taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
        return;
    }

    if ((irq_status & XAXIDMA_IRQ_ERROR_MASK) != 0U) {
        XAxiDma_BdRingDumpRegs(dma->tx_ring_ptr);
        dma->dma_error("Tx irq error");
        taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
        return;
    }

    if ((irq_status & (XAXIDMA_IRQ_DELAY_MASK | XAXIDMA_IRQ_IOC_MASK)) != 0U) {
        XAxiDma_Bd* start_bd = nullptr;
        const u32 bd_count = XAxiDma_BdRingFromHw(dma->tx_ring_ptr, XAXIDMA_ALL_BDS, &start_bd);

        XAxiDma_Bd* cur_bd = start_bd;
        for (u32 i = 0; i < bd_count; ++i) {
            const u32 tx_ctrl = XAxiDma_BdGetCtrl(cur_bd);
            const bool last = (tx_ctrl & XAXIDMA_BD_CTRL_TXEOF_MASK) != 0U;

            dma->tx_callback(last);

            cur_bd = (XAxiDma_Bd*)XAxiDma_BdRingNext(dma->tx_ring_ptr, cur_bd);
        }

        if (dma->tx_ring_ptr->HwCnt == 0U) {
            XAxiDma_Bd* cur_bd = dma->tx_ring_ptr->PreHead;
            u32 to_send = 0U;
            for (u32 i = 0; i < dma->tx_ring_ptr->PreCnt; ++i) {
                const u32 tx_ctrl = XAxiDma_BdGetCtrl(cur_bd);
                const bool last = (tx_ctrl & XAXIDMA_BD_CTRL_TXEOF_MASK) != 0U;

                if (last) {
                    to_send = i + 1U;
                    break;
                }

                cur_bd = (XAxiDma_Bd*)XAxiDma_BdRingNext(dma->tx_ring_ptr, cur_bd);
            }

            if (to_send > 0U) {
                u32 status = XAxiDma_BdRingToHw(dma->tx_ring_ptr, to_send, dma->tx_ring_ptr->PreHead);
                if (status != XST_SUCCESS) {
                    dma->dma_error("Tx bd to hw failed in irq");
                }
            }
        }
    }
    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
}
