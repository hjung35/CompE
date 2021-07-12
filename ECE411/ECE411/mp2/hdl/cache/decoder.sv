module decoder (
    input logic [2:0] decoder_in,
    output logic [7:0] decoder_out   
);

always_comb begin
    decoder_out = 8'b0;
    unique case (decoder_in)
        0: decoder_out[0] = 1'b1;
        1: decoder_out[1] = 1'b1;
        2: decoder_out[2] = 1'b1;
        3: decoder_out[3] = 1'b1;
        4: decoder_out[4] = 1'b1;
        5: decoder_out[5] = 1'b1;
        6: decoder_out[6] = 1'b1;
        7: decoder_out[7] = 1'b1;
    endcase
end

endmodule : decoder
