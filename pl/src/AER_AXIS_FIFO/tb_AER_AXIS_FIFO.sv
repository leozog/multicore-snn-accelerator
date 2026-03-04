`timescale 1ns / 1ps

module tb_AER_AXIS_FIFO;

    localparam integer M_AXIS_TDATA_WIDTH = 8;
    localparam integer AER_DATA_WIDTH     = 8;
    localparam integer FIFO_SIZE          = 3;
    localparam integer TX_COUNT           = 5;

    logic                          clk;
    logic                          rstn;

    logic [AER_DATA_WIDTH-1:0]     aerin_addr;
    logic                          aerin_req;
    logic                          aerin_ack;
    logic                          aerin_last;

    logic [M_AXIS_TDATA_WIDTH-1:0] m_axis_tdata;
    logic                          m_axis_tvalid;
    logic                          m_axis_tready;
    logic                          m_axis_tlast;

    AER_AXIS_FIFO #(
        .M_AXIS_TDATA_WIDTH (M_AXIS_TDATA_WIDTH),
        .AER_DATA_WIDTH     (AER_DATA_WIDTH),
        .FIFO_MIN_SIZE      (FIFO_SIZE)
    ) aer_axis_fifo_0 (
        .CLK          (clk),
        .RSTN       (rstn),
        .AERIN_ADDR   (aerin_addr),
        .AERIN_REQ    (aerin_req),
        .AERIN_ACK    (aerin_ack),
        .AERIN_LAST   (aerin_last),
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
            rstn         = 1'b0;
            aerin_addr   = 'x;
            aerin_req    = 0;
            aerin_last   = 0;
            m_axis_tdata = 'x;
            m_axis_tvalid= 0;
            m_axis_tready= 0;

            repeat (2) @(posedge clk);
            rstn         = 1'b1;
            repeat (2) @(posedge clk);
        end
    endtask

    task automatic aer_producer(int count);
        begin
            for (int i = 0; i < count; i++) begin
                aerin_addr = AER_DATA_WIDTH'(1 + i);
                aerin_req  = 1'b1;
                aerin_last = (i == count - 1);

                wait(aerin_ack == 1'b1);
                @(posedge clk);
                #0.01;

                aerin_req  = 1'b0;
                wait(aerin_ack == 1'b0);
                @(posedge clk);
                #0.01;
            end

            aerin_req  = 1'b0;
            aerin_last = 1'b0;
            aerin_addr = '0;
        end
    endtask

    task automatic axis_consumer_ready_pattern();
        begin
            m_axis_tready = 1'b1;
            repeat (8)  @(posedge clk);
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
            aer_producer(TX_COUNT);
            axis_consumer_ready_pattern();
            axis_consumer();
        join
        repeat(10) @(posedge clk);
        $finish;
    end

endmodule
