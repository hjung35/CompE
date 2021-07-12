import rv32i_types::*;

module stage_reg (
  input logic clk, rst, load,
  input stage_reg_t stage_reg_in,
  output stage_reg_t stage_reg_out
);

stage_reg_t data;

always_comb begin
  if (load) data = stage_reg_in;
  else data = stage_reg_out;
end

always_ff @(posedge clk) begin
  if (rst) stage_reg_out = 0;
  else stage_reg_out = data; 
end

endmodule