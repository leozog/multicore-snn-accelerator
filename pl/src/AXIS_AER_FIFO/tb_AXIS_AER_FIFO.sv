`timescale 1ns / 1ps

module tb_AXIS_AER_FIFO;

    localparam integer S_AXIS_TDATA_WIDTH = 32;
    localparam integer AER_DATA_WIDTH = 17;
    localparam integer FIFO_SIZE = 4;
    localparam integer TX_COUNT = 7;

    logic                          clk;
    logic                          rstn;

    logic [S_AXIS_TDATA_WIDTH-1:0] s_axis_tdata;
    logic                          s_axis_tvalid;
    logic                          s_axis_tready;
    logic                          s_axis_tlast;

    logic [AER_DATA_WIDTH-1:0]     aerout_addr;
    logic                          aerout_req;
    logic                          aerout_ack;
    logic                          aerout_last;

    AXIS_AER_FIFO #(
        .S_AXIS_TDATA_WIDTH (S_AXIS_TDATA_WIDTH),
        .AER_DATA_WIDTH     (AER_DATA_WIDTH),
        .FIFO_MIN_SIZE      (FIFO_SIZE)
    ) axis_aer_fifo_0 (
        .CLK             (clk),
        .RSTN            (rstn),
        .S_AXIS_TDATA    (s_axis_tdata),
        .S_AXIS_TVALID   (s_axis_tvalid),
        .S_AXIS_TREADY   (s_axis_tready),
        .S_AXIS_TLAST    (s_axis_tlast),
        .AEROUT_ADDR     (aerout_addr),
        .AEROUT_REQ      (aerout_req),
        .AEROUT_ACK      (aerout_ack),
        .AEROUT_LAST     (aerout_last)
    );

    initial begin
        clk = 1'b0;
        forever #1 clk = ~clk;
    end

    task automatic reset();
        begin
            rstn          = 1'b0;
            s_axis_tvalid = 1'b0;
            s_axis_tlast  = 1'b0;
            s_axis_tdata  = 'x;
            aerout_addr   = 'x;
            aerout_ack    = 1'b0;
            
            repeat(2) @(posedge clk);
            rstn = 1'b1;
            repeat(2) @(posedge clk);
        end
    endtask

    task automatic axis_producer(int count);
        begin
            for (int i = 0; i < count; i++) begin
                s_axis_tdata  = S_AXIS_TDATA_WIDTH'(i + 1);
                s_axis_tvalid = 1'b1;
                s_axis_tlast  = (i == count - 1) ? 1'b1 : 1'b0;
                
                wait(s_axis_tready == 1);
                @(posedge clk);
                #0.01;
            end
            
            s_axis_tvalid = 1'b0;
            s_axis_tlast  = 1'b0;
            s_axis_tdata  = '0;
        end
    endtask

    task automatic aer_consumer();
        begin
            int i = 0;
            while (1) begin
                wait(aerout_req == 1'b1);
                @(posedge clk);
                
                if (i >= 1 && i <= 3) begin
                    aerout_ack = 1'b0;
                    @(posedge clk);
                end else begin
                    aerout_ack = 1'b1;
                    
                    wait(aerout_req == 1'b0);
                    @(posedge clk);
                    
                    aerout_ack = 1'b0;
                end
                i = i + 1;
            end
        end
    endtask

    initial begin
        reset();
        fork
            axis_producer(TX_COUNT);
            aer_consumer();
        join
        repeat(10) @(posedge clk);
        $finish;
    end

endmodule

