template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
void Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::rx_setup()
{
    u32 status;

    rx_ring_ptr = XAxiDma_GetRxRing(&axi_dma);

    u32 bd_count = XAxiDma_BdRingCntCalc(XAXIDMA_BD_MINIMUM_ALIGNMENT, BD_RING_SIZE + 1);
    max_rx_buffers = bd_count;

    status = XAxiDma_BdRingCreate(rx_ring_ptr, (UINTPTR)rx_bd, (UINTPTR)rx_bd, XAXIDMA_BD_MINIMUM_ALIGNMENT, bd_count);
    if (status != XST_SUCCESS) {
        dma_error("Rx bd ring create failed");
    }

    XAxiDma_Bd bd_template;
    XAxiDma_BdClear(&bd_template);
    status = XAxiDma_BdRingClone(rx_ring_ptr, &bd_template);
    if (status != XST_SUCCESS) {
        dma_error("Rx clone bds failed");
    }

    rx_alloc();

    status = XAxiDma_BdRingSetCoalesce(rx_ring_ptr, 1, 1);
    if (status != XST_SUCCESS) {
        dma_error("Rx set coalesce failed");
    }

    status = XAxiDma_BdRingStart(rx_ring_ptr);
    if (status != XST_SUCCESS) {
        dma_error("Rx start bd ring failed");
    }
}

template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
void Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::rx_alloc()
{
    if (rx_ring_ptr->PostCnt != 0U) {
        if (XAxiDma_BdRingFree(rx_ring_ptr, rx_ring_ptr->PostCnt, rx_ring_ptr->PostHead) != XST_SUCCESS) {
            dma_error("Rx bd free failed");
        }
    }

    u32 count = rx_ring_ptr->FreeCnt;
    XAxiDma_Bd* start_bd = nullptr;
    if (XAxiDma_BdRingAlloc(rx_ring_ptr, count, &start_bd) != XST_SUCCESS) {
        dma_error("Rx bd alloc failed");
    }

    XAxiDma_Bd* cur_bd = start_bd;
    for (u32 i = 0; i < count; ++i) {
        auto buffer_uptr = std::make_unique<std::vector<RX_WORD>>(cfg.max_rx_buffer_len);
        std::vector<RX_WORD>* buffer_obj = buffer_uptr.get();

        if (XAxiDma_BdSetBufAddr(cur_bd, (UINTPTR)buffer_obj->data()) != XST_SUCCESS) {
            dma_error("Rx set buffer address failed");
        }
        const u32 buffer_len_bytes = cfg.max_rx_buffer_len * sizeof(RX_WORD);
        if (XAxiDma_BdSetLength(cur_bd, buffer_len_bytes, rx_ring_ptr->MaxTransferLen) != XST_SUCCESS) {
            dma_error("Rx set buffer length failed");
        }
        XAxiDma_BdSetId(cur_bd, (UINTPTR)buffer_uptr.release());

        XAxiDma_BdSetCtrl(cur_bd, 0U);

        cur_bd = (XAxiDma_Bd*)XAxiDma_BdRingNext(rx_ring_ptr, cur_bd);
    }

    if (XAxiDma_BdRingToHw(rx_ring_ptr, count, start_bd) != XST_SUCCESS) {
        dma_error("Rx bd to hw failed");
    }
}

template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
void Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::rx_intr_handler(void* p)
{
    Dma* dma = (Dma*)p;
    u32 irq_status = XAxiDma_BdRingGetIrq(dma->rx_ring_ptr);
    XAxiDma_BdRingAckIrq(dma->rx_ring_ptr, irq_status);

    UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();

    if ((irq_status & XAXIDMA_IRQ_ALL_MASK) == 0U) {
        taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
        return;
    }

    if ((irq_status & XAXIDMA_IRQ_ERROR_MASK) != 0U) {
        XAxiDma_BdRingDumpRegs(dma->rx_ring_ptr);
        dma->dma_error("Rx irq error");
        taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
        return;
    }

    if ((irq_status & (XAXIDMA_IRQ_DELAY_MASK | XAXIDMA_IRQ_IOC_MASK)) != 0U) {
        XAxiDma_Bd* start_bd = nullptr;
        const u32 bd_count = XAxiDma_BdRingFromHw(dma->rx_ring_ptr, XAXIDMA_ALL_BDS, &start_bd);

        XAxiDma_Bd* cur_bd = start_bd;
        for (u32 i = 0; i < bd_count; ++i) {
            const u32 rx_status = XAxiDma_BdGetSts(cur_bd);
            const bool last = (rx_status & XAXIDMA_BD_STS_RXEOF_MASK) != 0U;

            const UINTPTR id = XAxiDma_BdGetId(cur_bd);
            if (id == 0U) {
                dma->dma_error("Rx buffer id 0");
                taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
                return;
            }
            std::unique_ptr<std::vector<RX_WORD>> buffer_uptr(reinterpret_cast<std::vector<RX_WORD>*>(id));

            u32 actual_len_words =
                XAxiDma_BdGetActualLength(cur_bd, dma->rx_ring_ptr->MaxTransferLen) / sizeof(RX_WORD);
            if (last && (actual_len_words > 0U)) {
                actual_len_words -= 1U; // remove EOF marker if present
            }
            buffer_uptr->resize(actual_len_words);

            dma->rx_callback(std::move(buffer_uptr), last);

            cur_bd = (XAxiDma_Bd*)XAxiDma_BdRingNext(dma->rx_ring_ptr, cur_bd);
        }
    }
    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
}
