import rv32i_types::*;

module ID
(
  input logic clk, rst, stage_reg_load,
  // Input from IF stage
  input logic [31:0] PC_reg_out,
  // Input from WB Stage Register
  input logic [4:0] regfile_dest,
  input logic [31:0] regfile_in,
  input logic regfile_load,
  // Other inputs
  input logic [31:0] instruction,
  // Stage register outputs
  output stage_reg_t stage_reg_out
);

logic [2:0] funct3;
logic [6:0] funct7;
rv32i_opcode opcode;
logic [31:0] i_imm, s_imm, b_imm, u_imm, j_imm;
logic [4:0] rs1, rs2, rd;

instruction_splitter isp(.*);

logic [31:0] rs1_out, rs2_out;
regfile regfile(
  .clk, .rst, .load(regfile_load),
  .in(regfile_in), .dest(regfile_dest),
  .src_a(rs1), .src_b(rs2),
  .reg_a(rs1_out), .reg_b(rs2_out)
);

rv32i_control_word ctrl;

control_ROM ctrl_ROM(
  .opcode,
  .funct3,
  .ctrl
);

// Update stage register with new values in current stage
stage_reg_t stage_reg_in;
always_comb begin
  stage_reg_in.pc_reg = PC_reg_out;
  stage_reg_in.control_word = ctrl;
  stage_reg_in.rs1_out = rs1_out;
  stage_reg_in.rs2_out = rs2_out;
  stage_reg_in.instruction = instruction;
end

stage_reg stage_reg(
  .clk, .rst, .load(stage_reg_load),
  .stage_reg_in, .stage_reg_out 
);

endmodule