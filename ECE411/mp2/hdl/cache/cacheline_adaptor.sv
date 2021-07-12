module cacheline_adaptor
(
    input clk,
    input reset_n,

    // Port to LLC (Lowest Level Cache)
    input logic [255:0] line_i,
    output logic [255:0] line_o,
    input logic [31:0] address_i,
    input read_i,
    input write_i,
    output logic resp_o,

    // Port to memory
    input logic [63:0] burst_i,
    output logic [63:0] burst_o,
    output logic [31:0] address_o,
    output logic read_o,
    output logic write_o,
    input resp_i
);

enum int unsigned{
	INIT, LOAD_INIT, LOAD_1, LOAD_2, LOAD_3, LOAD_4, LOAD_DONE,	STORE_1, STORE_2, STORE_3, STORE_DONE
} state, next_states;


always @ (negedge clk) begin
    case(state)

		INIT: begin
		    if (read_i == 1'b0 && write_i == 1'b1 && reset_n == 1'b1) begin
				address_o = address_i;
				write_o = 1'b1;
				burst_o = line_i[63:0];
				next_states = STORE_1;
			end

		    else if (read_i == 1'b1 && write_i == 1'b0 && reset_n == 1'b1) begin
				address_o = address_i;		
				read_o = 1'b1;
				next_states = LOAD_1;
			end

		    else begin
				next_states = INIT;
			end
		end

		LOAD_INIT: begin 
			next_states = LOAD_1;
		end

		LOAD_1: begin

			if (resp_i == 1'b1 && read_i == 1'b0) begin

				line_o[63:0] = burst_i[63:0];
				next_states = LOAD_2;
			end

		end

		LOAD_2: begin
			if (resp_i == 1'b1 && read_i == 1'b0) begin
				line_o[127:64] = burst_i[63:0];
				next_states = LOAD_3;
			end

			
		end

		LOAD_3: begin
			if (resp_i == 1'b1 && read_i == 1'b0) begin
				line_o[191:128] = burst_i[63:0];

				next_states = LOAD_4;
			end
			
		end

		LOAD_4: begin
			if (resp_i == 1'b1 && read_i == 1'b0) begin
				line_o[255:192] = burst_i[63:0];
				resp_o = 1'b1;
				next_states = LOAD_DONE;

			end	

		end

		LOAD_DONE: begin
			resp_o = 1'b1;
			read_o = 1'b0;
			write_o = 1'b0;	
			next_states = INIT;	
		end


		STORE_1: begin
			if(resp_i == 1'b1 && write_i == 1'b0) begin	
				burst_o = line_i[127:64];
				next_states = STORE_2;

			end
		end

		STORE_2: begin
			if (resp_i == 1'b1  && write_i == 1'b0) begin	
				burst_o = line_i[191:128];
				next_states = STORE_3;
			end
		end

		STORE_3: begin	
			if (resp_i == 1'b1  && write_i == 1'b0)  begin
				burst_o = line_i[255:192];
				next_states = STORE_DONE;
			end
		end

		STORE_DONE: begin
			resp_o = 1'b1;
			read_o = 1'b0;
			write_o = 1'b0;
			next_states = INIT;
		end	
	endcase

end

always_ff @(posedge clk)
begin: next_state_assignment
    /* Assignment of next state on clock edge */
	 state <= next_states;
end

	
endmodule : cacheline_adaptor
