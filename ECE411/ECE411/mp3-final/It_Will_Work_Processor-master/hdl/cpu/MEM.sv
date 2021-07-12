import rv32i_types::*;

module MEM
(
    input logic clk, rst,
    input stage_reg_load,
    //cpu pipeline signals
    input stage_reg_t stage_reg_in,
    output stage_reg_t stage_reg_out,
    //memory signals
    //note that mem_rdata is directed directly to wb stage
    input rv32i_word mem_rdata,
    output logic mem_read,
    output logic mem_write,
    output rv32i_word mem_wdata,
    output rv32i_word mem_addr,
    output logic [3:0] mem_byte_enable
);

stage_reg_t prev_stage;

stage_reg MEM_stage_reg(
    .load(stage_reg_load),
    .stage_reg_in(prev_stage),
    .*
);

store_funct3_t store_funct3;
assign store_funct3 = store_funct3_t'(stage_reg_in.control_word.funct3);
load_funct3_t load_funct3;
assign load_funct3 = load_funct3_t'(stage_reg_in.control_word.funct3);
rv32i_opcode opcode;
assign opcode = rv32i_opcode'(stage_reg_in.instruction[6:0]);
assign mem_read = stage_reg_in.control_word.mem_read;
assign mem_write = stage_reg_in.control_word.mem_write;
assign mem_addr = stage_reg_in.alu_out;


logic [3:0] wmask, rmask;

always_comb begin
//Calculate write mask
    wmask = 4'b0;
    rmask = 4'b0;
    case (store_funct3)
        sw: wmask = 4'b1111;
        sh: begin
            case (mem_addr[1:0])
                2'b00: wmask = 4'b0011;
                2'b10: wmask = 4'b1100;
            endcase
        end
        sb: begin
            case (mem_addr[1:0])
                2'b00: wmask = 4'b0001;
                2'b01: wmask = 4'b0010;
                2'b10: wmask = 4'b0100;
                2'b11: wmask = 4'b1000;
            endcase
        end
    endcase

//Calculate read mask
    case (load_funct3)
        lw: rmask = 4'b1111;
        lh, lhu: begin
            case (mem_addr[1:0])
                2'b00: rmask = 4'b0011;
                2'b10: rmask = 4'b1100;
            endcase
        end
        lb, lbu: begin
            case (mem_addr[1:0])
                2'b00: rmask = 4'b0001;
                2'b01: rmask = 4'b0010;
                2'b10: rmask = 4'b0100;
                2'b11: rmask = 4'b1000;
            endcase
        end
    endcase

    case(opcode)
        op_load: mem_byte_enable = rmask;
        op_store: mem_byte_enable = wmask;
        default: mem_byte_enable = 4'd0;
    endcase
end

always_comb begin
//Shift the rs2_out accordingly for different store operations
	prev_stage = stage_reg_in;
    mem_wdata = stage_reg_in.rs2_out;
    case (store_funct3)
        sh: begin
            case (mem_addr[1:0])
                2'b00: mem_wdata = {16'd0, stage_reg_in.rs2_out[15:0]};
                2'b10: mem_wdata = {stage_reg_in.rs2_out[15:0], 16'd0};
            endcase
        end
        sb: begin
            case (mem_addr[1:0])
                2'b00: mem_wdata = {24'd0, stage_reg_in.rs2_out[7:0]};
                2'b01: mem_wdata = {16'd0, stage_reg_in.rs2_out[7:0], 8'd0};
                2'b10: mem_wdata = {8'd0, stage_reg_in.rs2_out[7:0], 16'd0};
                2'b11: mem_wdata = {stage_reg_in.rs2_out[7:0], 24'd0};
            endcase
        end
    endcase

// fetch rdata from magic memory
    prev_stage.mem_rdata = mem_rdata;
end

endmodule : MEM