import rv32i_types::*;

module EX
(
  input logic clk, rst, stage_reg_load,
  // Input from previous stage
  input stage_reg_t stage_reg_prev,
  input stage_reg_t mem_wb_stage_reg,
  input logic [31:0] regfile_data,
  output stage_reg_t stage_reg_out,
  output load_RAW_stall
);

stage_reg_t stage_reg_in;
rv32i_opcode opcode;
logic load_RAW_stall_fwd;
assign load_RAW_stall = load_RAW_stall_fwd;

logic [31:0] rs1_out, rs2_out;
forwarding_unit fwd(
  .id_ex_stage_reg(stage_reg_prev),
  .ex_mem_stage_reg(stage_reg_out),
  .mem_wb_stage_reg, .regfile_data,
  .rs1_out, .rs2_out,
  .load_RAW_stall(load_RAW_stall_fwd)
);

logic [31:0] i_imm, s_imm, b_imm, u_imm, j_imm;
instruction_splitter isp(
  .instruction(stage_reg_prev.instruction),
  .opcode, .funct3(), .funct7(), .rs1(), .rs2(), .rd(),
  .*
);

logic [31:0] alu_mux_out;
alu_ops alu_op;
always_comb begin: ALUMUX
  unique case (stage_reg_prev.control_word.alumux_sel)
    alumux::i_imm: alu_mux_out = i_imm;
    alumux::u_imm: alu_mux_out = u_imm;
    alumux::b_imm: alu_mux_out = b_imm;
    alumux::s_imm: alu_mux_out = s_imm;
    alumux::j_imm: alu_mux_out = j_imm;
    alumux::rs2_out: alu_mux_out = rs2_out;
  endcase
  alu_op = stage_reg_prev.control_word.aluop;
  // Special case of alu_op, SRAI and SRAR
  if (stage_reg_prev.control_word.funct3 == 3'b101 && stage_reg_prev.instruction[30])
    alu_op = alu_sra;
end

logic [31:0] alu_out;
alu ALU(
  .aluop(alu_op),
  .a(rs1_out),
  .b(alu_mux_out),
  .f(alu_out)
);

logic [31:0] cmp_mux_out;
always_comb begin: CMPMUX
  unique case (stage_reg_prev.control_word.cmpmux_sel)
    cmpmux::i_imm: cmp_mux_out = i_imm;
    cmpmux::rs2_out: cmp_mux_out = rs2_out;
  endcase
end

logic br_en;
cmp cmp(
  .cmpop(branch_funct3_t'(stage_reg_prev.control_word.funct3)),
  .a(rs1_out),
  .b(cmp_mux_out),
  .br_en
);

logic [31:0] pc_adder_mux_out;
always_comb begin: PC_ADDER_MUX
  case(stage_reg_prev.control_word.pcaddermux_sel)
    pcaddermux::i_imm: pc_adder_mux_out = i_imm;
    pcaddermux::j_imm: pc_adder_mux_out = j_imm;
    pcaddermux::u_imm: pc_adder_mux_out = u_imm;
    pcaddermux::b_imm: pc_adder_mux_out = b_imm;
  endcase
end


always_comb begin: SETTING_STAGE_REG_IN
  stage_reg_in = stage_reg_prev;
  stage_reg_in.alu_out = alu_out;
  stage_reg_in.br_en = br_en;
  stage_reg_in.pc_plus_reg = stage_reg_prev.pc_reg + pc_adder_mux_out;
  stage_reg_in.rs1_out = rs1_out;
  stage_reg_in.rs2_out = rs2_out;


  if (opcode == op_br) begin
    stage_reg_in.control_word.pcmux_sel = br_en ? pcmux::alu_out : pcmux::pc_plus4;
  end
end

logic [1:0] flush_counter, next_counter;
stage_reg_t stage_reg_next, stage_reg_flush;

stage_reg stage_reg(
  .clk, .rst, .load(stage_reg_load),
  .stage_reg_in(stage_reg_next), .stage_reg_out
);


always_comb begin: ADD_X0_INSTRUCTION
  stage_reg_flush.control_word.regfile_load = 1'b0;
  stage_reg_flush.control_word.aluop = alu_add;
  stage_reg_flush.control_word.funct3 = 3'b000;
  stage_reg_flush.control_word.alumux_sel = alumux::rs2_out;
  stage_reg_flush.control_word.pcmux_sel = pcmux::pc_plus4;
  stage_reg_flush.control_word.cmpmux_sel = cmpmux::rs2_out;
  stage_reg_flush.control_word.pcaddermux_sel = pcaddermux::u_imm;
  stage_reg_flush.control_word.mem_read = 1'b0;
  stage_reg_flush.control_word.mem_write = 1'b0;
  stage_reg_flush.control_word.wbmux_sel = wbmux::alu_out;
  stage_reg_flush.instruction = 32'h00000013;
  stage_reg_flush.br_en = 1'b0;
end

always_comb begin
  if (flush_counter == 2'b00 && !load_RAW_stall) stage_reg_next = stage_reg_in;
  else stage_reg_next = stage_reg_flush;
end

always_ff @(posedge clk) begin
  if (rst) flush_counter = 2'b00;
  else flush_counter = next_counter;
end

always_comb begin
  next_counter = flush_counter;
  if (stage_reg_load) begin
    case (flush_counter)
      2'b00: begin
        if (((opcode == op_br && br_en) || opcode == op_jal || opcode == op_jalr) && !load_RAW_stall_fwd ) next_counter = 2'b01;
      end
      2'b01: begin
        next_counter = 2'b10;
      end
      2'b10: begin
        next_counter = 2'b11;
      end
      2'b11: begin
        next_counter = 2'b00;
      end
    endcase
  end
  else next_counter = flush_counter;
end


endmodule