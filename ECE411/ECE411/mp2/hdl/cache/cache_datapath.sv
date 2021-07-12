module cache_datapath #(
    parameter s_offset = 5,
    parameter s_index  = 3,
    parameter s_tag    = 32 - s_offset - s_index,
    parameter s_mask   = 2**s_offset,
    parameter s_line   = 8*s_mask,
    parameter num_sets = 2**s_index
)

(
    input logic clk,
    input rv32i_word mem_address,
    input rv32i_word mem_wdata,
    input logic read_data,
    input logic [1:0] sel_dirty,
    input logic [1:0] set_dirty,
    input logic [1:0] set_valid,
    input logic [1:0] flush_dirty,
    input logic [1:0] load_tag,
    input logic [1:0] load_data,
	 input logic [1:0] cache_read,
    input logic [s_line-1:0] pmem_rdata,
    input logic [1:0][s_mask-1:0] write_en,


    output rv32i_word mem_rdata,
    output logic [s_line-1:0] pmem_wdata,
    output logic [1:0] dirty_signal,
    output logic lru,
    output logic hit_0,
    output logic hit_1,
    output logic hit,
    output logic miss_0,
    output logic miss_1,
    output logic miss
);

rv32i_word mem_addr;
assign mem_addr = mem_address;

rv32i_word wdata;
assign wdata = mem_wdata;



/* SIGNALS */
logic [1:0][s_line-1:0] data_in;
logic [1:0][s_line-1:0] data_mux_in;
logic [1:0][s_line-1:0] cacheline_out;
logic [num_sets-1:0] decoder_out;
logic [1:0][s_tag-1:0] tag_signal;
logic [1:0] valid_signal;
logic [1:0] tag_hit;

 
/* ARRAY */
array lru_array(
    .clk,
    .read (1'b1),
    .load (cache_read[0] | cache_read[1] | load_data[0] | load_data[1]),
    .rindex  (mem_addr[7:5]),
    .windex  (mem_addr[7:5]),
    .datain  (cache_read[0] | load_data[0]),
    .dataout (lru)
);

array dirty[1:0] (
    .clk,
    .read (1'b1),
    .load (sel_dirty | flush_dirty),
    .rindex (mem_addr[7:5]),
    .windex (mem_addr[7:5]),
    .datain (set_dirty),
    .dataout (dirty_signal)
);

array valid[1:0] (
    .clk,
    .read (1'b1),
    .load (set_valid),
    .rindex  (mem_addr[7:5]),
    .windex  (mem_addr[7:5]),    
    .datain  (1'b1),
    .dataout (valid_signal)
);

array #(s_index, s_tag) tag[1:0] (
    .clk,
    .read (read_data),
    .load (load_tag),
    .rindex  (mem_addr[7:5]),
    .windex  (mem_addr[7:5]),
    .datain  (mem_addr[31:8]),
    .dataout (tag_signal)
);

data_array #(s_offset, s_index) line[1:0] (
    .clk,
    .read (read_data),
    .write_en (write_en),
    .rindex  (mem_addr[7:5]),
    .windex  (mem_addr[7:5]),
    .datain  (data_in),
    .dataout (cacheline_out)
);

cmp #(s_tag) tag_cmp[1:0] (
    .cmpop(beq),
    .a(tag_signal), 
    .b(mem_addr[31:8]),
    .br_en(tag_hit)
);

decoder decoder (
    .decoder_in(mem_addr[4:2]),
    .decoder_out
);

always_comb begin
    hit = hit_0 | hit_1;
    miss = miss_0 & miss_1;

    case (hit_0)
        0: pmem_wdata = cacheline_out[1];
        1: pmem_wdata = cacheline_out[0];
    endcase

    case (mem_address[4:2])
        0: mem_rdata = pmem_wdata[31:0];
        1: mem_rdata = pmem_wdata[63:32];
        2: mem_rdata = pmem_wdata[95:64];
        3: mem_rdata = pmem_wdata[127:96];
        4: mem_rdata = pmem_wdata[159:128];
        5: mem_rdata = pmem_wdata[191:160];
        6: mem_rdata = pmem_wdata[223:192];
        7: mem_rdata = pmem_wdata[255:224];
    endcase
end

always_comb begin
    hit_0 = valid_signal[0] & tag_hit[0];
    hit_1 = valid_signal[1] & tag_hit[1];
    miss_0= ~hit_0;
    miss_1 = ~hit_1;
    

    data_mux_in[0][31:0] = decoder_out[0] ? wdata[31:0] : cacheline_out[0][31:0]; 
    data_mux_in[0][63:32] = decoder_out[1] ? wdata[31:0] : cacheline_out[0][63:32];
    data_mux_in[0][95:64] = decoder_out[2] ? wdata[31:0] : cacheline_out[0][95:64];
    data_mux_in[0][127:96] = decoder_out[3] ? wdata[31:0] : cacheline_out[0][127:96];
    data_mux_in[0][159:128] = decoder_out[4] ? wdata[31:0] : cacheline_out[0][159:128];
    data_mux_in[0][191:160] = decoder_out[5] ? wdata[31:0] : cacheline_out[0][191:160];
    data_mux_in[0][223:192] = decoder_out[6] ? wdata[31:0] : cacheline_out[0][223:192];
    data_mux_in[0][255:224] = decoder_out[7] ? wdata[31:0] : cacheline_out[0][255:224];

    data_mux_in[1][31:0] = decoder_out[0] ? wdata[31:0] : cacheline_out[1][31:0]; 
    data_mux_in[1][63:32] = decoder_out[1] ? wdata[31:0] : cacheline_out[1][63:32];
    data_mux_in[1][95:64] = decoder_out[2] ? wdata[31:0] : cacheline_out[1][95:64];
    data_mux_in[1][127:96] = decoder_out[3] ? wdata[31:0] : cacheline_out[1][127:96];
    data_mux_in[1][159:128] = decoder_out[4] ? wdata[31:0] : cacheline_out[1][159:128];
    data_mux_in[1][191:160] = decoder_out[5] ? wdata[31:0] : cacheline_out[1][191:160];
    data_mux_in[1][223:192] = decoder_out[6] ? wdata[31:0] : cacheline_out[1][223:192];
    data_mux_in[1][255:224] = decoder_out[7] ? wdata[31:0] : cacheline_out[1][255:224];

    case (sel_dirty[0])
        0: data_in[0] = pmem_rdata;
        1: data_in[0] = data_mux_in[0];
    endcase

    case (sel_dirty[1])
        0: data_in[1] = pmem_rdata;
        1: data_in[1] = data_mux_in[1];
    endcase
end

endmodule : cache_datapath
