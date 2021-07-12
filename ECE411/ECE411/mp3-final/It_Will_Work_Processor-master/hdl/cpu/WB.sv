import rv32i_types::*;

module WB
(
    input logic clk, rst,
    input stage_reg_load,
    //cpu pipeline signals
    input stage_reg_t stage_reg_in,
    output stage_reg_t stage_reg_out,
    //wbmux output, sent to regfile in EX
    output rv32i_word regfile_data,
    output logic [4:0] rd
);

stage_reg WB_stage_reg(
    .load(stage_reg_load),
    .*
);

load_funct3_t load_funct3;
assign load_funct3 = load_funct3_t'(stage_reg_in.control_word.funct3);
rv32i_word mem_addr;
assign mem_addr = stage_reg_in.alu_out;

instruction_splitter isp(
    .instruction(stage_reg_in.instruction),
    .rd, .funct3(), .funct7(), .opcode(), .i_imm(),
    .s_imm(), .b_imm(), .u_imm(), .j_imm(), .rs1(), .rs2()
);

always_comb begin
    unique case (stage_reg_in.control_word.wbmux_sel)
        // regfilemux cases are not complete
        // use default of alu_out to avoid compile error for now
        wbmux::alu_out: regfile_data = stage_reg_in.alu_out;
        wbmux::br_en: regfile_data = {31'd0, stage_reg_in.br_en};
        wbmux::pc_plus4: regfile_data = stage_reg_in.pc_reg + 32'd4;
        wbmux::pc_plus_reg: regfile_data = stage_reg_in.pc_plus_reg;
        wbmux::mem_rdata: begin
            case (load_funct3)
            lb: begin
                case (mem_addr[1:0])
                    2'b00: regfile_data = {{24{stage_reg_in.mem_rdata[7]}}, stage_reg_in.mem_rdata[7:0]};
                    2'b01: regfile_data = {{24{stage_reg_in.mem_rdata[15]}}, stage_reg_in.mem_rdata[15:8]};
                    2'b10: regfile_data = {{24{stage_reg_in.mem_rdata[23]}}, stage_reg_in.mem_rdata[23:16]};
                    2'b11: regfile_data = {{24{stage_reg_in.mem_rdata[31]}}, stage_reg_in.mem_rdata[31:24]};
                endcase
            end
            lbu: begin
                case (mem_addr[1:0])
                    2'b00: regfile_data = {24'd0, stage_reg_in.mem_rdata[7:0]};
                    2'b01: regfile_data = {24'd0, stage_reg_in.mem_rdata[15:8]};
                    2'b10: regfile_data = {24'd0, stage_reg_in.mem_rdata[23:16]};
                    2'b11: regfile_data = {24'd0, stage_reg_in.mem_rdata[31:24]};
                endcase
            end
            lh: begin
                case (mem_addr[1:0])
                    2'b00: regfile_data = {{16{stage_reg_in.mem_rdata[15]}}, stage_reg_in.mem_rdata[15:0]};
                    2'b10: regfile_data = {{16{stage_reg_in.mem_rdata[31]}}, stage_reg_in.mem_rdata[31:16]};
                    default: regfile_data = {{16{stage_reg_in.mem_rdata[15]}}, stage_reg_in.mem_rdata[15:0]};
                endcase
            end
            lhu: begin
                case (mem_addr[1:0])
                    2'b00: regfile_data = {16'd0, stage_reg_in.mem_rdata[15:0]};
                    2'b10: regfile_data = {16'd0, stage_reg_in.mem_rdata[31:16]};
                    default:
                        regfile_data = {16'd0, stage_reg_in.mem_rdata[15:0]};
                endcase
            end
            lw: regfile_data = stage_reg_in.mem_rdata;
		      endcase
        end
	endcase
end


endmodule : WB


