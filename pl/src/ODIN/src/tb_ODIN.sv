`timescale 1ns / 1ps

module tb_ODIN;

    localparam int REG_LAST_ADDR = 27;
    localparam int NEURAM_NEURONS = 256;
    localparam int SYNRAM_WORDS  = 8192;
    localparam logic [127:0] NEUR_LIF_DISABLED = 128'h8000_0000_0000_0000_0000_0000_0000_0001;

    localparam logic [1:0] BRAM_REGMEM = 2'b00;
    localparam logic [1:0] BRAM_NEURAM = 2'b01;
    localparam logic [1:0] BRAM_SYNRAM = 2'b10;

    logic         clk;
    logic         rstn;

    logic         rstn_out;
    logic         busy;

    logic         bram_mem_clk;
    logic         bram_mem_rst;
    logic         bram_mem_en;
    logic [3:0]   bram_mem_we;
    logic [16:0]  bram_mem_addr;
    logic [31:0]  bram_mem_din;
    logic [31:0]  bram_mem_dout;

    logic [16:0]  aerin_addr;
    logic         aerin_req;
    logic         aerin_ack;

    logic [7:0]   aerout_addr;
    logic         aerout_req;
    logic         aerout_ack;

    integer       out_spike_count;
    logic [7:0]   last_spike_addr;
    integer       spike_count_by_neur [0:255];

    logic [31:0]  rd_data;
    logic [31:0]  wr_data;
    logic [31:0]  wr_mask;
    logic [127:0] rd_neur_data;
    logic [127:0] wr_neur_data;

    integer       idx;
    integer       neur_idx;

    initial begin
        clk = 1'b0;
        forever #1 clk = ~clk;
    end

    assign bram_mem_clk = clk;

    function automatic logic [16:0] mk_bram_addr(
        input logic [1:0] region,
        input logic [12:0] word_addr
    );
        mk_bram_addr = {region, word_addr, 2'b00};
    endfunction

    function automatic logic [16:0] mk_neuron_event(input logic [7:0] neuron_idx);
        mk_neuron_event = {1'b0, neuron_idx, 8'h07};
    endfunction

    function automatic logic [31:0] reg_wmask(input logic [12:0] addr);
        case (addr)
            13'd0,
            13'd1: reg_wmask = 32'h0000_0001;
    
            13'd2,
            13'd3,
            13'd4,
            13'd5,
            13'd6,
            13'd7,
            13'd8,
            13'd9: reg_wmask = 32'hFFFF_FFFF;

            13'd18: reg_wmask = 32'h000F_FFFF;
            
            13'd19,
            13'd20: reg_wmask = 32'h0000_0001;
            
            13'd21,
            13'd22: reg_wmask = 32'h0000_00FF;

            13'd23,
            13'd24,
            13'd25: reg_wmask = 32'h0000_0001;

            13'd26,
            13'd27: reg_wmask = 32'h0000_0000;

            default: reg_wmask = 32'h0000_0000;
        endcase
    endfunction

    function automatic logic [31:0] reg_test_pattern(input integer addr);
        reg_test_pattern = 32'hFFFF_FFFF;
    endfunction

    function automatic logic [31:0] mem_test_pattern(input integer addr, input logic [31:0] m);
        mem_test_pattern = m ^ (32'(addr) * 32'hDEAD_BEEF);
    endfunction

    task automatic reset_dut;
        begin
            rstn          = 1'b0;
            bram_mem_rst  = 1'b1;
            bram_mem_en   = 1'b0;
            bram_mem_we   = 4'b0;
            bram_mem_addr = '0;
            bram_mem_din  = '0;
            aerin_addr    = '0;
            aerin_req     = 1'b0;
            aerout_ack    = 1'b0;
            out_spike_count = 0;
            last_spike_addr = '0;
            clear_spike_counters();

            repeat (5) @(posedge clk);
            bram_mem_rst = 1'b0;
            rstn         = 1'b1;
            repeat (5) @(posedge clk);
        end
    endtask

    task automatic clear_spike_counters;
        begin
            out_spike_count = 0;
            last_spike_addr = '0;
            for (neur_idx = 0; neur_idx < 256; neur_idx = neur_idx + 1)
                spike_count_by_neur[neur_idx] = 0;
        end
    endtask

    task automatic bram_write32(
        input logic [1:0] region,
        input logic [12:0] word_addr,
        input logic [31:0] data
    );
        begin
            @(posedge clk);
            bram_mem_en   <= 1'b1;
            bram_mem_we   <= 4'hF;
            bram_mem_addr <= mk_bram_addr(region, word_addr);
            bram_mem_din  <= data;

            @(posedge clk);
            bram_mem_en   <= 1'b0;
            bram_mem_we   <= 4'h0;
            bram_mem_addr <= '0;
            bram_mem_din  <= '0;
        end
    endtask

    task automatic bram_read32(
        input logic [1:0] region,
        input logic [12:0] word_addr,
        output logic [31:0] data
    );
        begin
            @(posedge clk);
            bram_mem_en   <= 1'b1;
            bram_mem_we   <= 4'h0;
            bram_mem_addr <= mk_bram_addr(region, word_addr);

            @(posedge clk);
            @(posedge clk);
            data = bram_mem_dout;

            bram_mem_en   <= 1'b0;
            bram_mem_addr <= '0;
            @(posedge clk);
        end
    endtask

    task automatic neuram_write128(
        input logic [7:0] neur_idx,
        input logic [127:0] data
    );
        logic [12:0] chunk_addr;
        begin
            chunk_addr = {3'b000, 2'b00, neur_idx};
            bram_write32(BRAM_NEURAM, chunk_addr, data[ 31:  0]);

            chunk_addr = {3'b000, 2'b01, neur_idx};
            bram_write32(BRAM_NEURAM, chunk_addr, data[ 63: 32]);

            chunk_addr = {3'b000, 2'b10, neur_idx};
            bram_write32(BRAM_NEURAM, chunk_addr, data[ 95: 64]);

            chunk_addr = {3'b000, 2'b11, neur_idx};
            bram_write32(BRAM_NEURAM, chunk_addr, data[127: 96]);
        end
    endtask

    task automatic neuram_read128(
        input logic [7:0] neur_idx,
        output logic [127:0] data
    );
        logic [12:0] chunk_addr;
        logic [31:0] d0, d1, d2, d3;
        begin
            chunk_addr = {3'b000, 2'b00, neur_idx};
            bram_read32(BRAM_NEURAM, chunk_addr, d0);

            chunk_addr = {3'b000, 2'b01, neur_idx};
            bram_read32(BRAM_NEURAM, chunk_addr, d1);

            chunk_addr = {3'b000, 2'b10, neur_idx};
            bram_read32(BRAM_NEURAM, chunk_addr, d2);

            chunk_addr = {3'b000, 2'b11, neur_idx};
            bram_read32(BRAM_NEURAM, chunk_addr, d3);

            data = {d3, d2, d1, d0};
        end
    endtask

    task automatic set_lif_neuron(
        input logic [7:0] neur_idx,
        input logic [7:0] threshold,
        input logic [7:0] init_v,
        input logic       leak_en,
        input logic [6:0] leak_str,
        input logic       neur_disable
    );
        logic [127:0] cfg;
        begin
            cfg = 128'h0;
            cfg[0]     = 1'b1;
            cfg[16:9]  = threshold;
            cfg[77:70] = init_v;
            cfg[8]     = leak_en;
            cfg[7:1]   = leak_str;
            cfg[127]   = neur_disable;
            neuram_write128(neur_idx, cfg);
        end
    endtask

    task automatic set_all_neurons_lif_disabled;
        begin
            for (idx = 0; idx < NEURAM_NEURONS; idx = idx + 1) begin
                set_lif_neuron(
                    idx[7:0],
                    8'd0,
                    8'd0,
                    1'b0,
                    7'd0,
                    1'b1
                );

                if ((idx & 8'h3F) == 0)
                    $display("NEURAM disable progress: neuron %0d/%0d", idx, NEURAM_NEURONS);
            end
        end
    endtask
    
    function automatic logic [12:0] syn_word_addr(
        input logic [7:0] dendrite,
        input logic [7:0] axon
    );
        syn_word_addr = {axon, dendrite[7:3]};
    endfunction

    task automatic set_synapse(
        input logic [7:0] dendrite,
        input logic [7:0] axon,
        input logic [2:0] weight,
        input logic       mapping_table_bit 
    );
        logic [12:0] addr;
        logic [31:0] word_data;
        int lsb;
        begin
            addr = syn_word_addr(dendrite, axon);
            lsb = {dendrite[2:0], 2'b00};

            bram_read32(BRAM_SYNRAM, addr, word_data);
            word_data[lsb +: 4] = {mapping_table_bit, weight};
            bram_write32(BRAM_SYNRAM, addr, word_data);
            bram_read32(BRAM_SYNRAM, addr, rd_data);

            assert (rd_data[lsb +: 4] == {mapping_table_bit, weight})
                else $fatal(1, "SYNRAM set_synapse failed: dend=%0d axon=%0d exp=%0d got=%0d",
                            dendrite, axon, {mapping_table_bit, weight}, rd_data[lsb +: 4]);
        end
    endtask

    task automatic aer_send(input logic [16:0] event_addr);
        begin
            wait (aerin_ack == 1'b0);
            aerin_addr = event_addr;
            aerin_req  = 1'b1;

            wait (aerin_ack == 1'b1);
            @(posedge clk);
            // #0.01;

            aerin_req  = 1'b0;
            wait (aerin_ack == 1'b0);
            @(posedge clk);
            // #0.01;
        end
    endtask

    task automatic aerout_auto_ack;
        begin
            forever begin
                wait (aerout_req == 1'b1);
                @(posedge clk);
                #0.01;

                aerout_ack = 1'b1;

                wait (aerout_req == 1'b0);
                @(posedge clk);
                #0.01;
                
                aerout_ack = 1'b0;
                
                out_spike_count = out_spike_count + 1;
                last_spike_addr = aerout_addr;
                spike_count_by_neur[aerout_addr] = spike_count_by_neur[aerout_addr] + 1;
            end
        end
    endtask

    task automatic test_bram_full_rw;
        begin
            $display("--- REGMEM full write/read test");
            for (idx = 0; idx <= REG_LAST_ADDR; idx = idx + 1) begin
                wr_mask = reg_wmask(idx[12:0]);
                wr_data = reg_test_pattern(idx) & wr_mask;

                if (idx != 27)
                    bram_write32(BRAM_REGMEM, idx[12:0], wr_data);

                bram_read32(BRAM_REGMEM, idx[12:0], rd_data);

                if (idx == 27) begin
                    $display("REG[27] (busy, read-only) = 0x%08x", rd_data);
                end else begin
                    assert (rd_data == wr_data)
                        else $fatal(1, "REGMEM mismatch at addr %0d: exp=0x%08x got=0x%08x", idx, wr_data, rd_data);
                end
            end
            
            $display("--- Setting GATE_ACTIVITY to 1");
            bram_write32(BRAM_REGMEM, 13'd0, 32'h0000_0001);

            $display("--- NEURAM full 128-bit write/read test (neurons 0..%0d)", NEURAM_NEURONS-1);
            for (idx = 0; idx < NEURAM_NEURONS; idx = idx + 1) begin
                wr_neur_data[ 31:  0] = mem_test_pattern((idx << 2) + 0, 32'h1357_2468);
                wr_neur_data[ 63: 32] = mem_test_pattern((idx << 2) + 1, 32'h2468_1357);
                wr_neur_data[ 95: 64] = mem_test_pattern((idx << 2) + 2, 32'h55AA_33CC);
                wr_neur_data[127: 96] = mem_test_pattern((idx << 2) + 3, 32'hAA55_CC33);
                neuram_write128(idx[7:0], wr_neur_data);

                if ((idx & 8'h3F) == 0)
                    $display("NEURAM write progress: neuron %0d/%0d", idx, NEURAM_NEURONS);
            end
            for (idx = 0; idx < NEURAM_NEURONS; idx = idx + 1) begin
                wr_neur_data[ 31:  0] = mem_test_pattern((idx << 2) + 0, 32'h1357_2468);
                wr_neur_data[ 63: 32] = mem_test_pattern((idx << 2) + 1, 32'h2468_1357);
                wr_neur_data[ 95: 64] = mem_test_pattern((idx << 2) + 2, 32'h55AA_33CC);
                wr_neur_data[127: 96] = mem_test_pattern((idx << 2) + 3, 32'hAA55_CC33);
                neuram_read128(idx[7:0], rd_neur_data);
                assert (rd_neur_data == wr_neur_data)
                    else $fatal(1, "NEURAM mismatch at neuron %0d: exp=0x%032x got=0x%032x", idx, wr_neur_data, rd_neur_data);

                if ((idx & 8'h3F) == 0)
                    $display("NEURAM read progress: neuron %0d/%0d", idx, NEURAM_NEURONS);
            end

            $display("--- SYNRAM full write/read test (0..%0d)", SYNRAM_WORDS-1);
            for (idx = 0; idx < SYNRAM_WORDS; idx = idx + 1) begin
                wr_data = mem_test_pattern(idx, 32'h89AB_CDEF);
                bram_write32(BRAM_SYNRAM, idx[12:0], wr_data);

                if ((idx & 13'h3FF) == 0)
                    $display("SYNRAM write progress: %0d/%0d", idx, SYNRAM_WORDS);
            end
            for (idx = 0; idx < SYNRAM_WORDS; idx = idx + 1) begin
                wr_data = mem_test_pattern(idx, 32'h89AB_CDEF);
                bram_read32(BRAM_SYNRAM, idx[12:0], rd_data);
                assert (rd_data == wr_data)
                    else $fatal(1, "SYNRAM mismatch at addr %0d: exp=0x%08x got=0x%08x", idx, wr_data, rd_data);

                if ((idx & 13'h3FF) == 0)
                    $display("SYNRAM read progress: %0d/%0d", idx, SYNRAM_WORDS);
            end

            $display("--- Setting GATE_ACTIVITY to 0");
            bram_write32(BRAM_REGMEM, 13'd0, 32'h0000_0000);

            $display("--- All register/NEURAM/SYNRAM checks passed");
        end
    endtask

    task automatic test_bram_full_zero;
        begin
            $display("--- Zeroing BRAM-mapped memories");

            for (idx = 0; idx <= REG_LAST_ADDR; idx = idx + 1) begin
                if (idx != 27)
                    bram_write32(BRAM_REGMEM, idx[12:0], 32'h0000_0000);
            end
            for (idx = 0; idx <= REG_LAST_ADDR; idx = idx + 1) begin
                bram_read32(BRAM_REGMEM, idx[12:0], rd_data);
                if (idx != 27) begin
                    assert (rd_data == 32'h0000_0000)
                        else $fatal(1, "REGMEM zero check failed at addr %0d: got=0x%08x", idx, rd_data);
                end
            end
            
            $display("--- Setting GATE_ACTIVITY to 1");
            bram_write32(BRAM_REGMEM, 13'd0, 32'h0000_0001);

            $display("--- Setting all neurons to default disabled");
            set_all_neurons_lif_disabled();
            for (idx = 0; idx < NEURAM_NEURONS; idx = idx + 1) begin
                neuram_read128(idx[7:0], rd_neur_data);
                assert (rd_neur_data == NEUR_LIF_DISABLED)
                    else $fatal(1, "NEURAM default-disabled check failed at neuron %0d: got=0x%032x", idx, rd_neur_data);

                if ((idx & 8'h3F) == 0)
                    $display("NEURAM zero read progress: neuron %0d/%0d", idx, NEURAM_NEURONS);
            end

            for (idx = 0; idx < SYNRAM_WORDS; idx = idx + 1) begin
                bram_write32(BRAM_SYNRAM, idx[12:0], 32'h0000_0000);

                if ((idx & 13'h3FF) == 0)
                    $display("SYNRAM zero progress: %0d/%0d", idx, SYNRAM_WORDS);
            end
            for (idx = 0; idx < SYNRAM_WORDS; idx = idx + 1) begin
                bram_read32(BRAM_SYNRAM, idx[12:0], rd_data);
                assert (rd_data == 32'h0000_0000)
                    else $fatal(1, "SYNRAM zero check failed at addr %0d: got=0x%08x", idx, rd_data);

                if ((idx & 13'h3FF) == 0)
                    $display("SYNRAM zero read progress: %0d/%0d", idx, SYNRAM_WORDS);
            end
            
            $display("--- Setting GATE_ACTIVITY to 0");
            bram_write32(BRAM_REGMEM, 13'd0, 32'h0000_0000);

            $display("--- Zeroing complete");
        end
    endtask

    task automatic test_open_loop_two_lif;
        begin
            $display("--- Open-loop test: 2 LIF neurons, 3 synapses");

            clear_spike_counters();

            bram_write32(BRAM_REGMEM, 13'd0, 32'h0000_0001); // set GATE_ACTIVITY to 1
            bram_write32(BRAM_REGMEM, 13'd1, 32'h0000_0001); // set OPEN_LOOP to 1

            set_lif_neuron(8'd0, 8'd5, 8'd0, 1'b0, 7'd0, 1'b0); // neuron 0, threshold 5, enabled
            set_lif_neuron(8'd1, 8'd4, 8'd0, 1'b0, 7'd0, 1'b0); // neuron 1, threshold 4, enabled

            set_synapse(8'd0, 8'd10, 4'd3, 1'b1); // synapse from neuron 10 to neuron 0, weight 3, mapping table bit 1 - weight enabled
            set_synapse(8'd0, 8'd11, 4'd3, 1'b1); // synapse from neuron 11 to neuron 0, weight 3, mapping table bit 1 - weight enabled
            set_synapse(8'd1, 8'd10, 4'd4, 1'b1); // synapse from neuron 10 to neuron 1, weight 4, mapping table bit 1 - weight enabled

            bram_write32(BRAM_REGMEM, 13'd0, 32'h0000_0000); // set GATE_ACTIVITY to 0


            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));
            
            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));

            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));
            
            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));

            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));
            
            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));

            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));
            
            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));


            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));
            
            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));

            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));
            
            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));

            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));
            
            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));

            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));
            
            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));


            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));
            
            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));

            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));
            
            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));

            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));
            
            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));

            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));
            
            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));


            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));
            
            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));

            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));
            
            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));

            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));
            
            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));

            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));
            
            aer_send(mk_neuron_event(8'd10));
            aer_send(mk_neuron_event(8'd11));

            wait (!busy);

            repeat (5) @(posedge clk);

            // neuron 1 should spike first (threshold 4, weight 4), then neuron 0 (threshold 5, weights 3+3=6)

            // assert (spike_count_by_neur[0] == 1)
            //    else $fatal(1, "Open-loop test failed: neuron 0 did not spike");
            // assert (spike_count_by_neur[1] == 1)
            //    else $fatal(1, "Open-loop test failed: neuron 1 did not spike");

            // $display("Open-loop spikes: neuron0=%0d neuron1=%0d total=%0d",
            //         spike_count_by_neur[0], spike_count_by_neur[1], out_spike_count);
        end
    endtask

    initial begin
        reset_dut();

        fork
            aerout_auto_ack();
        join_none

        test_bram_full_rw();
        test_bram_full_zero();
        test_open_loop_two_lif();

        $display("--- Done");
        $finish;
    end

    ODIN #(
        .N(256),
        .M(8)
    ) dut (
        .CLK           (clk),
        .RSTN          (rstn),
        .RSTN_OUT      (rstn_out),
        .BUSY          (busy),

        .BRAM_MEM_CLK  (bram_mem_clk),
        .BRAM_MEM_RST  (bram_mem_rst),
        .BRAM_MEM_EN   (bram_mem_en),
        .BRAM_MEM_WE   (bram_mem_we),
        .BRAM_MEM_ADDR (bram_mem_addr),
        .BRAM_MEM_DIN  (bram_mem_din),
        .BRAM_MEM_DOUT (bram_mem_dout),

        .AERIN_ADDR    (aerin_addr),
        .AERIN_REQ     (aerin_req),
        .AERIN_ACK     (aerin_ack),

        .AEROUT_ADDR   (aerout_addr),
        .AEROUT_REQ    (aerout_req),
        .AEROUT_ACK    (aerout_ack)
    );

endmodule
