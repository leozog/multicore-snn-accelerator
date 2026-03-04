`timescale 1ns / 1ps

module AXIS_AER_FIFO #
(
    parameter integer S_AXIS_TDATA_WIDTH = 32,
    parameter integer AER_DATA_WIDTH = 32,
    parameter integer FIFO_MIN_SIZE = 16
)
(
    (* X_INTERFACE_INFO = "xilinx.com:signal:clock:1.0 CLK CLK" *)
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 1000000, FREQ_TOLERANCE_HZ -1, ASSOCIATED_RESET RSTN, ASSOCIATED_BUSIF S_AXIS:AEROUT" *)
    input wire                          CLK,
    (* X_INTERFACE_INFO = "xilinx.com:signal:reset:1.0 RSTN RSTN" *)
    input wire                          RSTN,

    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 S_AXIS TDATA" *)
    input wire [S_AXIS_TDATA_WIDTH-1:0] S_AXIS_TDATA,
    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 S_AXIS TVALID" *)
    input wire                          S_AXIS_TVALID,
    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 S_AXIS TREADY" *)
    output wire                         S_AXIS_TREADY,
    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 S_AXIS TLAST" *)
    input wire                          S_AXIS_TLAST,

    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AEROUT ADDR" *)
    output reg [AER_DATA_WIDTH-1:0]     AEROUT_ADDR,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AEROUT REQ" *)
	output wire 	                    AEROUT_REQ,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AEROUT ACK" *)
	input  wire 	                    AEROUT_ACK,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AEROUT LAST" *)
	output reg 		                    AEROUT_LAST
);
    localparam integer ADDR_WIDTH = $clog2(FIFO_MIN_SIZE);
    localparam integer FIFO_SIZE = 1 << ADDR_WIDTH;

    reg [AER_DATA_WIDTH-1:0] data_fifo [FIFO_SIZE-1:0];
    reg last_fifo [FIFO_SIZE-1:0];
    reg [ADDR_WIDTH:0] write_ptr;
    reg [ADDR_WIDTH:0] read_ptr;

    wire empty = write_ptr == read_ptr;
    wire full  = (write_ptr[ADDR_WIDTH] != read_ptr[ADDR_WIDTH]) && 
                 (write_ptr[ADDR_WIDTH-1:0] == read_ptr[ADDR_WIDTH-1:0]);

    // AXIS INPUT
    assign S_AXIS_TREADY = RSTN && !full;
    wire write = S_AXIS_TVALID & S_AXIS_TREADY;

    always @(posedge CLK, negedge RSTN)
        if (!RSTN)      write_ptr <= 1'b0;
        else if (write) write_ptr <= write_ptr + 1'b1;
        else            write_ptr <= write_ptr;
        
    always @(posedge CLK)   
        if (write) begin
            data_fifo[write_ptr[ADDR_WIDTH-1:0]] <= S_AXIS_TDATA;
            last_fifo[write_ptr[ADDR_WIDTH-1:0]] <= S_AXIS_TLAST;
        end
    // AXIS INPUT
    
    // AER OUTPUT
    enum logic [1:0] {AER_IDLE, AER_REQ, AER_WAIT_ACKN} aer_state, next_aer_state;

    always @(posedge CLK, negedge RSTN)
        if (!RSTN)      aer_state <= AER_IDLE;
        else            aer_state <= next_aer_state;

    always_comb
        case (aer_state)
            AER_IDLE:
                if (empty)      next_aer_state = AER_IDLE;
                else            next_aer_state = AEROUT_ACK ? AER_IDLE : AER_REQ;
            AER_REQ:
                                next_aer_state = AEROUT_ACK ? AER_WAIT_ACKN : AER_REQ;
            AER_WAIT_ACKN:
                if (AEROUT_ACK) next_aer_state = AER_WAIT_ACKN;
                else            next_aer_state = empty ? AER_IDLE : AER_REQ;
            default:            
                                next_aer_state = AER_IDLE;
        endcase
    
    assign AEROUT_REQ = aer_state == AER_REQ;
    wire read = aer_state != AER_REQ && next_aer_state == AER_REQ;

    always @(posedge CLK, negedge RSTN)
        if (!RSTN)      read_ptr <= 1'b0;
        else if (read)  read_ptr <= read_ptr + 1'b1;
        else            read_ptr <= read_ptr;

    always @(posedge CLK)
        if (read) begin
            AEROUT_ADDR <= data_fifo[read_ptr[ADDR_WIDTH-1:0]];
            AEROUT_LAST <= last_fifo[read_ptr[ADDR_WIDTH-1:0]];
        end
    // AER OUTPUT

endmodule
