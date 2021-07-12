import rv32i_types::*;

module cpu(
  input logic clk, rst,

  input logic inst_resp,
  input logic [31:0] inst_rdata,
  output logic [31:0] inst_addr,
  output logic inst_read,

  input logic data_resp,
  input logic [31:0] data_rdata,
  output logic data_read,
  output logic data_write,
  output logic [3:0] data_mbe,
  output logic [31:0] data_addr,
  output logic [31:0] data_wdata
);

logic [31:0] PC_reg_out;
logic stage_reg_load;
logic load_RAW_stall;

assign inst_addr = PC_reg_out;

assign inst_read = 1'b1;

stage_reg_t stage_reg_ID, stage_reg_EX, stage_reg_MEM, stage_reg_WB;

logic [31:0] regfile_in;
logic [4:0] rd;

//Stall pipeline when cache misses happen
always_comb begin
  stage_reg_load = 1'b1;
  if ((data_read || data_write)) begin
    if (~data_resp) stage_reg_load = 1'b0;
  end
  if (~inst_resp)
    stage_reg_load = 1'b0;
end

IF IF(
  .clk, .rst, .load_pc(stage_reg_load && !load_RAW_stall),
  .pcmux_sel(stage_reg_MEM.control_word.pcmux_sel),
  .PC_reg_out,
  .pc_plus_reg(stage_reg_MEM.pc_plus_reg),
  .alu_out(stage_reg_MEM.alu_out)
);

ID ID(
  .clk, .rst, .stage_reg_load(stage_reg_load && !load_RAW_stall),
  .PC_reg_out, .regfile_dest(rd),
  .regfile_in,
  .regfile_load(stage_reg_MEM.control_word.regfile_load),
  .instruction(inst_rdata),
  .stage_reg_out(stage_reg_ID)
);

EX EX(
  .clk, .rst, .stage_reg_load,
  .stage_reg_prev(stage_reg_ID),
  .stage_reg_out(stage_reg_EX),
  .mem_wb_stage_reg(stage_reg_MEM),
  .regfile_data(regfile_in),
  .load_RAW_stall
);

MEM MEM(
  .clk, .rst, .stage_reg_load,
  .stage_reg_in(stage_reg_EX),
  .stage_reg_out(stage_reg_MEM),
  .mem_read(data_read),
  .mem_write(data_write),
  .mem_rdata(data_rdata),
  .mem_wdata(data_wdata),
  .mem_addr(data_addr),
  .mem_byte_enable(data_mbe)
);

WB WB(
  .clk, .rst, .stage_reg_load,
  .stage_reg_in(stage_reg_MEM),
  .stage_reg_out(stage_reg_WB),
  .regfile_data(regfile_in),
  .rd
);

endmodule