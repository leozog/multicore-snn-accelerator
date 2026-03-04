// Copyright (C) 2016-2019 Universit� catholique de Louvain (UCLouvain), Belgium.
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
// "ODIN.v" - ODIN Spiking Neural Network (SNN) top-level module
// 
// Project: ODIN - An online-learning digital spiking neuromorphic processor
//
// Author:  C. Frenkel, Universit� catholique de Louvain (UCLouvain), 04/2017
//
// Cite/paper: C. Frenkel, M. Lefebvre, J.-D. Legat and D. Bol, "A 0.086-mm� 12.7-pJ/SOP 64k-Synapse 256-Neuron Online-Learning
//             Digital Spiking Neuromorphic Processor in 28-nm CMOS," IEEE Transactions on Biomedical Circuits and Systems,
//             vol. 13, no. 1, pp. 145-158, 2019.
//
//------------------------------------------------------------------------------


module ODIN #(
	parameter N = 256,
	parameter M = 8
)(
    // Global input     -------------------------------
    (* X_INTERFACE_INFO = "xilinx.com:signal:clock:1.0 CLK CLK" *)
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 1000000, FREQ_TOLERANCE_HZ -1, ASSOCIATED_RESET RSTN, ASSOCIATED_BUSIF BRAM_MEM:AERIN:AEROUT" *)
    input wire                          CLK,
    (* X_INTERFACE_INFO = "xilinx.com:signal:reset:1.0 RSTN RSTN" *)
    input wire                          RSTN,
    
    // SPI slave        -------------------------------
    // input  wire           SCK,
    // input  wire           MOSI,
    // output wire           MISO,

    (* X_INTERFACE_INFO = "xilinx.com:signal:reset:1.0 RSTN_OUT RSTN_OUT" *)
    output wire           RSTN_OUT,
    output wire           BUSY,
    
    // BRAM_MEM        -------------------------------
    (* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME BRAM_MEM, MASTER_TYPE BRAM_CTRL, READ_WRITE_MODE READ_WRITE, MEM_WIDTH 32, MEM_SIZE 131072, READ_LATENCY 1" *)
    (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 BRAM_MEM CLK" *)
    input  wire           BRAM_MEM_CLK,
    (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 BRAM_MEM RST" *)
    input  wire           BRAM_MEM_RST,
    (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 BRAM_MEM EN" *)
    input  wire           BRAM_MEM_EN,
    (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 BRAM_MEM WE" *)
    input  wire [  3:0]   BRAM_MEM_WE,
    (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 BRAM_MEM ADDR" *)
    input  wire [ 16:0]   BRAM_MEM_ADDR,
    (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 BRAM_MEM DIN" *)
    input  wire [ 31:0]   BRAM_MEM_DIN,
    (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 BRAM_MEM DOUT" *)
    output reg [ 31:0]    BRAM_MEM_DOUT,

	// Input 17-bit AER -------------------------------
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AERIN ADDR" *)
	input  wire [  2*M:0] AERIN_ADDR,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AERIN REQ" *)
	input  wire           AERIN_REQ,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AERIN ACK" *)
	output wire 		  AERIN_ACK,

	// Output 8-bit AER -------------------------------
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AEROUT ADDR" *)
	output wire [  M-1:0] AEROUT_ADDR,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AEROUT REQ" *)
	output wire 	      AEROUT_REQ,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AEROUT ACK" *)
	input  wire 	      AEROUT_ACK
);

    //----------------------------------------------------------------------------------
	//	Internal regs and wires
	//----------------------------------------------------------------------------------

    // Reset and clock
    wire                 RSTN_sync;
    reg                  RST_sync_int, RST_sync, RSTN_syncn;

    // AER output
    wire                 AEROUT_CTRL_BUSY;
    
    // REG + parameter bank
    wire                 REGMEM_GATE_ACTIVITY, REGMEM_GATE_ACTIVITY_sync;
    wire                 REGMEM_OPEN_LOOP;
    wire [        N-1:0] REGMEM_SYN_SIGN;
    wire [         19:0] REGMEM_BURST_TIMEREF;
    wire                 REGMEM_OUT_AER_MONITOR_EN;
    wire [        M-1:0] REGMEM_MONITOR_NEUR_ADDR;
    wire [        M-1:0] REGMEM_MONITOR_SYN_ADDR; 
    wire                 REGMEM_AER_SRC_CTRL_nNEUR;
    wire                 REGMEM_UPDATE_UNMAPPED_SYN;
	wire                 REGMEM_PROPAGATE_UNMAPPED_SYN;
	wire                 REGMEM_SDSP_ON_SYN_STIM;
    wire                 REGMEM_SW_RST;
    
    // Controller
    // wire                 CTRL_READBACK_EVENT;
    // wire                 CTRL_PROG_EVENT;
    // wire [      2*M-1:0] CTRL_REGMEM_ADDR;
    // wire [          1:0] CTRL_OP_CODE;
    wire [      2*M-1:0] CTRL_PROG_DATA;
    wire [          7:0] CTRL_PRE_EN;
    wire                 CTRL_BIST_REF;
    wire                 CTRL_SYNARRAY_WE;
    wire                 CTRL_NEURMEM_WE;
    wire [         12:0] CTRL_SYNARRAY_ADDR;
    wire [        M-1:0] CTRL_NEURMEM_ADDR;
    wire                 CTRL_SYNARRAY_CS;
    wire                 CTRL_NEURMEM_CS;
    wire                 CTRL_NEUR_EVENT; 
    wire                 CTRL_NEUR_TREF;  
    wire [          4:0] CTRL_NEUR_VIRTS;
    wire                 CTRL_NEUR_BURST_END;
    wire                 CTRL_NEUR_MAPTABLE;
    wire                 CTRL_SCHED_POP_N;
    wire [        M-1:0] CTRL_SCHED_ADDR;
    wire [          6:0] CTRL_SCHED_EVENT_IN;
    wire [          4:0] CTRL_SCHED_VIRTS;
    wire                 CTRL_AEROUT_POP_NEUR;
    
    // Synaptic core
    wire [         31:0] SYNARRAY_RDATA;
    wire [         31:0] SYNARRAY_WDATA;
    wire                 SYN_SIGN;
    
    // Scheduler
    wire                 SCHED_EMPTY;
    wire                 SCHED_FULL;
    wire                 SCHED_BURST_END;
    wire [         12:0] SCHED_DATA_OUT;
    
    // Neuron core
    wire [        127:0] NEUR_STATE;
    wire [         14:0] NEUR_STATE_MONITOR;
    wire [          6:0] NEUR_EVENT_OUT;
    wire [        N-1:0] NEUR_V_UP;
    wire [        N-1:0] NEUR_V_DOWN;
    
    
    //----------------------------------------------------------------------------------
	//	Reset (with double sync barrier)
	//----------------------------------------------------------------------------------
    
    always @(posedge CLK) begin
        RST_sync_int <= ~RSTN;
		RST_sync     <= RST_sync_int || REGMEM_SW_RST;
	end
    
    assign RSTN_sync = ~RST_sync;
    
    always @(negedge CLK) begin
        RSTN_syncn <= RSTN_sync;
    end
    
    assign RSTN_OUT = RSTN_sync;

    wire [31:0] BRAM_MEM_REGMEM_DOUT;
    wire BRAM_MEM_REGMEM_RD;
    wire [31:0] BRAM_MEM_NEURAM_DOUT;
    wire BRAM_MEM_NEURAM_RD;
    wire [31:0] BRAM_MEM_SYNRAM_DOUT;
    wire BRAM_MEM_SYNRAM_RD;

    reg BRAM_MEM_RD;
    reg [31:0] BRAM_MEM_DOUT_int;
    
    always @(*)
        if (BRAM_MEM_REGMEM_RD) begin 
            BRAM_MEM_DOUT = BRAM_MEM_REGMEM_DOUT;
            BRAM_MEM_RD = 1;
        end else if (BRAM_MEM_NEURAM_RD) begin
            BRAM_MEM_DOUT = BRAM_MEM_NEURAM_DOUT;
            BRAM_MEM_RD = 1;
        end else if (BRAM_MEM_SYNRAM_RD) begin
            BRAM_MEM_DOUT = BRAM_MEM_SYNRAM_DOUT;
            BRAM_MEM_RD = 1;
        end else begin
            BRAM_MEM_DOUT = BRAM_MEM_DOUT_int;
            BRAM_MEM_RD = 0;
        end
    
    always @(posedge CLK)
        if (BRAM_MEM_RD) BRAM_MEM_DOUT_int <= BRAM_MEM_DOUT;
        else             BRAM_MEM_DOUT_int <= BRAM_MEM_DOUT_int;
    
    //----------------------------------------------------------------------------------
	//	AER OUT
	//----------------------------------------------------------------------------------
    
    aer_out #(
        .N(N),
        .M(M)
    ) aer_out_0 (

        // Global input ----------------------------------- 
        .CLK(CLK),
        .RST(RST_sync),
        
        // Inputs from REG configuration latches ----------
        .REGMEM_GATE_ACTIVITY_sync(REGMEM_GATE_ACTIVITY_sync),
        .REGMEM_OUT_AER_MONITOR_EN(REGMEM_OUT_AER_MONITOR_EN),
        .REGMEM_MONITOR_NEUR_ADDR(REGMEM_MONITOR_NEUR_ADDR),
        .REGMEM_MONITOR_SYN_ADDR(REGMEM_MONITOR_SYN_ADDR), 
        .REGMEM_AER_SRC_CTRL_nNEUR(REGMEM_AER_SRC_CTRL_nNEUR),
        
        // Neuron data inputs -----------------------------
        .NEUR_STATE_MONITOR(NEUR_STATE_MONITOR),
        .NEUR_EVENT_OUT(NEUR_EVENT_OUT),
        .CTRL_NEURMEM_WE(CTRL_NEURMEM_WE), 
        .CTRL_NEURMEM_ADDR(CTRL_NEURMEM_ADDR),
        .CTRL_NEURMEM_CS(CTRL_NEURMEM_CS),
        
        // Synapse data inputs ----------------------------
        .SYNARRAY_WDATA(SYNARRAY_WDATA),
        .CTRL_SYNARRAY_WE(CTRL_SYNARRAY_WE), 
        .CTRL_SYNARRAY_ADDR(CTRL_SYNARRAY_ADDR),
        .CTRL_SYNARRAY_CS(CTRL_SYNARRAY_CS),
        
        // Input from scheduler ---------------------------
        .SCHED_DATA_OUT(SCHED_DATA_OUT),
        
        // Input from controller --------------------------
        .CTRL_AEROUT_POP_NEUR(CTRL_AEROUT_POP_NEUR),
        
        // Output to controller ---------------------------
        .AEROUT_CTRL_BUSY(AEROUT_CTRL_BUSY),
        
        // Output 8-bit AER link --------------------------
        .AEROUT_ADDR(AEROUT_ADDR),
        .AEROUT_REQ(AEROUT_REQ),
        .AEROUT_ACK(AEROUT_ACK)
    );
    
    
    //----------------------------------------------------------------------------------
	//	REG + parameter bank + clock int/ext handling
	//----------------------------------------------------------------------------------

    registers #(
        .N(N),
        .M(M)
    ) registers_0 (

        // Global inputs ------------------------------------------
        .CLK(CLK),
        .RST(~RSTN),

        // Global reset  -----------------------------------------
        .REGMEM_SW_RST(REGMEM_SW_RST),

        // Input from the controller  ----------------------------
        .ODIN_BUSY(BUSY),
        .REGMEM_GATE_ACTIVITY_sync(REGMEM_GATE_ACTIVITY_sync),
    
        // SPI slave interface ------------------------------------
        // .SCK(SCK),
        // .MISO(MISO),
        // .MOSI(MOSI),
        
        // Control interface for readback -------------------------
        // .CTRL_READBACK_EVENT(CTRL_READBACK_EVENT),
        // .CTRL_PROG_EVENT(CTRL_PROG_EVENT),
        // .CTRL_REGMEM_ADDR(CTRL_REGMEM_ADDR),
        // .CTRL_OP_CODE(CTRL_OP_CODE),
        // .CTRL_PROG_DATA(CTRL_PROG_DATA),
        // .SYNARRAY_RDATA(SYNARRAY_RDATA),
        // .NEUR_STATE(NEUR_STATE),

        .BRAM_MEM_EN(BRAM_MEM_EN),
        .BRAM_MEM_WE(BRAM_MEM_WE),
        .BRAM_MEM_ADDR(BRAM_MEM_ADDR),
        .BRAM_MEM_DIN(BRAM_MEM_DIN),
        .BRAM_MEM_REGMEM_DOUT(BRAM_MEM_REGMEM_DOUT),
        .BRAM_MEM_REGMEM_RD(BRAM_MEM_REGMEM_RD),
    
        // Configuration registers output -------------------------
        .REGMEM_GATE_ACTIVITY(REGMEM_GATE_ACTIVITY),
        .REGMEM_OPEN_LOOP(REGMEM_OPEN_LOOP),
        .REGMEM_SYN_SIGN(REGMEM_SYN_SIGN),
        .REGMEM_BURST_TIMEREF(REGMEM_BURST_TIMEREF),
        .REGMEM_OUT_AER_MONITOR_EN(REGMEM_OUT_AER_MONITOR_EN),
        .REGMEM_AER_SRC_CTRL_nNEUR(REGMEM_AER_SRC_CTRL_nNEUR),
        .REGMEM_MONITOR_NEUR_ADDR(REGMEM_MONITOR_NEUR_ADDR),
        .REGMEM_MONITOR_SYN_ADDR(REGMEM_MONITOR_SYN_ADDR),
        .REGMEM_UPDATE_UNMAPPED_SYN(REGMEM_UPDATE_UNMAPPED_SYN),
		.REGMEM_PROPAGATE_UNMAPPED_SYN(REGMEM_PROPAGATE_UNMAPPED_SYN),
		.REGMEM_SDSP_ON_SYN_STIM(REGMEM_SDSP_ON_SYN_STIM)
    );
    
    
    //----------------------------------------------------------------------------------
	//	Controller
	//----------------------------------------------------------------------------------

    controller #(
        .N(N),
        .M(M)
    ) controller_0 (
    
        // Global inputs ------------------------------------------
        .CLK(CLK),
        .RST(RST_sync),
    
        // Inputs from AER ----------------------------------------
        .AERIN_ADDR(AERIN_ADDR),
        .AERIN_REQ(AERIN_REQ),
        .AERIN_ACK(AERIN_ACK),

        // Control interface for readback -------------------------
        // .CTRL_READBACK_EVENT(CTRL_READBACK_EVENT),
        // .CTRL_PROG_EVENT(CTRL_PROG_EVENT),
        // .CTRL_REGMEM_ADDR(CTRL_REGMEM_ADDR),
        // .CTRL_OP_CODE(CTRL_OP_CODE),
		.REGMEM_SDSP_ON_SYN_STIM(REGMEM_SDSP_ON_SYN_STIM),
        
        // Inputs from REG configuration registers ----------------
        .REGMEM_GATE_ACTIVITY(REGMEM_GATE_ACTIVITY),
        .REGMEM_GATE_ACTIVITY_sync(REGMEM_GATE_ACTIVITY_sync),
        .REGMEM_MONITOR_NEUR_ADDR(REGMEM_MONITOR_NEUR_ADDR),
        
        // Inputs from scheduler ----------------------------------
        .SCHED_EMPTY(SCHED_EMPTY),
        .SCHED_FULL(SCHED_FULL),
        .SCHED_BURST_END(SCHED_BURST_END),
        .SCHED_DATA_OUT(SCHED_DATA_OUT),
        
        // Input from AER output ----------------------------------
        .AEROUT_CTRL_BUSY(AEROUT_CTRL_BUSY),

        // Status output  -----------------------------------------
        .ODIN_BUSY(BUSY),
        
        // Outputs to synaptic core -------------------------------
        .CTRL_PRE_EN(CTRL_PRE_EN),
        .CTRL_BIST_REF(CTRL_BIST_REF),
        .CTRL_SYNARRAY_WE(CTRL_SYNARRAY_WE),
        .CTRL_SYNARRAY_ADDR(CTRL_SYNARRAY_ADDR),
        .CTRL_SYNARRAY_CS(CTRL_SYNARRAY_CS),
        .CTRL_NEURMEM_WE(CTRL_NEURMEM_WE),
        .CTRL_NEURMEM_ADDR(CTRL_NEURMEM_ADDR),
        .CTRL_NEURMEM_CS(CTRL_NEURMEM_CS),
        
        // Outputs to neurons -------------------------------------
        .CTRL_NEUR_EVENT(CTRL_NEUR_EVENT), 
        .CTRL_NEUR_TREF(CTRL_NEUR_TREF),
        .CTRL_NEUR_VIRTS(CTRL_NEUR_VIRTS),
        .CTRL_NEUR_BURST_END(CTRL_NEUR_BURST_END),
        
        // Outputs to scheduler -----------------------------------
        .CTRL_SCHED_POP_N(CTRL_SCHED_POP_N),
        .CTRL_SCHED_ADDR(CTRL_SCHED_ADDR),
        .CTRL_SCHED_EVENT_IN(CTRL_SCHED_EVENT_IN),
        .CTRL_SCHED_VIRTS(CTRL_SCHED_VIRTS),

        // Output to AER output -----------------------------------
        .CTRL_AEROUT_POP_NEUR(CTRL_AEROUT_POP_NEUR)
    );
    
    
    //----------------------------------------------------------------------------------
	//	Scheduler
	//----------------------------------------------------------------------------------

    scheduler #(
        .prio_num(57),
        .N(N),
        .M(M)
    ) scheduler_0 (
    
        // Global inputs ------------------------------------------
        .CLK(CLK),
        .RSTN(RSTN_sync),
    
        // Inputs from controller ---------------------------------
        .CTRL_SCHED_POP_N(CTRL_SCHED_POP_N),
        .CTRL_SCHED_VIRTS(CTRL_SCHED_VIRTS),
        .CTRL_SCHED_ADDR(CTRL_SCHED_ADDR),
        .CTRL_SCHED_EVENT_IN(CTRL_SCHED_EVENT_IN),
        
        // Inputs from neurons ------------------------------------
        .CTRL_NEURMEM_ADDR(CTRL_NEURMEM_ADDR),
        .NEUR_EVENT_OUT(NEUR_EVENT_OUT),
        
        // Inputs from REG configuration registers ----------------
        .REGMEM_OPEN_LOOP(REGMEM_OPEN_LOOP),
        .REGMEM_BURST_TIMEREF(REGMEM_BURST_TIMEREF),
        
        // Outputs ------------------------------------------------
        .SCHED_EMPTY(SCHED_EMPTY),
        .SCHED_FULL(SCHED_FULL),
        .SCHED_BURST_END(SCHED_BURST_END),
        .SCHED_DATA_OUT(SCHED_DATA_OUT)
    );
    
    
    //----------------------------------------------------------------------------------
	//	Synaptic core
	//----------------------------------------------------------------------------------
   
    synaptic_core #(
        .N(N),
        .M(M)
    ) synaptic_core_0 (
    
        // Global inputs ------------------------------------------
        .RSTN_syncn(RSTN_syncn),
        .CLK(CLK),

        // Inputs from REG configuration registers ----------------
        .REGMEM_GATE_ACTIVITY_sync(REGMEM_GATE_ACTIVITY_sync),
        .REGMEM_SYN_SIGN(REGMEM_SYN_SIGN),
        .REGMEM_UPDATE_UNMAPPED_SYN(REGMEM_UPDATE_UNMAPPED_SYN),
        
        // Inputs from controller ---------------------------------
        .CTRL_PRE_EN(CTRL_PRE_EN),
        .CTRL_BIST_REF(CTRL_BIST_REF),
        .CTRL_SYNARRAY_WE(CTRL_SYNARRAY_WE),
        .CTRL_SYNARRAY_ADDR(CTRL_SYNARRAY_ADDR),
        .CTRL_SYNARRAY_CS(CTRL_SYNARRAY_CS),
        // .CTRL_PROG_DATA(CTRL_PROG_DATA),
        // .CTRL_REGMEM_ADDR(CTRL_REGMEM_ADDR),
        
        // Inputs from neurons ------------------------------------
        .NEUR_V_UP(NEUR_V_UP),
        .NEUR_V_DOWN(NEUR_V_DOWN),
        
        // Outputs ------------------------------------------------
        .SYNARRAY_RDATA(SYNARRAY_RDATA),
        .SYNARRAY_WDATA(SYNARRAY_WDATA),
        .SYN_SIGN(SYN_SIGN),

        // Synapse ram interface ----------------------------------
        .BRAM_MEM_EN(BRAM_MEM_EN),
        .BRAM_MEM_WE(BRAM_MEM_WE),
        .BRAM_MEM_ADDR(BRAM_MEM_ADDR),
        .BRAM_MEM_DIN(BRAM_MEM_DIN),
        .BRAM_MEM_SYNRAM_DOUT(BRAM_MEM_SYNRAM_DOUT),
        .BRAM_MEM_SYNRAM_RD(BRAM_MEM_SYNRAM_RD)
	);
    
    
    //----------------------------------------------------------------------------------
	//	Neural core
	//----------------------------------------------------------------------------------
      
    neuron_core #(
        .N(N),
        .M(M)
    ) neuron_core_0 (
    
        // Global inputs ------------------------------------------
        .RSTN_syncn(RSTN_syncn),
        .CLK(CLK),
        
        // Inputs from REG configuration registers ----------------
        .REGMEM_GATE_ACTIVITY_sync(REGMEM_GATE_ACTIVITY_sync),
        .REGMEM_PROPAGATE_UNMAPPED_SYN(REGMEM_PROPAGATE_UNMAPPED_SYN),
		
        // Synaptic inputs ----------------------------------------
        .SYNARRAY_RDATA(SYNARRAY_RDATA),
        .SYN_SIGN(SYN_SIGN),
        
        // Inputs from controller ---------------------------------
        .CTRL_NEUR_EVENT(CTRL_NEUR_EVENT),
        .CTRL_NEUR_TREF(CTRL_NEUR_TREF),
        .CTRL_NEUR_VIRTS(CTRL_NEUR_VIRTS),
        .CTRL_NEURMEM_WE(CTRL_NEURMEM_WE),
        .CTRL_NEURMEM_ADDR(CTRL_NEURMEM_ADDR),
        .CTRL_NEURMEM_CS(CTRL_NEURMEM_CS),
        // .CTRL_PROG_DATA(CTRL_PROG_DATA),
        // .CTRL_REGMEM_ADDR(CTRL_REGMEM_ADDR),
        
        // Inputs from scheduler ----------------------------------
        .CTRL_NEUR_BURST_END(CTRL_NEUR_BURST_END), 
        
        // Outputs ------------------------------------------------
        .NEUR_STATE(NEUR_STATE),
        .NEUR_EVENT_OUT(NEUR_EVENT_OUT),
        .NEUR_V_UP(NEUR_V_UP),
        .NEUR_V_DOWN(NEUR_V_DOWN),
        .NEUR_STATE_MONITOR(NEUR_STATE_MONITOR),
        
        // Neuron ram interface -----------------------------------
        .BRAM_MEM_EN(BRAM_MEM_EN),
        .BRAM_MEM_WE(BRAM_MEM_WE),
        .BRAM_MEM_ADDR(BRAM_MEM_ADDR),
        .BRAM_MEM_DIN(BRAM_MEM_DIN),
        .BRAM_MEM_NEURAM_DOUT(BRAM_MEM_NEURAM_DOUT),
        .BRAM_MEM_NEURAM_RD(BRAM_MEM_NEURAM_RD)
    );
     
        
    
endmodule
