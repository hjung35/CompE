module Icache_ctrl (
  input logic clk, rst,

  // CPU <---> Control
  input logic mem_read, mem_write, pmem_resp,
  input logic [31:0] mem_address,
  output logic [31:0] pmem_address,
  output logic pmem_read, pmem_write, mem_resp,

  // Datapath <---> Control
  input logic cache_hit,
  input logic next_line_cached,
  output logic preload_start, preload_done,
  output logic data_read, data_load,
  output logic lru_read, lru_load,
  output logic tag_read, tag_load,
  output logic valid_read, valid_load, valid_datain,
  output logic comb_tag_read, comb_valid_read, comb_data_read,
  output logic [1:0] way_index_sel
);

reg [31:0] addr_in_preload;
logic [31:0] addr_in_preload_in;

enum int unsigned {
  s_hit,
  s_load,
  s_preload
} state, next_state;

function void set_defaults();
  pmem_read = 1'b0; 
  pmem_write = 1'b0; 
  mem_resp = 1'b0;

  data_read = 1'b0;
  data_load = 1'b0;
  comb_data_read = 1'b0;
  lru_read = 1'b0; 
  lru_load = 1'b0;
  tag_read = 1'b0;
  comb_tag_read = 1'b0; 
  tag_load = 1'b0;
  valid_read = 1'b0;
  comb_valid_read = 1'b0; 
  valid_load = 1'b0; 
  valid_datain = 1'b0;
  way_index_sel = 2'b00;

  pmem_address = mem_address;

  //prefetching signals
  preload_start = 1'b0;
  preload_done = 1'b0;
  addr_in_preload_in = 32'd0;

endfunction

always_comb begin
  set_defaults();
  case (state)
    s_hit: begin
      comb_tag_read = 1'b1;
      comb_valid_read = 1'b1;
      comb_data_read = 1'b1;
      lru_read = 1'b1;
      if (cache_hit) begin
        lru_load = 1'b1;
        mem_resp = 1'b1;
      end
      way_index_sel = 2'b00;
    end
    s_load: begin
      if (pmem_resp) begin
        valid_load = 1'b1;
        valid_datain = 1'b1;
        tag_load = 1'b1;
        data_load = 1'b1;
        way_index_sel = 2'b01;
        lru_read = 1'b1;
        lru_load = 1'b1;
        mem_resp = 1'b1;
        comb_data_read = 1'b1;
        preload_start = ~next_line_cached;
        addr_in_preload_in = pmem_address + 32'd32;
      end
      else begin
        pmem_read = 1'b1;
        comb_data_read = 1'b1;
        // for setting next_line_cached in Icache_dp
        data_read = 1'b1;
        lru_read = 1'b1;
        way_index_sel = 2'b01;
        valid_read = 1'b1;
      end
    end
    s_preload: begin
      if (pmem_resp) begin
        preload_done = 1'b1;
        pmem_address = addr_in_preload;
        data_load = 1'b1;
        valid_load = 1'b1;
        valid_datain = 1'b1; 
        tag_load = 1'b1;
        lru_load = 1'b1;
      end
      else begin
        pmem_read = 1'b1;
        pmem_address = addr_in_preload;
        preload_done = pmem_resp;
        data_read = 1'b1;
        way_index_sel = 2'b00;
        valid_read = 1'b1;
        lru_read = 1'b1;
        tag_read = 1'b1;
      end
    end
  endcase
end

always_comb begin
  next_state = state;
  unique case (state)
    s_hit: begin
      if (~cache_hit && mem_read) next_state = s_load;
    end
    s_load: begin
      if (pmem_resp) begin
        if (next_line_cached) next_state = s_hit;
        else next_state = s_preload;
      end
    end
    s_preload: begin
        if (pmem_resp) next_state = s_hit;
      end
  endcase
end

always_ff @(posedge clk) begin
  if (rst)
    state <= s_hit;
  else
    state <= next_state;
  if (preload_start)
    addr_in_preload <= addr_in_preload_in;

end

endmodule : Icache_ctrl
