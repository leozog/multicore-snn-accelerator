`timescale 1ns / 1ps

module AER_AXIS_FIFO #
(
    parameter integer M_AXIS_TDATA_WIDTH = 32,
    parameter integer AER_DATA_WIDTH = 32,
    parameter integer FIFO_MIN_SIZE = 16
)
(
    (* X_INTERFACE_INFO = "xilinx.com:signal:clock:1.0 CLK CLK" *)
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 1000000, FREQ_TOLERANCE_HZ -1, ASSOCIATED_RESET RSTN, ASSOCIATED_BUSIF AERIN:M_AXIS" *)
    input wire                          CLK,
    (* X_INTERFACE_INFO = "xilinx.com:signal:reset:1.0 RSTN RSTN" *)
    input wire                          RSTN,

    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AERIN ADDR" *)
    input wire [AER_DATA_WIDTH-1:0]     AERIN_ADDR,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AERIN REQ" *)
    input wire                          AERIN_REQ,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AERIN ACK" *)
    output wire                         AERIN_ACK,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AERIN LAST" *)
    input wire                          AERIN_LAST,
    
    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 M_AXIS TDATA" *)
    output reg [M_AXIS_TDATA_WIDTH-1:0] M_AXIS_TDATA,
    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 M_AXIS TVALID" *)
    output reg                          M_AXIS_TVALID,
    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 M_AXIS TREADY" *)
    input wire                          M_AXIS_TREADY,
    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 M_AXIS TLAST" *)
    output reg                          M_AXIS_TLAST
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

    // AER INPUT
    enum logic {AER_IDLE, AER_ACK} aer_state, next_aer_state;
    
    always @(posedge CLK, negedge RSTN)
        if (!RSTN)      aer_state <= AER_IDLE;
        else            aer_state <= next_aer_state;

    always_comb
        case (aer_state)
            AER_IDLE:
                if (AERIN_REQ)  next_aer_state = !full ? AER_ACK : AER_IDLE;
                else            next_aer_state = AER_IDLE;
            AER_ACK:
                                next_aer_state = AERIN_REQ ? AER_ACK : AER_IDLE;
            default:            
                                next_aer_state = AER_IDLE;
        endcase
    
    assign AERIN_ACK = aer_state == AER_ACK;
    wire write = RSTN && aer_state != AER_ACK && next_aer_state == AER_ACK;

    always @(posedge CLK, negedge RSTN)
        if (!RSTN)      write_ptr <= 1'b0;
        else if (write) write_ptr <= write_ptr + 1'b1;
        else            write_ptr <= write_ptr;

    always @(posedge CLK)
        if (write) begin
            data_fifo[write_ptr[ADDR_WIDTH-1:0]] <= AERIN_ADDR;
            last_fifo[write_ptr[ADDR_WIDTH-1:0]] <= AERIN_LAST;
        end
    // AER INPUT
    
    // AXIS OUTPUT
    wire axis_valid_int = RSTN && !empty;
    wire read = M_AXIS_TREADY & axis_valid_int;

    always @(posedge CLK, negedge RSTN)
        if (!RSTN)      read_ptr <= 1'b0;
        else if (read)  read_ptr <= read_ptr + 1'b1;
        else            read_ptr <= read_ptr;
        
    assign M_AXIS_TVALID = !empty;
    assign M_AXIS_TDATA  = data_fifo[read_ptr[ADDR_WIDTH-1:0]];
    assign M_AXIS_TLAST  = last_fifo[read_ptr[ADDR_WIDTH-1:0]];
    // AXIS OUTPUT

endmodule
