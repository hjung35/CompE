import rv32i_types::*;

module mp2
(
    input clk,
    input rst,
    input pmem_resp,
    input [63:0] pmem_rdata,
    output logic pmem_read,
    output logic pmem_write,
    output rv32i_word pmem_address,
    output [63:0] pmem_wdata
);

rv32i_word mem_wdata;
rv32i_word mem_rdata;
rv32i_word mem_address;

logic mem_resp;
logic mem_read;
logic mem_write;
logic [3:0] mem_byte_enable;

logic [255:0] cacheline_rdata;
logic [255:0] cacheline_wdata;
logic cacheline_read;
logic cacheline_write;
logic cacheline_resp;
logic cacheline_mem_address;


// Keep cpu named `cpu` for RVFI Monitor
// Note: you have to rename your mp2 module to `cpu`
cpu cpu(.*);

// Keep cache named `cache` for RVFI Monitor
cache cache(
	.pmem_rdata(cacheline_rdata),
	.pmem_wdata(cahceline_wdata),
	.*
);

// From MP0
cacheline_adaptor cacheline_adaptor(
	.clk,
   .reset_n(~rst),

   .line_i(cahceline_wdata),
   .line_o(cahceline_rdata),
   .address_i(cacheline_mem_address),
   .read_i(cacheline_read),
   .write_i(cacheline_write),
   .resp_o(cacheline_resp),
 
   .burst_i(pmem_rdata),
   .burst_o(pmem_wdata),
   .address_o(pmem_address),
   .read_o(pmem_read),
   .write_o(pmem_write),
   .resp_i(pmem_resp)
);

endmodule : mp2
