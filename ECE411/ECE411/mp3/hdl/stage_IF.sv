`define BAD_MUX_SEL $fatal("%0t %s %0d: Illegal mux select", $time, `__FILE__, `__LINE__)

import rv32i_types::*;

module stage_IF (
	
    input clk,
    input rst,
    input load_pc
    input pcmux::pcmux_sel_t pcmux_sel, 

    /* actual output of PC register which will go back to small adder register for pc_plus4 or maybe stay as it is  */
	output rv32i_word PC_reg_out	
);

rv32i_word pcmux_out;
rv32i_word PC_reg_out;
rv32i_word pc_plus4;

/* PC register */
pc_register PC_reg(
    .clk  (clk),
	.rst  (rst),
    .load (load_pc),
    .in   (pcmux_out),
    .out  (PC_reg_out)
);

always_comb begin : MUXES
    
    /* pcmux */
    unique case (pcmux_sel)
        pcmux::pc_plus4: pcmux_out = PC_reg_out + 4;
        pcmux::alu_out: pcmux_out = alu_out;
        pcmux::alu_mod2: pcmux_out = alu_out & 32'hfffffffe;
        default: `BAD_MUX_SEL;
    endcase

end

endmodule : stage_IF
