import rv32i_types::*;

module control_ROM(
  input rv32i_opcode opcode,
  input logic [2:0] funct3,
  output rv32i_control_word ctrl
);

always_comb begin
  ctrl.regfile_load = 1'b0;
  ctrl.funct3 = funct3;
  ctrl.pcaddermux_sel = pcaddermux::u_imm;
  ctrl.alumux_sel = alumux::rs2_out;
  ctrl.pcmux_sel = pcmux::pc_plus4;
  ctrl.cmpmux_sel = cmpmux::rs2_out;
  ctrl.wbmux_sel = wbmux::alu_out;
  ctrl.aluop = alu_add;
  ctrl.mem_read = 1'b0;
  ctrl.mem_write = 1'b0;

  case (opcode)
    op_lui: begin
      ctrl.regfile_load = 1'b1;
      ctrl.alumux_sel = alumux::u_imm;
    end
    op_auipc: begin
      ctrl.regfile_load = 1'b1;
      ctrl.pcaddermux_sel = pcaddermux::u_imm;
      ctrl.wbmux_sel = wbmux::pc_plus_reg;
    end
    op_jal: begin
      ctrl.regfile_load = 1'b1;
      ctrl.pcaddermux_sel = pcaddermux::j_imm;
      ctrl.pcmux_sel = pcmux::alu_out;
      ctrl.wbmux_sel = wbmux::pc_plus4;
    end
    op_jalr: begin
      ctrl.regfile_load = 1'b1;
      ctrl.alumux_sel = alumux::i_imm;
      ctrl.pcmux_sel = pcmux::alu_mod2;
      ctrl.wbmux_sel = wbmux::pc_plus4;
    end
    op_br: begin
      ctrl.pcaddermux_sel = pcaddermux::b_imm;
      ctrl.pcmux_sel = pcmux::pc_plus4;
      ctrl.cmpmux_sel = cmpmux::rs2_out;
    end
    op_load: begin
      ctrl.regfile_load = 1'b1;
      ctrl.alumux_sel = alumux::i_imm;
      ctrl.wbmux_sel = wbmux::mem_rdata;
      ctrl.mem_read = 1'b1;
    end
    op_store: begin
      ctrl.alumux_sel = alumux::s_imm;
      ctrl.mem_write = 1'b1;
    end
    op_imm: begin
      ctrl.regfile_load = 1'b1;
      ctrl.aluop = alu_ops'(funct3);
      ctrl.alumux_sel = alumux::i_imm;
      ctrl.wbmux_sel = wbmux::alu_out;
      if (arith_funct3_t'(funct3) == slt || arith_funct3_t'(funct3) == sltu) begin
        ctrl.wbmux_sel = wbmux::br_en;
        ctrl.cmpmux_sel = cmpmux::i_imm;
      end
    end
    op_reg: begin
      ctrl.regfile_load = 1'b1;
      ctrl.aluop = alu_ops'(funct3);
      ctrl.alumux_sel = alumux::rs2_out;
      ctrl.wbmux_sel = wbmux::alu_out;
      if (arith_funct3_t'(funct3) == slt || arith_funct3_t'(funct3) == sltu)
        ctrl.wbmux_sel = wbmux::br_en;
    end
  endcase
end

endmodule