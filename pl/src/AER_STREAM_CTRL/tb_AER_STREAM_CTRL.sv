`timescale 1ns / 1ps

module tb_AER_STREAM_CTRL;

	localparam integer AERIN_DATA_WIDTH  = 17;
	localparam integer AEROUT_DATA_WIDTH = 8;
	localparam integer AERIN_TX_COUNT_0  = 3;
	localparam integer AERIN_TX_COUNT_1  = 0;
	localparam integer AEROUT_TX_COUNT_0   = 2;

	logic                          clk;
	logic                          rstn;
	logic                          core_busy;

	logic [AERIN_DATA_WIDTH-1:0]   aerin_i_addr;
	logic                          aerin_i_req;
	logic                          aerin_i_ack;
	logic                          aerin_i_last;

	logic [AERIN_DATA_WIDTH-1:0]   aerin_o_addr;
	logic                          aerin_o_req;
	logic                          aerin_o_ack;

	logic [AEROUT_DATA_WIDTH-1:0]  aerout_i_addr;
	logic                          aerout_i_req;
	logic                          aerout_i_ack;

	logic [AEROUT_DATA_WIDTH-1:0]  aerout_o_addr;
	logic                          aerout_o_req;
	logic                          aerout_o_ack;
	logic                          aerout_o_last;

	AER_STREAM_CTRL #(
		.AERIN_DATA_WIDTH  (AERIN_DATA_WIDTH),
		.AEROUT_DATA_WIDTH (AEROUT_DATA_WIDTH)
	) aer_stream_ctrl_0 (
		.CLK          (clk),
		.RSTN         (rstn),
		.CORE_BUSY    (core_busy),

		.AERIN_I_ADDR (aerin_i_addr),
		.AERIN_I_REQ  (aerin_i_req),
		.AERIN_I_ACK  (aerin_i_ack),
		.AERIN_I_LAST (aerin_i_last),

		.AERIN_O_ADDR (aerin_o_addr),
		.AERIN_O_REQ  (aerin_o_req),
		.AERIN_O_ACK  (aerin_o_ack),

		.AEROUT_I_ADDR(aerout_i_addr),
		.AEROUT_I_REQ (aerout_i_req),
		.AEROUT_I_ACK (aerout_i_ack),

		.AEROUT_O_ADDR(aerout_o_addr),
		.AEROUT_O_REQ (aerout_o_req),
		.AEROUT_O_ACK (aerout_o_ack),
		.AEROUT_O_LAST(aerout_o_last)
	);

	initial begin
		clk = 1'b0;
		forever #2 clk = ~clk;
	end

	task automatic reset();
		begin
			rstn          = 1'b0;
			core_busy     = 1'b0;
			aerin_i_addr  = '0;
			aerin_i_req   = 1'b0;
			aerin_i_last  = 1'b0;
			aerin_o_ack   = 1'b0;
			aerout_i_addr = '0;
			aerout_i_req  = 1'b0;
			aerout_o_ack  = 1'b0;

			repeat (2) @(posedge clk);
			rstn = 1'b1;
			repeat (2) @(posedge clk);
		end
	endtask

	task automatic aerin_producer(
		input int                           count,
		input logic [AERIN_DATA_WIDTH-1:0]  base_addr
	);
		begin
			for (int i = 0; i < count + 1; i++) begin
				aerin_i_last = (i == count);
				aerin_i_addr = aerin_i_last ? 0 : base_addr + AERIN_DATA_WIDTH'(i);
				aerin_i_req  = 1'b1;

				wait(aerin_i_ack == 1'b1);
				@(posedge clk);
				#0.01;

				aerin_i_req  = 1'b0;
				aerin_i_last = 1'b0;
				wait(aerin_i_ack == 1'b0);
				@(posedge clk);
				#0.01;
			end
			aerin_i_addr = '0;
		end
	endtask

	task automatic aerout_producer(
		input int                            count,
		input logic [AEROUT_DATA_WIDTH-1:0]  base_addr
	);
		begin
			for (int i = 0; i < count; i++) begin
                // similate the delay bc of processing the event
                repeat (4) @(posedge clk);
				
                aerout_i_addr = base_addr + AEROUT_DATA_WIDTH'(i);
				aerout_i_req  = 1'b1;

				wait(aerout_i_ack == 1'b1);
				@(posedge clk);
				#0.01;

				aerout_i_req = 1'b0;
				wait(aerout_i_ack == 1'b0);
				@(posedge clk);
				#0.01;
			end
			aerout_i_addr = '0;
		end
	endtask

	task automatic aerin_consumer();
		begin
			while (1) begin
				wait(aerin_o_req == 1'b1);
				@(posedge clk);
				aerin_o_ack = 1'b1;
				wait(aerin_o_req == 1'b0);
				@(posedge clk);
				aerin_o_ack = 1'b0;
			end
		end
	endtask

	task automatic aerout_consumer();
		begin
			while (1) begin
				wait(aerout_o_req == 1'b1);
				@(posedge clk);
				aerout_o_ack = 1'b1;
				wait(aerout_o_req == 1'b0);
				@(posedge clk);
				aerout_o_ack = 1'b0;
			end
		end
	endtask

	initial begin
		reset();

		fork
			aerin_consumer();
			aerout_consumer();
        join_none

        fork
            begin
                // packet 0 - 3 words
                aerin_producer(AERIN_TX_COUNT_0, 17'h0010);
                // packet 1 - no data only last
                aerin_producer(AERIN_TX_COUNT_1, 17'h0010);
            end
		join_none
		repeat (2) @(posedge clk);
		core_busy = 1'b1;

        // packet 0 output - 2 words
		aerout_producer(AEROUT_TX_COUNT_0, 8'h20);
		core_busy = 1'b0;
        

		repeat (20) @(posedge clk);
		$finish;
	end

endmodule
