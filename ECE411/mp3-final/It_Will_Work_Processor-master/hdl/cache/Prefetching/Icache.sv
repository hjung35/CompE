module Icache #(
    parameter s_offset = 5,
    parameter s_index  = 3,
    parameter s_tag    = 32 - s_offset - s_index,
    parameter s_mask   = 2**s_offset,
    parameter s_line   = 8*s_mask,
    parameter num_sets = 2**s_index
)
(
    input logic clk, rst,

    // Cacheline Adaptor <---> Cache
    input logic pmem_resp,
    input logic [255:0] pmem_rdata,
    output logic [31:0] pmem_address,
    output logic pmem_read, pmem_write,
    // Data enable signal for L2 cache
    output logic [31:0] byte_enable_L2,

    // Cache <---> CPU
    input logic mem_read, mem_write,
    input logic [31:0] mem_wdata, mem_address,
    output logic mem_resp,
    output logic [31:0] mem_rdata

);

logic [255:0] mem_rdata256;
assign byte_enable_L2 = 32'hffffffff;

logic cache_hit, lru_read, lru_load, tag_read, tag_load;
logic valid_read, valid_load, valid_datain;
logic data_read ,data_load;
logic comb_tag_read, comb_valid_read, comb_data_read;
logic [1:0] way_index_sel;
logic next_line_cached, preload_start, preload_done;
Icache_ctrl control
(.*);

Icache_dp datapath
(.*);

bus_adapter bus_adapter
(
    .address(mem_address),
    .mem_rdata256,
    .mem_rdata,
    .mem_byte_enable(4'hf),
    .mem_wdata(),
    .mem_wdata256(),
    .mem_byte_enable256()
);

endmodule : Icache