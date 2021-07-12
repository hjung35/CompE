module comparator #(
    parameter width = 32
)

(
    input logic [width-1:0] a, b,
    output logic f
);

always_comb
begin : compare
    f = (a == b) ? 1'b1 : 1'b0;
end

endmodule : comparator