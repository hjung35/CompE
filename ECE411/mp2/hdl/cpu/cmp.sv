import rv32i_types::*;

module cmp #(
	parameter width = 32
)
(
	input branch_funct3_t cmpop, 
	input logic [width-1:0] a, b,
	output logic br_en
);

always_comb begin : compare
	unique case (cmpop)
		beq:  br_en = (a == b) ? 1'b1 : 1'b0;
        bne:  br_en = (a != b) ? 1'b1 : 1'b0;
        blt:  br_en = ($signed(a) < $signed(b)) ? 1'b1 : 1'b0;
        bge:  br_en = ($signed(a) >= $signed(b)) ? 1'b1 : 1'b0;
        bltu: br_en = (a < b) ? 1'b1 : 1'b0;
        bgeu: br_en = (a >= b) ? 1'b1 : 1'b0;
	endcase 
end

endmodule : cmp
