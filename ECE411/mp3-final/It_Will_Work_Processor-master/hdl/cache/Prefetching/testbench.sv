
module pf_testbench;
`timescale 1ns/1ps

logic clk, rst;

// Cacheline Adaptor <---> Cache
logic pmem_resp;
logic [255:0] pmem_rdata;
logic [31:0] pmem_address;
logic [255:0] pmem_wdata;
logic pmem_read, pmem_write;
// Data enable signal for L2 cache
logic [31:0] byte_enable_L2;
// Cache <---> CPU
logic mem_read, mem_write;
logic [31:0] mem_wdata, mem_address;
logic mem_resp;
logic [31:0] mem_rdata;

Icache Icache(.*);

always #1 clk = ~clk;

initial begin
    clk <= '1;
    rst <= '1;

    // read from 0x60
    #2 rst <= 1'b0;
    mem_address <= 32'h60;
    mem_read <= 1'b1;

    #10 pmem_resp <= 1'b1;
    pmem_rdata <= 256'd1;

    #2 pmem_resp <= 1'b0;
    mem_read <= 1'b0;

    // read from 0x60 while preloading
    #2 mem_read <= 1'b1;

    #10 pmem_resp <= 1'b1;
    pmem_rdata <= 256'd2;

    #2 pmem_resp <= 1'b0;

    // read from 0x80    
    #10 mem_read <= 1'b0;
    #2 mem_read <= 1'b1;
    mem_address <= 32'h80;

    // read from 0xa0
    #10 mem_address <= 32'ha0;

    #10 pmem_resp <= 1'b1;
    pmem_rdata <= 256'd3;

    #2 pmem_resp <= 1'b0;
    mem_read <= 1'b0;

    // read from 0xa0 while preloading
    #2 mem_read <= 1'b1;

    #10 pmem_resp <= 1'b1;
    pmem_rdata <= 256'd2;

    #2 pmem_resp <= 1'b0;

    // read from 0xc0    
    #10 mem_read <= 1'b0;
    #2 mem_read <= 1'b1;
    mem_address <= 32'hc0;




    

end

endmodule
