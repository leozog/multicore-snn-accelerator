`timescale 1ns / 1ps

module AER_STREAM_CTRL #(
    parameter integer AERIN_DATA_WIDTH = 17,
    parameter integer AEROUT_DATA_WIDTH = 8
)(
    (* X_INTERFACE_INFO = "xilinx.com:signal:clock:1.0 CLK CLK" *)
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 1000000, FREQ_TOLERANCE_HZ -1, ASSOCIATED_RESET RSTN, ASSOCIATED_BUSIF AERIN_I:AERIN_O:AEROUT_I:AEROUT_O" *)
    input wire                          CLK,
    (* X_INTERFACE_INFO = "xilinx.com:signal:reset:1.0 RSTN RSTN" *)
    input wire                          RSTN,

    input logic                          CORE_BUSY,

    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AERIN_I ADDR" *)
    input logic [AERIN_DATA_WIDTH-1:0]   AERIN_I_ADDR,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AERIN_I REQ" *)
    input logic                          AERIN_I_REQ,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AERIN_I ACK" *)
    output logic                         AERIN_I_ACK,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AERIN_I LAST" *)
    input logic                          AERIN_I_LAST,

    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AERIN_O ADDR" *)
    output logic [AERIN_DATA_WIDTH-1:0]  AERIN_O_ADDR,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AERIN_O  REQ" *)
    output logic                         AERIN_O_REQ,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AERIN_O ACK" *)
    input logic                          AERIN_O_ACK,

    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AEROUT_I ADDR" *)
    input logic [AEROUT_DATA_WIDTH-1:0]  AEROUT_I_ADDR,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AEROUT_I REQ" *)
    input logic                          AEROUT_I_REQ,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AEROUT_I ACK" *)
    output logic                         AEROUT_I_ACK,

    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AEROUT_O ADDR" *)
    output logic [AEROUT_DATA_WIDTH-1:0] AEROUT_O_ADDR,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AEROUT_O REQ" *)
    output logic                         AEROUT_O_REQ,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AEROUT_O ACK" *)
    input logic                          AEROUT_O_ACK,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aer:1.0 AEROUT_O LAST" *)
    output logic                         AEROUT_O_LAST
);

assign AERIN_O_ADDR = AERIN_I_ADDR;
assign AEROUT_O_ADDR = AEROUT_I_ADDR;

enum logic [2:0] {WAIT_AERIN_LAST, WAIT_AERIN_REQN, WAIT_NOT_BUSY, SEND_AEROUT_LAST, WAIT_AEROUT_ACKN} state, next_state;

always @(posedge CLK, negedge RSTN)
    if (!RSTN) state <= WAIT_AERIN_LAST;
    else     state <= next_state;

always_comb
    case (state)
        WAIT_AERIN_LAST:    
            if (AERIN_I_REQ && AERIN_I_LAST)    next_state = WAIT_AERIN_REQN;
            else                                next_state = WAIT_AERIN_LAST;
        WAIT_AERIN_REQN:
            if (!AERIN_I_REQ)                   next_state = WAIT_NOT_BUSY;
            else                                next_state = WAIT_AERIN_REQN;
        WAIT_NOT_BUSY:
            if (!CORE_BUSY)                     next_state = SEND_AEROUT_LAST;
            else                                next_state = WAIT_NOT_BUSY;
        SEND_AEROUT_LAST:
            if (AEROUT_O_ACK)                   next_state = WAIT_AEROUT_ACKN;
            else                                next_state = SEND_AEROUT_LAST;
        WAIT_AEROUT_ACKN:
            if (!AEROUT_O_ACK)                  next_state = WAIT_AERIN_LAST;
            else                                next_state = WAIT_AEROUT_ACKN;
        default:                                
                                                next_state = WAIT_AERIN_LAST;
    endcase

always_comb begin
    case (state)
        WAIT_AERIN_LAST: begin
            if (AERIN_I_REQ && AERIN_I_LAST) begin
                AERIN_I_ACK = 0;
                AERIN_O_REQ = 0;
            end else begin
                AERIN_I_ACK = AERIN_O_ACK;
                AERIN_O_REQ = AERIN_I_REQ;
            end
        end
        WAIT_AERIN_REQN: begin
            AERIN_I_ACK = 1;
            AERIN_O_REQ = 0;
        end
        WAIT_NOT_BUSY: begin
            AERIN_I_ACK = 0;
            AERIN_O_REQ = 0;
        end
        SEND_AEROUT_LAST: begin
            AERIN_I_ACK = 0;
            AERIN_O_REQ = 0;
        end
        WAIT_AEROUT_ACKN: begin
            AERIN_I_ACK = 0;
            AERIN_O_REQ = 0;
        end
        default: begin
            AERIN_I_ACK = 0;
            AERIN_O_REQ = 0;
        end
    endcase
end

always_comb begin
    case (state)
        WAIT_AERIN_LAST: begin
            AEROUT_O_REQ  = AEROUT_I_REQ;
            AEROUT_I_ACK  = AEROUT_O_ACK;
            AEROUT_O_LAST = 0;
        end
        WAIT_AERIN_REQN: begin
            AEROUT_O_REQ  = AEROUT_I_REQ;
            AEROUT_I_ACK  = AEROUT_O_ACK;
            AEROUT_O_LAST = 0;
        end
        WAIT_NOT_BUSY: begin
            AEROUT_O_REQ  = AEROUT_I_REQ;
            AEROUT_I_ACK  = AEROUT_O_ACK;
            AEROUT_O_LAST = 0;
        end
        SEND_AEROUT_LAST: begin
            AEROUT_O_REQ  = 1;
            AEROUT_I_ACK  = 0;
            AEROUT_O_LAST = 1;
        end
        WAIT_AEROUT_ACKN: begin
            AEROUT_O_REQ  = 0;
            AEROUT_I_ACK  = 0;
            AEROUT_O_LAST = 0;
        end
        default: begin
            AEROUT_O_REQ  = 0;
            AEROUT_I_ACK  = 0;
            AEROUT_O_LAST = 0;
        end
    endcase
end

endmodule
