// Copyright (C) 2016-2019 Université catholique de Louvain (UCLouvain), Belgium.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 2.0 (the "License"); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-2.0/. The software, hardware and materials
// distributed under this License are provided in the hope that it will be useful
// on an as is basis, without warranties or conditions of any kind, either
// expressed or implied; without even the implied warranty of merchantability or
// fitness for a particular purpose. See the Solderpad Hardware License for more
// detailed permissions and limitations.
//------------------------------------------------------------------------------
//
// "spi_slave.v" - ODIN REG slave module
// 
// Project: ODIN - An online-learning digital spiking neuromorphic processor
//
// Author:  C. Frenkel, Université catholique de Louvain (UCLouvain), 04/2017
//
// Cite/paper: C. Frenkel, M. Lefebvre, J.-D. Legat and D. Bol, "A 0.086-mm² 12.7-pJ/SOP 64k-Synapse 256-Neuron Online-Learning
//             Digital Spiking Neuromorphic Processor in 28-nm CMOS," IEEE Transactions on Biomedical Circuits and Systems,
//             vol. 13, no. 1, pp. 145-158, 2019.
//
//------------------------------------------------------------------------------


module registers #(
    parameter N = 256,
    parameter M = 8
)(

    // Global inputs -----------------------------------------
    input  wire                 CLK,
    input  wire                 RST,

    // Global software reset  --------------------------------
    output reg                  REGMEM_SW_RST,

    // Input from the controller  ----------------------------
    input wire                  ODIN_BUSY,
    input wire                  REGMEM_GATE_ACTIVITY_sync,

    // REG slave interface ------------------------------------
    // input  wire                 CLK,
    // output wire                 MISO,
    // input  wire                 MOSI,

    // Control interface for readback -------------------------
    // output reg                  CTRL_READBACK_EVENT,
    // output reg                  CTRL_PROG_EVENT,
    // output reg  [      2*M-1:0] CTRL_BRAM_MEM_ADDR,
    // output reg  [          1:0] CTRL_OP_CODE,
    // output reg  [      2*M-1:0] CTRL_PROG_DATA,
    // input  wire [         31:0] SYNARRAY_RDATA,
    // input  wire [        127:0] NEUR_STATE,
    
    input  wire           BRAM_MEM_EN,
    input  wire  [  3:0]  BRAM_MEM_WE,
    input  wire  [ 16:0]  BRAM_MEM_ADDR,
    input  wire  [ 31:0]  BRAM_MEM_DIN,
    output reg   [ 31:0]  BRAM_MEM_REGMEM_DOUT,
    output reg            BRAM_MEM_REGMEM_RD,
    

    // Configuration registers output -------------------------
    output reg                  REGMEM_GATE_ACTIVITY,
    output reg                  REGMEM_OPEN_LOOP,
    output reg  [        N-1:0] REGMEM_SYN_SIGN,
    output reg  [         19:0] REGMEM_BURST_TIMEREF,
    output reg                  REGMEM_OUT_AER_MONITOR_EN,
    output reg                  REGMEM_AER_SRC_CTRL_nNEUR,
    output reg  [        M-1:0] REGMEM_MONITOR_NEUR_ADDR,
    output reg  [        M-1:0] REGMEM_MONITOR_SYN_ADDR,
    output reg                  REGMEM_UPDATE_UNMAPPED_SYN,
	output reg                  REGMEM_PROPAGATE_UNMAPPED_SYN,
	output reg                  REGMEM_SDSP_ON_SYN_STIM
); 
    genvar i;
    integer j;
    
    reg [31:0] syn_sign_readout;

    wire [12:0] addr = BRAM_MEM_ADDR[14:2];
    wire en = BRAM_MEM_EN && (BRAM_MEM_ADDR[16:15] == 2'b00) && (BRAM_MEM_ADDR[1:0] == 2'b00);
    wire [31:0] we;
    generate
        for (i=0; i<4; i=i+1) begin
            assign we[8*i +: 8] = {8{en}} & {8{BRAM_MEM_WE[i]}};
        end
    endgenerate
    wire [31:0] din = BRAM_MEM_DIN & we;
    
    always @(posedge CLK, posedge RST)
        if (RST) BRAM_MEM_REGMEM_RD <= 0;
        else     BRAM_MEM_REGMEM_RD <= en && ~(|BRAM_MEM_WE);
    
	//----------------------------------------------------------------------------------
	//	Output config. registers
	//----------------------------------------------------------------------------------
  
    //REGMEM_GATE_ACTIVITY - 1 bit - address 0
    always @(posedge CLK, posedge RST)
        if      (RST)                                                                                 REGMEM_GATE_ACTIVITY <= 1'b0;
        else if (addr == 13'd0)                                                                       REGMEM_GATE_ACTIVITY <= (REGMEM_GATE_ACTIVITY & ~we[0]) | din[0];
        else                                                                                          REGMEM_GATE_ACTIVITY <= REGMEM_GATE_ACTIVITY;
        
    //REGMEM_OPEN_LOOP - 1 bit - address 1
    always @(posedge CLK, posedge RST)
        if      (RST)                                                                                 REGMEM_OPEN_LOOP <= 1'b0;
        else if (addr == 13'd1)                                                                       REGMEM_OPEN_LOOP <= (REGMEM_OPEN_LOOP & ~we[0]) | din[0];
        else                                                                                          REGMEM_OPEN_LOOP <= REGMEM_OPEN_LOOP;
    
    //REGMEM_SYN_SIGN - 256 bits - addresses 2 to 17
    generate
        for (i=0; i<(N>>5); i=i+1) begin
            always @(posedge CLK, posedge RST)
                if      (RST)                                                                         REGMEM_SYN_SIGN[32*i+31:32*i] <= 32'b0;
                else if (addr == (13'd2+i))                                                           REGMEM_SYN_SIGN[32*i+31:32*i] <= (REGMEM_SYN_SIGN[32*i+31:32*i] & ~we) | din;
                else                                                                                  REGMEM_SYN_SIGN[32*i+31:32*i] <= REGMEM_SYN_SIGN[32*i+31:32*i];
        end
    endgenerate
    
    //REGMEM_BURST_TIMEREF - 20 bits - address 18
    always @(posedge CLK, posedge RST)
        if      (RST)                                                                                 REGMEM_BURST_TIMEREF <= 20'b0;
        else if (addr == 13'd18)                                                                      REGMEM_BURST_TIMEREF <= (REGMEM_BURST_TIMEREF & ~we[19:0]) | din[19:0];
        else                                                                                          REGMEM_BURST_TIMEREF <= REGMEM_BURST_TIMEREF;
    
    //REGMEM_AER_SRC_CTRL_nNEUR - 1 bit - address 19
    always @(posedge CLK, posedge RST)
        if      (RST)                                                                                 REGMEM_AER_SRC_CTRL_nNEUR <= 1'b0;
        else if (addr == 13'd19)                                                                      REGMEM_AER_SRC_CTRL_nNEUR <= (REGMEM_AER_SRC_CTRL_nNEUR & ~we[0]) | din[0];
        else                                                                                          REGMEM_AER_SRC_CTRL_nNEUR <= REGMEM_AER_SRC_CTRL_nNEUR;
    
    //REGMEM_OUT_AER_MONITOR_EN - 1 bit - address 20
    always @(posedge CLK, posedge RST)
        if      (RST)                                                                                 REGMEM_OUT_AER_MONITOR_EN <= 1'b0;
        else if (addr == 13'd20)                                                                      REGMEM_OUT_AER_MONITOR_EN <= (REGMEM_OUT_AER_MONITOR_EN & ~we[0]) | din[0];
        else                                                                                          REGMEM_OUT_AER_MONITOR_EN <= REGMEM_OUT_AER_MONITOR_EN;
    
    //REGMEM_MONITOR_NEUR_ADDR - M bit - address 21
    always @(posedge CLK, posedge RST)
        if      (RST)                                                                                 REGMEM_MONITOR_NEUR_ADDR <= {M{1'b0}};
        else if (addr == 13'd21)                                                                      REGMEM_MONITOR_NEUR_ADDR <= (REGMEM_MONITOR_NEUR_ADDR & ~we[M-1:0]) | din[M-1:0];
        else                                                                                          REGMEM_MONITOR_NEUR_ADDR <= REGMEM_MONITOR_NEUR_ADDR;
    
    //REGMEM_MONITOR_SYN_ADDR - M bit - address 22
    always @(posedge CLK, posedge RST)
        if      (RST)                                                                                 REGMEM_MONITOR_SYN_ADDR <= {M{1'b0}};
        else if (addr == 13'd22)                                                                      REGMEM_MONITOR_SYN_ADDR <= (REGMEM_MONITOR_SYN_ADDR & ~we[M-1:0]) | din[M-1:0];
        else                                                                                          REGMEM_MONITOR_SYN_ADDR <= REGMEM_MONITOR_SYN_ADDR;

    //REGMEM_UPDATE_UNMAPPED_SYN - 1 bit - address 23
    always @(posedge CLK, posedge RST)
        if      (RST)                                                                                 REGMEM_UPDATE_UNMAPPED_SYN <= 1'b0;
        else if (addr == 13'd23)                                                                      REGMEM_UPDATE_UNMAPPED_SYN <= (REGMEM_UPDATE_UNMAPPED_SYN & ~we[0]) | din[0];
        else                                                                                          REGMEM_UPDATE_UNMAPPED_SYN <= REGMEM_UPDATE_UNMAPPED_SYN;
    
	//REGMEM_PROPAGATE_UNMAPPED_SYN - 1 bit - address 24
    always @(posedge CLK, posedge RST)
        if      (RST)                                                                                 REGMEM_PROPAGATE_UNMAPPED_SYN <= 1'b0;
        else if (addr == 13'd24)                                                                      REGMEM_PROPAGATE_UNMAPPED_SYN <= (REGMEM_PROPAGATE_UNMAPPED_SYN & ~we[0]) | din[0];
        else                                                                                          REGMEM_PROPAGATE_UNMAPPED_SYN <= REGMEM_PROPAGATE_UNMAPPED_SYN;
    
	//REGMEM_SDSP_ON_SYN_STIM - 1 bit - address 25
    always @(posedge CLK, posedge RST)
        if      (RST)                                                                                 REGMEM_SDSP_ON_SYN_STIM <= 1'b0;
        else if (addr == 13'd25)                                                                      REGMEM_SDSP_ON_SYN_STIM <= (REGMEM_SDSP_ON_SYN_STIM & ~we[0]) | din[0];
        else                                                                                          REGMEM_SDSP_ON_SYN_STIM <= REGMEM_SDSP_ON_SYN_STIM;
	
    /*                                                 *
     * Some address room for other params if necessary *
     *                                                 */

    // Reset - 1 bit - address 26 - write 1 to trigger a software reset pulse of the core and peripheral modules through RSTN_OUT
    always @(posedge CLK, posedge RST) begin
        if (RST)                                                                                      REGMEM_SW_RST <= 1'b0;
        else if (addr == 13'd26)                                                                      REGMEM_SW_RST <= din[0];
        else                                                                                          REGMEM_SW_RST <= 1'b0;
    end

    // Busy flag - 1 bit - address 27 - read-only, reflects the ODIN_BUSY output

    //----------------------------------------------------------------------------------
    // Register readout logic
    //----------------------------------------------------------------------------------
    
    always @(*) begin
        syn_sign_readout = 32'b0;
        for (j = 0; j < (N>>5); j = j + 1) begin
            if (addr == (13'd2 + j[12:0]))
                syn_sign_readout = REGMEM_SYN_SIGN[32*j +: 32];
        end
    end
    
    always @(*) begin
        case (addr)
            13'd0:   BRAM_MEM_REGMEM_DOUT = {31'b0, REGMEM_GATE_ACTIVITY_sync};
            13'd1:   BRAM_MEM_REGMEM_DOUT = {31'b0, REGMEM_OPEN_LOOP};
            13'd18:  BRAM_MEM_REGMEM_DOUT = {12'b0, REGMEM_BURST_TIMEREF};
            13'd19:  BRAM_MEM_REGMEM_DOUT = {31'b0, REGMEM_AER_SRC_CTRL_nNEUR};
            13'd20:  BRAM_MEM_REGMEM_DOUT = {31'b0, REGMEM_OUT_AER_MONITOR_EN};
            13'd21:  BRAM_MEM_REGMEM_DOUT = {24'b0, REGMEM_MONITOR_NEUR_ADDR};
            13'd22:  BRAM_MEM_REGMEM_DOUT = {24'b0, REGMEM_MONITOR_SYN_ADDR};
            13'd23:  BRAM_MEM_REGMEM_DOUT = {31'b0, REGMEM_UPDATE_UNMAPPED_SYN};
            13'd24:  BRAM_MEM_REGMEM_DOUT = {31'b0, REGMEM_PROPAGATE_UNMAPPED_SYN};
            13'd25:  BRAM_MEM_REGMEM_DOUT = {31'b0, REGMEM_SDSP_ON_SYN_STIM};
            13'd26:  BRAM_MEM_REGMEM_DOUT = {32'b0};
            13'd27:  BRAM_MEM_REGMEM_DOUT = {31'b0, ODIN_BUSY};
            default: BRAM_MEM_REGMEM_DOUT = syn_sign_readout;
        endcase
    end

endmodule
