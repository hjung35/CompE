`define BAD_MUX_SEL $fatal("%0t %s %0d: Illegal mux select", $time, `__FILE__, `__LINE__)

import rv32i_types::*;

module datapath
(
    input clk,
    input rst,
	input rv32i_word mem_rdata,
	 
	/* mux select signals are added */
	input pcmux::pcmux_sel_t pcmux_sel, 
	input marmux::marmux_sel_t marmux_sel,
	input regfilemux::regfilemux_sel_t regfilemux_sel,
	input alumux::alumux1_sel_t alumux1_sel,
	input alumux::alumux2_sel_t alumux2_sel,
	input cmpmux::cmpmux_sel_t cmpmux_sel, 
	 
	input alu_ops aluop,
	input branch_funct3_t cmpop,
	 
	 /* load signals are added */
	input load_pc,
	input load_ir,
	input load_regfile,
	input load_mar,
    input load_mdr,	 
	input load_data_out,
	 
	output logic br_en,	 
	output rv32i_opcode opcode,
	output logic [2:0] funct3,
	output logic [4:0] rs1,
    output logic [4:0] rs2,
	output logic [6:0] funct7,
	output logic [1:0] lsb_check,
	output rv32i_word mem_address,
	output rv32i_word mem_wdata // signal used by RVFI Monitor
);

/******************* Signals Needed for RVFI Monitor *************************/
rv32i_word pc_out;
rv32i_word pc_plus4_out;
rv32i_word pcmux_out;
rv32i_word marmux_out;
rv32i_word rs1_out;
rv32i_word rs2_out;
rv32i_reg rd;
rv32i_word mdrreg_out;
rv32i_word i_imm;
rv32i_word u_imm;
rv32i_word b_imm;
rv32i_word s_imm;
rv32i_word j_imm;
rv32i_word regfilemux_out;
rv32i_word alumux1_out;
rv32i_word alumux2_out;
rv32i_word alu_out;
rv32i_word cmpmux_out;
rv32i_word data_in;

rv32i_word data_temp;
/*****************************************************************************/
/***************************** Registers *************************************/
// Keep Instruction register named `IR` for RVFI Monitor
ir IR(
    .*,
    .load  (load_ir),
    .in    (mdrreg_out)
);

register MDR(
    .clk  (clk),
    .rst  (rst),
    .load (load_mdr),
    .in   (mem_rdata),
    .out  (mdrreg_out)
);

register mem_data_out(
    .clk  (clk),
	 .rst  (rst),
    .load (load_data_out),
    .in   (rs2_out),
    .out  (data_in)
);

register MAR(
    .clk  (clk),
	 .rst  (rst),
    .load (load_mar),
    .in   (marmux_out),
    .out  (mem_address)
);

pc_register PC(
    .clk  (clk),
	 .rst  (rst),
    .load (load_pc),
    .in   (pcmux_out),
    .out  (pc_out)
);

regfile regfile(
    .clk   (clk),
	.rst   (rst),
    .load  (load_regfile),
    .in    (regfilemux_out),
    .src_a (rs1),
    .src_b (rs2),
    .dest  (rd),
    .reg_a (rs1_out),
    .reg_b (rs2_out)
);

/*****************************************************************************/
assign data_temp = mem_address[1:0]


/******************************* ALU and CMP *********************************/
alu ALU(
    .aluop (aluop),
    .a     (alumux1_out),
    .b     (alumux2_out),
    .f     (alu_out)
);

cmp CMP( 
    .*, 
    .a (rs1_out), 
    .b(cmpmux_out)
);
/*****************************************************************************/
assign lsb_check = mem_address[1:0];

always_comb begin
    case (lsb_check)
        0: mem_wdata = data_in;
        1: mem_wdata = {data_in[23:0], 8'b0};
        2: mem_wdata = {data_in[15:0], 16'b0};
        3: mem_wdata = {data_in[7:0], 24'b0};
    endcase
end

/******************************** Muxes **************************************/
always_comb begin : MUXES
    // We provide one (incomplete) example of a mux instantiated using
    // a case statement.  Using enumerated types rather than bit vectors
    // provides compile time type safety.  Defensive programming is extremely
    // useful in SystemVerilog.  In this case, we actually use 
    // Offensive programming --- making simulation halt with a fatal message
    // warning when an unexpected mux select value occurs

    /* pcmux -- MUX2 */
    unique case (pcmux_sel)
        pcmux::pc_plus4: pcmux_out = pc_out + 4;
        pcmux::alu_out: pcmux_out = alu_out;
        pcmux::alu_mod2: pcmux_out = alu_out & 32'hfffffffe;
        default: `BAD_MUX_SEL;
    endcase

    /* marmux -- MUX2 */
    unique case (marmux_sel)
        marmux::pc_out: marmux_out = pc_out;
        marmux::alu_out: marmux_out = alu_out;
        default: `BAD_MUX_SEL;
    endcase

    /* regfilemux -- MUX4 */
    unique case (regfilemux_sel)
        regfilemux::alu_out: regfilemux_out = alu_out;
        regfilemux::br_en: regfilemux_out = {30'b0,br_en};
        regfilemux::u_imm: regfilemux_out = u_imm;
        regfilemux::lw: regfilemux_out = mdrreg_out;
        regfilemux::pc_plus4: regfilemux_out = pc_out + 4;
        regfilemux::lb: begin
            case (lsb_check)
                0: regfilemux_out = $signed(mdrreg_out[7:0]);
                1: regfilemux_out = $signed(mdrreg_out[15:8]);
                2: regfilemux_out = $signed(mdrreg_out[23:16]);
                3: regfilemux_out = $signed(mdrreg_out[31:24]);
            endcase
        end
        regfilemux::lbu: begin
            case (lsb_check)
                0: regfilemux_out = mdrreg_out[7:0];
                1: regfilemux_out = mdrreg_out[15:8];
                2: regfilemux_out = mdrreg_out[23:16];
                3: regfilemux_out = mdrreg_out[31:24];
            endcase
        end
        regfilemux::lh: begin
            regfilemux_out = (lsb_check) ? $signed(mdrreg_out[31:16]) : $signed(mdrreg_out[15:0]);
        end
        regfilemux::lhu: begin
            regfilemux_out = (lsb_check) ? mdrreg_out[31:16] : mdrreg_out[15:0];
        end
        default: `BAD_MUX_SEL;
    endcase

    /* alumux1 -- MUX2 */
    unique case (alumux1_sel)
        alumux::rs1_out: alumux1_out = rs1_out;
        alumux::pc_out: alumux1_out = pc_out;
        default: `BAD_MUX_SEL;
    endcase

    /* alumux2 -- MUX4 */
    unique case (alumux2_sel)
        alumux::i_imm: alumux2_out = i_imm;
        alumux::u_imm: alumux2_out = u_imm;
        alumux::b_imm: alumux2_out = b_imm;
        alumux::s_imm: alumux2_out = s_imm;
        alumux::j_imm: alumux2_out = j_imm;
        alumux::rs2_out: alumux2_out = rs2_out;
        default: `BAD_MUX_SEL;
    endcase

    unique case (cmpmux_sel)
        cmpmux::rs2_out: cmpmux_out = rs2_out;
        cmpmux::i_imm: cmpmux_out = i_imm;
        default: `BAD_MUX_SEL;
    endcase
end
/*****************************************************************************/
endmodule : datapath
