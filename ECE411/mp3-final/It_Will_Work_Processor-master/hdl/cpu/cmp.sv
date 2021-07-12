import rv32i_types::*;

module cmp
(
  input branch_funct3_t cmpop,
  input rv32i_word a, b,
  output logic br_en
);

always_comb begin : comparator_logic
    unique case (cmpop)
        beq: br_en = a == b;
        bne: br_en = a != b;
        blt: br_en = $signed(a) < $signed(b);
        bslt: br_en = $signed(a) < $signed(b);
        bge: br_en = $signed(a) >= $signed(b);
        bltu: br_en = a < b;
        bsltu: br_en = a < b;
        bgeu: br_en = a >= b;
    endcase 
end

endmodule : cmp