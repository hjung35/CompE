module cache_arbiter
(
    input clk, rst,

    //Icache Ports
    input logic inst_read,
    input logic [31:0] inst_addr,
    input logic [31:0] inst_byte_enable256,
    output logic inst_resp,
    output logic [255:0] inst_rdata, 

    //Dcache Ports
    input logic data_read, data_write,
    input logic [31:0] data_addr,
    input logic [255:0] data_wdata,
    input logic [31:0] data_byte_enable256,
    output logic data_resp,
    output logic [255:0] data_rdata,

    //Pmem Ports
    input logic mem_resp, 
    input logic [255:0] mem_rdata,
    output logic mem_read, mem_write, 
    output logic [255:0] mem_wdata,
    output logic [31:0] mem_addr,
    output logic [31:0] mem_byte_enable
    
);

enum int unsigned
{
    idle_s, inst_read_s, data_read_s, data_write_s, done_IR, done_DR, done_DW
} state, next_states;


always_ff @(posedge clk) begin
    if (rst)
        state <= idle_s;
    else
        state = next_states;
end

//State Transition
always_comb begin
    next_states = state;
    case(state)
        idle_s: begin
            if (inst_read)
                next_states = inst_read_s;
            else if (data_read)
                next_states = data_read_s;
            else if (data_write)
                next_states = data_write_s;
        end
        inst_read_s, data_read_s, data_write_s: begin
            if (mem_resp)
                next_states = idle_s;
        end
		endcase
end

//State Action
always_comb begin
    //Defaults
    inst_resp = 1'b0;
    data_resp = 1'b0;
    inst_rdata = 32'd0;
	data_rdata = 32'd0;
    mem_read = 1'b0;
    mem_write = 1'b0;
	mem_wdata = 32'd0;
    mem_addr = 32'd0;
    mem_byte_enable = 32'd0;
    case(state)
        idle_s: ;
        inst_read_s: begin
            mem_read = 1'b1;
            mem_addr = inst_addr;
            inst_rdata = mem_rdata;
            inst_resp = mem_resp;
            mem_byte_enable = inst_byte_enable256;

        end
        data_read_s: begin
            mem_read = 1'b1;
            mem_addr = data_addr;
            data_rdata = mem_rdata;
            data_resp = mem_resp;
            mem_byte_enable = data_byte_enable256;
        end
        data_write_s: begin
            mem_write = 1'b1;
            mem_addr = data_addr;
            mem_wdata = data_wdata;
            data_resp = mem_resp;
            mem_byte_enable = data_byte_enable256;

        end
    endcase
end


endmodule: cache_arbiter