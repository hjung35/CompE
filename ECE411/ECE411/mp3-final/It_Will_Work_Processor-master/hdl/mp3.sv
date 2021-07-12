import rv32i_types::*;

module mp3 (
  input logic clk, rst,
  input logic mem_resp, 
  input logic [63:0] mem_rdata,
  output logic mem_read, mem_write, 
  output logic [31:0] mem_addr,
  output logic [63:0] mem_wdata
);

// Wires from arbiter to Icache
logic [31:0] inst_addr_arbiter;
logic inst_resp_arbiter;
logic [255:0] inst_rdata_arbiter;
logic inst_read_arbiter;

// Wires from Icache to cpu
logic [31:0] inst_addr_cache;
logic inst_resp_cache;
logic [31:0] inst_rdata_cache;
logic inst_read_cache;
logic [31:0] inst_byte_enable256;

// Wires from arbiter to Dcache
logic [31:0] data_addr_arbiter;
logic data_resp_arbiter;
logic [255:0] data_rdata_arbiter;
logic data_read_arbiter;
logic data_write_arbiter;
logic [255:0] data_wdata_arbiter;
logic [31:0] data_byte_enable256;

// Wires from Dcache to cpu
logic [31:0] data_addr_cache;
logic data_resp_cache;
logic [31:0] data_rdata_cache;
logic data_read_cache;
logic data_write_cache;
logic [3:0] data_mbe_cache;
logic [31:0] data_wdata_cache;

// Wires between Arbiter to L2 cache
logic mem_read_L2, mem_write_L2, mem_resp_L2;
logic [255:0] mem_rdata_L2, mem_wdata_L2;
logic [31:0] mem_address_L2, mem_byte_enable_L2;


// Wires from L2 Cache to cacheline adaptor
logic [255:0] line_i;
logic [255:0] line_o;
logic [31:0] address_i;
logic read_i;
logic write_i;
logic resp_o;



cpu cpu(
  .clk, .rst,
  .inst_resp(inst_resp_cache),
  .inst_rdata(inst_rdata_cache),
  .inst_addr(inst_addr_cache),
  .inst_read(inst_read_cache),
  .data_resp(data_resp_cache),
  .data_rdata(data_rdata_cache),
  .data_addr(data_addr_cache),
  .data_read(data_read_cache),
  .data_wdata(data_wdata_cache),
  .data_write(data_write_cache),
  .data_mbe(data_mbe_cache)
);

Icache inst_cache(
  .clk, .rst,
  .pmem_resp(inst_resp_arbiter),
  .pmem_rdata(inst_rdata_arbiter),
  .pmem_address(inst_addr_arbiter),
  .pmem_read(inst_read_arbiter),
  .byte_enable_L2(inst_byte_enable256),
  .mem_read(inst_read_cache),
  .mem_address(inst_addr_cache),
  .mem_resp(inst_resp_cache),
  .mem_rdata(inst_rdata_cache)
);

cache data_cache(
  .clk, .rst,
  .pmem_resp(data_resp_arbiter),
  .pmem_rdata(data_rdata_arbiter),
  .pmem_address(data_addr_arbiter),
  .pmem_wdata(data_wdata_arbiter),
  .pmem_read(data_read_arbiter),
  .pmem_write(data_write_arbiter),
  .byte_enable_L2(data_byte_enable256),
  .mem_read(data_read_cache),
  .mem_write(data_write_cache),
  .mem_address(data_addr_cache),
  .mem_resp(data_resp_cache),
  .mem_rdata(data_rdata_cache),
  .mem_wdata(data_wdata_cache),
  .mem_byte_enable(data_mbe_cache)
);

cache_arbiter cache_arbiter(
  .clk, .rst,
  .inst_read(inst_read_arbiter),
  .inst_addr(inst_addr_arbiter),
  .inst_resp(inst_resp_arbiter),
  .inst_rdata(inst_rdata_arbiter),
  .inst_byte_enable256,
  .data_read(data_read_arbiter),
  .data_addr(data_addr_arbiter),
  .data_resp(data_resp_arbiter),
  .data_rdata(data_rdata_arbiter),
  .data_write(data_write_arbiter),
  .data_wdata(data_wdata_arbiter),
  .data_byte_enable256,
  .mem_resp(mem_resp_L2),
  .mem_addr(mem_address_L2),
  .mem_read(mem_read_L2),
  .mem_write(mem_write_L2),
  .mem_rdata(mem_rdata_L2),
  .mem_wdata(mem_wdata_L2),
  .mem_byte_enable(mem_byte_enable_L2)
);

L2_cache L2_cache(
  .clk, .rst,
  .pmem_resp(resp_o),
  .pmem_rdata(line_o),
  .pmem_address(address_i),
  .pmem_wdata(line_i),
  .pmem_read(read_i),
  .pmem_write(write_i),
  .mem_read(mem_read_L2),
  .mem_write(mem_write_L2),
  .mem_address(mem_address_L2),
  .mem_byte_enable256(mem_byte_enable_L2),
  .mem_resp(mem_resp_L2),
  .mem_rdata256(mem_rdata_L2),
  .mem_wdata256(mem_wdata_L2)

);


cacheline_adaptor cacheline_adaptor(
  .reset_n(~rst), 
  .burst_i(mem_rdata),
  .burst_o(mem_wdata),
  .address_o(mem_addr),
  .read_o(mem_read),
  .write_o(mem_write),
  .resp_i(mem_resp),
  .*);


endmodule: mp3