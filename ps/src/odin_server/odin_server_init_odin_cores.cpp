#include "dma/dma.hpp"
#include "event_router/event_router.hpp"
#include "odin/odin.hpp"
#include "odin_server.hpp"
#include "rtos_queue.hpp"
#include <memory>

void OdinServer::init_odin_cores(RtosQueue<RouterInput>& output_queue)
{
    u32 max_rx_buffer_len = 1024;

    // ODIN Core 0 configuration
    Dma<0xFFFF, u32, u8>::Cfg dma_cfg_0{ .baseaddr = XPAR_ODIN_CORE_0_AXI_DMA_0_BASEADDR,
                                         .tx_intr_id = XPAR_FABRIC_AXIDMA_0_MM2S_INTROUT_VEC_ID,
                                         .rx_intr_id = XPAR_FABRIC_AXIDMA_0_S2MM_INTROUT_VEC_ID,
                                         .max_rx_buffer_len = max_rx_buffer_len };
    ODIN<256, 8>::Cfg odin_cfg_0{ .core_id = 0,
                                  .baseaddr = XPAR_ODIN_CORE_0_AXI_BRAM_CTRL_0_S_AXI_BASEADDR,
                                  .dma = dma_cfg_0,
                                  .output_queue = output_queue };
    odin_cores.push_back(std::make_unique<ODIN<256, 8>>(odin_cfg_0));

    // ODIN Core 1 configuration
    Dma<0xFFFF, u32, u8>::Cfg dma_cfg_1{ .baseaddr = XPAR_ODIN_CORE_1_AXI_DMA_0_BASEADDR,
                                         .tx_intr_id = XPAR_FABRIC_AXIDMA_1_MM2S_INTROUT_VEC_ID,
                                         .rx_intr_id = XPAR_FABRIC_AXIDMA_1_S2MM_INTROUT_VEC_ID,
                                         .max_rx_buffer_len = max_rx_buffer_len };
    ODIN<256, 8>::Cfg odin_cfg_1{ .core_id = 1,
                                  .baseaddr = XPAR_ODIN_CORE_1_AXI_BRAM_CTRL_0_S_AXI_BASEADDR,
                                  .dma = dma_cfg_1,
                                  .output_queue = output_queue };
    odin_cores.push_back(std::make_unique<ODIN<256, 8>>(odin_cfg_1));

    // ODIN Core 2 configuration
    Dma<0xFFFF, u32, u8>::Cfg dma_cfg_2{ .baseaddr = XPAR_ODIN_CORE_2_AXI_DMA_0_BASEADDR,
                                         .tx_intr_id = XPAR_FABRIC_AXIDMA_2_MM2S_INTROUT_VEC_ID,
                                         .rx_intr_id = XPAR_FABRIC_AXIDMA_2_S2MM_INTROUT_VEC_ID,
                                         .max_rx_buffer_len = max_rx_buffer_len };
    ODIN<256, 8>::Cfg odin_cfg_2{ .core_id = 2,
                                  .baseaddr = XPAR_ODIN_CORE_2_AXI_BRAM_CTRL_0_S_AXI_BASEADDR,
                                  .dma = dma_cfg_2,
                                  .output_queue = output_queue };
    odin_cores.push_back(std::make_unique<ODIN<256, 8>>(odin_cfg_2));

    // ODIN Core 3 configuration
    Dma<0xFFFF, u32, u8>::Cfg dma_cfg_3{ .baseaddr = XPAR_ODIN_CORE_3_AXI_DMA_0_BASEADDR,
                                         .tx_intr_id = XPAR_FABRIC_AXIDMA_3_MM2S_INTROUT_VEC_ID,
                                         .rx_intr_id = XPAR_FABRIC_AXIDMA_3_S2MM_INTROUT_VEC_ID,
                                         .max_rx_buffer_len = max_rx_buffer_len };
    ODIN<256, 8>::Cfg odin_cfg_3{ .core_id = 3,
                                  .baseaddr = XPAR_ODIN_CORE_3_AXI_BRAM_CTRL_0_S_AXI_BASEADDR,
                                  .dma = dma_cfg_3,
                                  .output_queue = output_queue };
    odin_cores.push_back(std::make_unique<ODIN<256, 8>>(odin_cfg_3));

    // ODIN Core 4 configuration
    Dma<0xFFFF, u32, u8>::Cfg dma_cfg_4{ .baseaddr = XPAR_ODIN_CORE_4_AXI_DMA_0_BASEADDR,
                                         .tx_intr_id = XPAR_FABRIC_AXIDMA_4_MM2S_INTROUT_VEC_ID,
                                         .rx_intr_id = XPAR_FABRIC_AXIDMA_4_S2MM_INTROUT_VEC_ID,
                                         .max_rx_buffer_len = max_rx_buffer_len };
    ODIN<256, 8>::Cfg odin_cfg_4{ .core_id = 4,
                                  .baseaddr = XPAR_ODIN_CORE_4_AXI_BRAM_CTRL_0_S_AXI_BASEADDR,
                                  .dma = dma_cfg_4,
                                  .output_queue = output_queue };
    odin_cores.push_back(std::make_unique<ODIN<256, 8>>(odin_cfg_4));

    // odin_test::run_full_rw_test(XPAR_ODIN_CORE_0_AXI_BRAM_CTRL_0_S_AXI_BASEADDR);
    // odin_test::run_full_rw_test(XPAR_ODIN_CORE_1_AXI_BRAM_CTRL_0_S_AXI_BASEADDR);
    // odin_test::run_full_rw_test(XPAR_ODIN_CORE_2_AXI_BRAM_CTRL_0_S_AXI_BASEADDR);
    // odin_test::run_full_rw_test(XPAR_ODIN_CORE_3_AXI_BRAM_CTRL_0_S_AXI_BASEADDR);
    // odin_test::run_full_rw_test(XPAR_ODIN_CORE_4_AXI_BRAM_CTRL_0_S_AXI_BASEADDR);
}