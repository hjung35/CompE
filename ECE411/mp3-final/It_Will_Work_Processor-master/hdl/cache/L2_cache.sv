module L2_cache #(
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
    output logic [255:0] pmem_wdata,
    output logic pmem_read, pmem_write,

    // Cache <---> CPU
    input logic mem_read, mem_write,
    input logic [255:0] mem_wdata256,
    input logic [31:0] mem_byte_enable256, mem_address,
    output logic mem_resp,
    output logic [255:0] mem_rdata256

);

logic [23:0] tag_dataout;

logic cache_hit, lru_read, lru_load, tag_read, tag_load;
logic valid_read, valid_load, valid_datain;
logic dirty_read, dirty_load, dirty_datain, dirty_dataout;
logic data_read;
logic comb_tag_read, comb_valid_read, comb_dirty_read, comb_data_read;
logic [1:0] way_index_sel;
logic [31:0] data_write_en;
cache_control control
(.*);

cache_datapath datapath
(.*);


endmodule : L2_cache