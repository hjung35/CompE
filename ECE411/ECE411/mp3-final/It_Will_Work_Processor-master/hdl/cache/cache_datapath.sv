module cache_datapath #(
    parameter s_offset = 5,
    parameter s_index  = 3,
    parameter s_tag    = 32 - s_offset - s_index,
    parameter s_mask   = 2**s_offset,
    parameter s_line   = 8*s_mask,
    parameter num_sets = 2**s_index
)
(
    input logic clk, rst,
    // Control logic <---> Datapath
    input logic pmem_read,
    input logic data_read,
    input logic tag_load, tag_read,
    input logic valid_datain, valid_read, valid_load,
    input logic dirty_datain, dirty_read, dirty_load,
    input logic lru_load, lru_read,
    input logic [1:0] way_index_sel,
    // To allow one-cycle hit
    input logic comb_data_read, comb_tag_read, comb_valid_read, comb_dirty_read,
    output logic cache_hit,
    output logic dirty_dataout,
    input logic [31:0] data_write_en,

    // CPU <---> Datapath
    input logic mem_write,
    input logic [31:0] mem_byte_enable256,
    input logic [255:0] mem_wdata256,
    output logic [255:0] mem_rdata256,
    input logic [31:0] mem_address,

    // Main Memory <---> Datapath
    input logic [255:0] pmem_rdata,
    output logic [255:0] pmem_wdata,
    output logic [23:0] tag_dataout
);

logic [2:0] set_index;
assign set_index = mem_address[7:5];

logic [31:0] data0_write_en, data1_write_en;

logic way_index, lru_dataout;

logic [255:0] data_datain;

logic valid0_load, valid1_load, valid0_dataout, valid1_dataout;
logic valid0_dataout_comb, valid1_dataout_comb;

logic dirty0_load, dirty1_load, dirty0_dataout, dirty1_dataout;
logic dirty0_dataout_comb, dirty1_dataout_comb;

logic tag0_load, tag1_load;
logic [23:0] tag0_dataout, tag1_dataout;
logic [23:0] tag0_dataout_comb, tag1_dataout_comb;

logic [255:0] data0_dataout, data1_dataout;
logic [255:0] data0_dataout_comb, data1_dataout_comb;

data_array data0_array(
    .clk, .rst,
    .read(data_read),
    .write_en(data0_write_en),
    .rindex(set_index),
    .windex(set_index),
    .datain(data_datain),
    .dataout(data0_dataout),
    .dataout_comb(data0_dataout_comb)
);

data_array data1_array(
    .clk, .rst,
    .read(data_read),
    .write_en(data1_write_en),
    .rindex(set_index),
    .windex(set_index),
    .datain(data_datain),
    .dataout(data1_dataout),
    .dataout_comb(data1_dataout_comb)
);

array #(3, 24) tag0_array(
    .clk, .rst,
    .read(tag_read),
    .load(tag0_load),
    .rindex(set_index),
    .windex(set_index),
    .datain(mem_address[31:8]),
    .dataout(tag0_dataout),
    .dataout_comb(tag0_dataout_comb)
);

array #(3, 24) tag1_array(
    .clk, .rst,
    .read(tag_read),
    .load(tag1_load),
    .rindex(set_index),
    .windex(set_index),
    .datain(mem_address[31:8]),
    .dataout(tag1_dataout),
    .dataout_comb(tag1_dataout_comb)
);

array #(3, 1) valid0_array(
    .clk, .rst,
    .read(valid_read),
    .load(valid0_load),
    .rindex(set_index),
    .windex(set_index),
    .datain(valid_datain),
    .dataout(valid0_dataout),
    .dataout_comb(valid0_dataout_comb)
);

array #(3, 1) valid1_array(
    .clk, .rst,
    .read(valid_read),
    .load(valid1_load),
    .rindex(set_index),
    .windex(set_index),
    .datain(valid_datain),
    .dataout(valid1_dataout),
    .dataout_comb(valid1_dataout_comb)
);

array #(3, 1) dirty0_array(
    .clk, .rst,
    .read(dirty_read),
    .load(dirty0_load),
    .rindex(set_index),
    .windex(set_index),
    .datain(dirty_datain),
    .dataout(dirty0_dataout),
    .dataout_comb(dirty0_dataout_comb)
);

array #(3, 1) dirty1_array(
    .clk, .rst,
    .read(dirty_read),
    .load(dirty1_load),
    .rindex(set_index),
    .windex(set_index),
    .datain(dirty_datain),
    .dataout(dirty1_dataout),
    .dataout_comb(dirty1_dataout_comb)
);

array #(3, 1) lru_array(
    .clk, .rst,
    .read(lru_read),
    .load(lru_load),
    .rindex(set_index),
    .windex(set_index),
    .datain(~way_index),
    .dataout(lru_dataout),
    .dataout_comb()
);

logic CMP0, CMP1;
always_comb begin: COMPARATOR_LOGIC
    if (comb_valid_read && comb_tag_read) begin
        CMP0 = (mem_address[31:8] == tag0_dataout_comb) & valid0_dataout_comb;
        CMP1 = (mem_address[31:8] == tag1_dataout_comb) & valid1_dataout_comb;
    end
    else begin
        CMP0 = (mem_address[31:8] == tag0_dataout) & valid0_dataout;
        CMP1 = (mem_address[31:8] == tag1_dataout) & valid1_dataout;
    end
    cache_hit = CMP0 | CMP1;
end

always_comb begin: WAY_INDEX_MUX
    case (way_index_sel)
        2'b00: way_index = CMP1;
        2'b01: way_index = lru_dataout;
        2'b10: way_index = 1'b0;
        default: way_index = 1'b0;
    endcase
end

always_comb begin: WRITE_EN_MUX
    data0_write_en = (~way_index) ? data_write_en : 32'b0;
    data1_write_en = (way_index) ? data_write_en : 32'b0;
end

always_comb begin: DATAIN_MUXES
    data_datain = (pmem_read) ? pmem_rdata : mem_wdata256;
end

always_comb begin: LOAD_MUXES
    tag0_load = tag_load & ~way_index;
    tag1_load = tag_load & way_index;

    valid0_load = valid_load & ~way_index;
    valid1_load = valid_load & way_index;

    dirty0_load = dirty_load & ~way_index;
    dirty1_load = dirty_load & way_index;
end

always_comb begin: DATAOUT_MUXES
    tag_dataout = (way_index) ? tag1_dataout : tag0_dataout;
    
    if (!comb_dirty_read)
        dirty_dataout = (way_index) ? dirty1_dataout : dirty0_dataout;
    else
        dirty_dataout = (way_index) ? dirty1_dataout_comb : dirty0_dataout_comb;
    if (!comb_data_read)
        mem_rdata256 = (way_index) ? data1_dataout : data0_dataout;
    else
        mem_rdata256 = (way_index) ? data1_dataout_comb : data0_dataout_comb;

    pmem_wdata = (lru_dataout) ? data1_dataout : data0_dataout;
end

endmodule : cache_datapath
