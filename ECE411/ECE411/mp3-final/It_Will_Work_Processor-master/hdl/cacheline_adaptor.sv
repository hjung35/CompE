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

enum int unsigned {
    idle,
    load_0,
    load_1,
    load_2,
    load_3,
    load_4,
    store,
    store_0,
    store_1,
    store_2,
    store_3,
    store_4
} state, next_state;

logic [255:0] temp, next_temp;

always_comb begin
    // Default Outputs
    next_temp = temp;
    next_state = state;
    line_o = temp;
    resp_o = 1'b0;
    burst_o = 64'b0;
    address_o = address_i;
    read_o = 1'b0;
    write_o = 1'b0;

    case (state)
        idle: ;
        load_0: begin
            next_temp = {temp[255:64], burst_i};
            read_o = 1'b1;
        end
        load_1: begin
            next_temp = {temp[255:128], burst_i, temp[63:0]};
            read_o = 1'b1;
        end
        load_2: begin
            next_temp = {temp[255:192], burst_i, temp[127:0]};
            read_o = 1'b1;
        end
        load_3: begin
            next_temp = {burst_i, temp[191:0]};
            read_o = 1'b1;
        end
        load_4: begin
            line_o = temp;
            resp_o = 1;
        end

        store_0: begin
            burst_o = line_i[63:0];
            write_o = 1'b1;
        end
        store_1: begin
            burst_o = line_i[127:64];
            write_o = 1'b1;
        end
        store_2: begin
            burst_o = line_i[191:128];
            write_o = 1'b1;
        end
        store_3: begin
            burst_o = line_i[255:192];
            write_o = 1'b1;
        end
        store_4: begin
            resp_o = 1'b1;
        end
        default: ;
    endcase

    unique case (state)
        idle:
            if (read_i) next_state = load_0;
            else if (write_i) next_state = store_0;
        load_0: if (resp_i) next_state = load_1;
        load_1: next_state = load_2;
        load_2: next_state = load_3;
        load_3: next_state = load_4;
        load_4: next_state = idle;

        store: if (resp_i) next_state = store_0;
        store_0: if (resp_i) next_state = store_1;
        store_1: next_state = store_2;
        store_2: next_state = store_3;
        store_3: next_state = store_4;
        store_4: next_state = idle;
        default: ;
    endcase
end

always_ff @(posedge clk) begin
    state = next_state;
    temp = next_temp;
end

endmodule : cacheline_adaptor
