module cache #(
    parameter s_offset = 5,
    parameter s_index  = 3,
    parameter s_tag    = 32 - s_offset - s_index,
    parameter s_mask   = 2**s_offset,
    parameter s_line   = 8*s_mask,
    parameter num_sets = 2**s_index
)

(
    input clk,
    
    /* Cache <-> Physical Memory */
    input pmem_resp,
    input [255:0] pmem_rdata,
    output [255:0] pmem_wdata,
    output logic pmem_read,
    output logic pmem_write,
    output rv32i_word pmem_address,
    
    /* Cache <-> CPU */
    input logic mem_read,
    input logic mem_write,
    input logic [3:0] mem_byte_enable,
    input rv32i_word mem_address,
    input rv32i_word mem_wdata,
    output rv32i_word mem_rdata,
    output mem_resp
);

/* Signals */
logic read_data;
logic [1:0] sel_dirty;
logic [1:0] set_dirty;
logic [1:0] set_valid;
logic [1:0] flush_dirty;
logic [1:0] load_tag;
logic [1:0] load_data;
logic [1:0] cache_read;
logic [1:0] dirty_signal;

logic lru;
logic hit_0;
logic hit_1;
logic hit;
logic miss_0;
logic miss_1;
logic miss;
logic [s_mask-1:0] mem_byte_enable256;
logic [1:0] [s_mask-1:0] write_en;

always_comb begin

	 write_en = 0;

    case (load_data)
        2'b01: write_en[0] = (mem_byte_enable256 == 0) ? {s_mask{1'b1}} : mem_byte_enable256;
        2'b10: write_en[1] = (mem_byte_enable256 == 0) ? {s_mask{1'b1}} : mem_byte_enable256;
    endcase
end

cache_control control(
    .*
);

cache_datapath datapath(
    .*,
    .dirty_signal(dirty_signal)
);

bus_adapter bus_adapter(
    .mem_wdata256(),
    .mem_rdata256(line_data),
    .mem_wdata,
    .mem_rdata,
    .mem_byte_enable,
    .mem_byte_enable256,
    .address(mem_address)
);

endmodule : cache
