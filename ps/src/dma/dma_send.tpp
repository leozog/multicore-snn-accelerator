#include <vector>
template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
u32 Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::send_packet(std::unique_ptr<std::vector<TX_WORD>>&& buffer)
{
    XAxiDma_BdRingIntDisable(tx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);

    if (!buffer) {
        dma_error("Send packet null buffer");
    }

    const bool first = true;
    const bool last = true;

    if (get_free_tx_buffers() == 0U) {
        Logger::warning("DMA %08X send_packet no free tx buffers, dropping packet", cfg.baseaddr);
        XAxiDma_BdRingIntEnable(tx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);
        return 0;
    }

    u32 status = send_buffer(buffer, first, last);
    if (status != XST_SUCCESS) {
        Logger::warning("DMA %08X send_packet failed to send buffer, dropping packet", cfg.baseaddr);
        XAxiDma_BdRingIntEnable(tx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);
        return 0;
    }

    if (tx_ring_ptr->HwCnt == 0U) {
        status = XAxiDma_BdRingToHw(tx_ring_ptr, 1, tx_ring_ptr->PreHead);
        if (status != XST_SUCCESS) {
            dma_error("Tx bd to hw failed");
        }
    }

    XAxiDma_BdRingIntEnable(tx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);
    return 1;
}

template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
u32 Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::send_packet(std::vector<std::unique_ptr<std::vector<TX_WORD>>>&& buffers)
{
    XAxiDma_BdRingIntDisable(tx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);

    u32 to_send = 0U;
    for (u32 i = 0; i < buffers.size(); ++i) {
        if (!buffers[i]) {
            dma_error("Send packet null buffer");
        }

        const bool first = (i == 0U);
        bool last = (i == (buffers.size() - 1U));

        u32 free_buffers = get_free_tx_buffers();
        if (free_buffers == 0U) {
            Logger::warning("DMA %08X send_packet no free tx buffers for buffer %u/%u, dropping rest of packet",
                            cfg.baseaddr,
                            static_cast<unsigned>(i + 1U),
                            static_cast<unsigned>(buffers.size()));
            break;
        } else if (free_buffers == 1U) {
            last = true;
        }

        u32 status = send_buffer(buffers[i], first, last);
        if (status != XST_SUCCESS) {
            Logger::warning("DMA %08X send_packet failed to send buffer %u/%u, dropping rest of packet",
                            cfg.baseaddr,
                            static_cast<unsigned>(i + 1U),
                            static_cast<unsigned>(buffers.size()));
            break;
        } else {
            to_send += 1U;
        }
    }

    if (tx_ring_ptr->HwCnt == 0U) {
        u32 status = XAxiDma_BdRingToHw(tx_ring_ptr, to_send, tx_ring_ptr->PreHead);
        if (status != XST_SUCCESS) {
            dma_error("Tx bd to hw failed");
        }
    }

    XAxiDma_BdRingIntEnable(tx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);
    return to_send;
}

template<size_t BD_RING_SIZE, typename TX_WORD, typename RX_WORD>
u32 Dma<BD_RING_SIZE, TX_WORD, RX_WORD>::send_buffer(std::unique_ptr<std::vector<TX_WORD>>& buffer_uptr,
                                                     bool first,
                                                     bool last)
{
    if (!buffer_uptr) {
        dma_error("Send buffer_obj null payload");
    }

    if (XAxiDma_BdRingGetFreeCnt(tx_ring_ptr) <= 0) {
        return XST_FAILURE;
    }

    if (last) {
        buffer_uptr->push_back(0); // add EOF marker
    }

    const u32 buffer_len_bytes = static_cast<u32>(buffer_uptr->size() * sizeof(TX_WORD));
    if (buffer_len_bytes > tx_ring_ptr->MaxTransferLen) {
        dma_error("Send buffer_data too large");
    }

    XAxiDma_Bd* cur_bd = nullptr;
    u32 status = XAxiDma_BdRingAlloc(tx_ring_ptr, 1, &cur_bd);
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    status = XAxiDma_BdSetBufAddr(cur_bd, (UINTPTR)buffer_uptr->data());
    if (status != XST_SUCCESS) {
        dma_error("Send buffer_data set buffer_data address failed");
    }
    status = XAxiDma_BdSetLength(cur_bd, buffer_len_bytes, tx_ring_ptr->MaxTransferLen);
    if (status != XST_SUCCESS) {
        dma_error("Send buffer_data set buffer_data length failed");
    }
    XAxiDma_BdSetId(cur_bd, (UINTPTR)buffer_uptr.release());

    u32 buffer_ctrl = 0U;
    if (first) {
        buffer_ctrl |= XAXIDMA_BD_CTRL_TXSOF_MASK;
    }
    if (last) {
        buffer_ctrl |= XAXIDMA_BD_CTRL_TXEOF_MASK; // mark the last word with EOF
    }
    XAxiDma_BdSetCtrl(cur_bd, buffer_ctrl);

    return XST_SUCCESS;
}