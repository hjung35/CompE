`define BAD_MUX_SEL $fatal("%0t %s %0d: Illegal mux select", $time, `__FILE__, `__LINE__)

import rv32i_types::*;

module IF (
    input logic clk,
    input logic rst,
    input logic load_pc,
    input logic [31:0] pc_plus_reg,
    input pcmux::pcmux_sel_t pcmux_sel,
    input logic [31:0] alu_out,
	output rv32i_word PC_reg_out
);

rv32i_word pcmux_out;
rv32i_word pc_plus4;

pc_register PC_reg(
    .clk  (clk),
	.rst  (rst),
    .load (load_pc),
    .in   (pcmux_out),
    .out  (PC_reg_out)
);

always_comb begin : MUXES
    unique case (pcmux_sel)
        pcmux::pc_plus4: pcmux_out = PC_reg_out + 4;
        pcmux::alu_out: pcmux_out = pc_plus_reg;
        pcmux::alu_mod2: pcmux_out = alu_out & 32'hfffffffc;
        default: `BAD_MUX_SEL;
    endcase

end

endmodule : IF
