module cache_control (
  input logic clk, rst,

  // CPU <---> Control
  input logic mem_read, mem_write, pmem_resp,
  input logic [31:0] mem_address,
  output logic [31:0] pmem_address,
  output logic pmem_read, pmem_write, mem_resp,

  // Datapath <---> Control
  input logic cache_hit, dirty_dataout,
  input logic [23:0] tag_dataout,
  input [31:0] mem_byte_enable256,
  output logic data_read,
  output logic [31:0] data_write_en,
  output logic lru_read, lru_load,
  output logic tag_read, tag_load,
  output logic valid_read, valid_load, valid_datain,
  output logic dirty_read, dirty_load, dirty_datain,
  output logic comb_tag_read, comb_valid_read, comb_dirty_read, comb_data_read,
  output logic [1:0] way_index_sel
);

enum int unsigned {
  s_hit,
  s_read,
  s_write,
  s_miss,
  s_evict
} state, next_state;

function void set_defaults();
  pmem_read = 1'b0; 
  pmem_write = 1'b0; 
  mem_resp = 1'b0;

  data_read = 1'b0;
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
  dirty_read = 1'b0; 
  comb_dirty_read = 1'b0;
  dirty_load = 1'b0; 
  dirty_datain = 1'b0;
  way_index_sel = 2'b00;
  data_write_en = 32'h0;

  pmem_address = mem_address;
  next_state = state;
endfunction

always_comb begin
  set_defaults();
  case (state)
    s_hit: begin
      comb_tag_read = 1'b1;
      comb_valid_read = 1'b1;
      comb_data_read = 1'b1;
      tag_read = 1'b1;
      valid_read = 1'b1;
      dirty_read = 1'b1;
      lru_read = 1'b1;
      way_index_sel = 2'b00;
      if (cache_hit) begin
        lru_load = 1'b1;
        mem_resp = 1'b1;
      end
      if (cache_hit && mem_write) begin
        dirty_load = 1'b1; 
        dirty_datain = 1'b1;
        data_write_en = mem_byte_enable256;
      end
    end
    s_read: begin
        lru_read = 1'b1;
        way_index_sel = 2'b01;
        dirty_read = 1'b1;
        data_read = 1'b1;
        tag_read = 1'b1;
    end
    s_write: begin
        lru_read = 1'b1;
        way_index_sel = 2'b01;
        dirty_read = 1'b1;
        data_read = 1'b1;
        tag_read = 1'b1;
    end
    s_miss: begin
      pmem_read = 1'b1;
      data_read = 1'b1;
      way_index_sel = 2'b01;
      dirty_read = 1'b1;
      lru_read = 1'b1;
      tag_read = 1'b1;
      valid_read = 1'b1;
      if (pmem_resp) begin
        valid_datain = 1'b1; 
        valid_load = 1'b1;
        dirty_load = 1'b1; 
        dirty_datain = 1'b1; 
        data_write_en = 32'hFFFFFFFF;
        tag_load = 1'b1;
        lru_load = 1'b1;
      end
    end
    s_evict: begin
      pmem_write = 1'b1;
      way_index_sel = 2'b01;
      tag_read = 1'b1;
      pmem_address = {tag_dataout, mem_address[7:5], 5'b0};
      dirty_load = 1'b1; dirty_datain = 1'b0;
      valid_load = 1'b1; valid_datain = 1'b0;
    end
  endcase

  unique case (state)
    s_hit: begin
      if (~cache_hit && mem_read) next_state = s_read;
      else if (~cache_hit && mem_write) next_state = s_write;
    end
    s_read: begin
      if (cache_hit) next_state = s_hit;
      else if (~cache_hit & dirty_dataout) next_state = s_evict;
      else next_state = s_miss;
    end
    /*
      The actual logic behind this code (note cache_hit is always low):
      if (dirty_dataout) next_state = s_evict;
      else next_state = s_miss;
      But somehow quartus curshes if any changes are made
    */
    s_write: begin
      if (cache_hit) next_state = s_hit;
      else if (~cache_hit & dirty_dataout) next_state = s_evict;
      else next_state = s_miss;
    end
    s_miss: begin
      if (pmem_resp) begin
        if (mem_read) next_state = s_read;
        else if (mem_write) next_state = s_write;
      end
    end
    s_evict: begin
      if (pmem_resp) next_state = s_miss;
    end
  endcase
end

always_ff @(posedge clk) begin
  if (rst)
    state = s_hit;
  else
    state = next_state;
end

endmodule : cache_control
