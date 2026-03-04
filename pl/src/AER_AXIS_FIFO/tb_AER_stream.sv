`timescale 1ns / 1ps

module tb_AER_stream;

    localparam int unsigned S_AXIS_TDATA_WIDTH = 32;
    localparam int unsigned M_AXIS_TDATA_WIDTH = 32;
    localparam int unsigned AER_DATA_WIDTH     = 16;
    localparam int unsigned FIFO1_SIZE          = 8;
    localparam int unsigned FIFO2_SIZE          = 4;
    localparam int unsigned TX_COUNT           = 7;

    logic                          clk;
    logic                          rstn;

    logic [S_AXIS_TDATA_WIDTH-1:0] s_axis_tdata;
    logic                          s_axis_tvalid;
    logic                          s_axis_tready;
    logic                          s_axis_tlast;

    logic [AER_DATA_WIDTH-1:0]     aer_addr;
    logic                          aer_req;
    logic                          aer_ack;
    logic                          aer_last;

    logic [M_AXIS_TDATA_WIDTH-1:0] m_axis_tdata;
    logic                          m_axis_tvalid;
    logic                          m_axis_tready;
    logic                          m_axis_tlast;

    AXIS_AER_FIFO #(
        .S_AXIS_TDATA_WIDTH(S_AXIS_TDATA_WIDTH),
        .AER_DATA_WIDTH    (AER_DATA_WIDTH),
        .FIFO_MIN_SIZE         (FIFO1_SIZE)
    ) axis_aer_fifo_0 (
        .CLK        (clk),
        .RSTN       (rstn),
        .S_AXIS_TDATA(s_axis_tdata),
        .S_AXIS_TVALID(s_axis_tvalid),
        .S_AXIS_TREADY(s_axis_tready),
        .S_AXIS_TLAST(s_axis_tlast),
        .AEROUT_ADDR(aer_addr),
        .AEROUT_REQ (aer_req),
        .AEROUT_ACK (aer_ack),
        .AEROUT_LAST(aer_last)
    );

    AER_AXIS_FIFO #(
        .M_AXIS_TDATA_WIDTH(M_AXIS_TDATA_WIDTH),
        .AER_DATA_WIDTH    (AER_DATA_WIDTH),
        .FIFO_MIN_SIZE         (FIFO2_SIZE)
    ) aer_axis_fifo_0 (
        .CLK          (clk),
        .RSTN         (rstn),
        .AERIN_ADDR   (aer_addr),
        .AERIN_REQ    (aer_req),
        .AERIN_ACK    (aer_ack),
        .AERIN_LAST   (aer_last),
        .M_AXIS_TDATA (m_axis_tdata),
        .M_AXIS_TVALID(m_axis_tvalid),
        .M_AXIS_TREADY(m_axis_tready),
        .M_AXIS_TLAST (m_axis_tlast)
    );

    initial begin
        clk = 1'b0;
        forever #2 clk = ~clk;
    end

    task automatic reset();
        begin
            rstn          = 1'b0;
            s_axis_tdata  = 'x;
            s_axis_tvalid = 1'b0;
            s_axis_tlast  = 1'b0;
            aer_addr      = 'x;
            m_axis_tdata  = 'x;
            m_axis_tvalid = 1'b0;
            m_axis_tready = 1'b0;

            repeat (2) @(posedge clk);
            rstn = 1'b1;
            repeat (2) @(posedge clk);
        end
    endtask

    task automatic axis_producer(int unsigned count);
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

    task axis_consumer_ready_pattern();
        begin
            m_axis_tready = 1'b1;
            repeat (10)  @(posedge clk);
            m_axis_tready = 1'b0;
            repeat (10) @(posedge clk);
            m_axis_tready = 1'b1;
        end
    endtask

    task automatic axis_consumer();
        begin
            while (1) begin
                wait(m_axis_tready && m_axis_tvalid);
                @(posedge clk);
                #0.01;
            end
        end
    endtask

    initial begin
        reset();
        fork
            axis_producer(TX_COUNT);
            axis_consumer_ready_pattern();
            axis_consumer();
        join
        repeat (10) @(posedge clk);
        $finish;
    end

endmodule
