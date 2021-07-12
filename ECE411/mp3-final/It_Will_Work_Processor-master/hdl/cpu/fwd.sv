import rv32i_types::*;

module forwarding_unit
(
  input stage_reg_t id_ex_stage_reg,
  input stage_reg_t ex_mem_stage_reg,
  input stage_reg_t mem_wb_stage_reg,
  input logic [31:0] regfile_data,
  output logic [31:0] rs1_out, rs2_out,
  output logic load_RAW_stall
);

logic [4:0] ex_rs1, ex_rs2, mem_rd, wb_rd;
rv32i_opcode ex_opcode;
instruction_splitter isp0(
  .instruction(id_ex_stage_reg.instruction),
  .rs1(ex_rs1),
  .rs2(ex_rs2),
  .opcode(ex_opcode)
);

instruction_splitter isp1(
  .instruction(ex_mem_stage_reg.instruction),
  .opcode(mem_opcode),
  .rd(mem_rd)
);

instruction_splitter isp2(
  .instruction(mem_wb_stage_reg.instruction),
  .opcode(wb_opcode),
  .rd(wb_rd)
);


always_comb begin
  // No forwarding default case
  rs1_out = id_ex_stage_reg.rs1_out;
  rs2_out = id_ex_stage_reg.rs2_out;
  load_RAW_stall = 1'b0;

  if (mem_rd == ex_rs1 && mem_rd != 0 && ex_mem_stage_reg.control_word.regfile_load) begin
    case (ex_mem_stage_reg.control_word.wbmux_sel)
      wbmux::alu_out: rs1_out = ex_mem_stage_reg.alu_out;
      wbmux::br_en: rs1_out = {31'd0, ex_mem_stage_reg.br_en};
      wbmux::pc_plus4: rs1_out = ex_mem_stage_reg.pc_reg + 32'd4;
      wbmux::pc_plus_reg: rs1_out = ex_mem_stage_reg.pc_plus_reg;
      wbmux::mem_rdata: load_RAW_stall = 1'b1;
	 endcase
  end
  else if (wb_rd == ex_rs1 && wb_rd != 0 && mem_wb_stage_reg.control_word.regfile_load) begin
    rs1_out = regfile_data;
  end

  if (mem_rd == ex_rs2 && mem_rd != 0 &&
      (ex_opcode == op_br || ex_opcode == op_reg ||ex_opcode == op_store) && 
      ex_mem_stage_reg.control_word.regfile_load) begin
    case (ex_mem_stage_reg.control_word.wbmux_sel)
      wbmux::alu_out: rs2_out = ex_mem_stage_reg.alu_out;
      wbmux::br_en: rs2_out = {31'd0, ex_mem_stage_reg.br_en};
      wbmux::pc_plus4: rs2_out = ex_mem_stage_reg.pc_reg + 32'd4;
      wbmux::pc_plus_reg: rs2_out = ex_mem_stage_reg.pc_plus_reg;
      wbmux::mem_rdata: load_RAW_stall = 1'b1;
    endcase
  end
  else if (wb_rd == ex_rs2 && wb_rd != 0 && mem_wb_stage_reg.control_word.regfile_load) begin
    rs2_out = regfile_data;
  end

end

endmodule