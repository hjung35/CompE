import rv32i_types::*; /* Import types defined in rv32i_types.sv */

module cpu_control
(
    input clk,
    input rst,
	 input logic mem_resp,
	 input logic br_en,
    input rv32i_opcode opcode,
    input logic [2:0] funct3,
    input logic [4:0] rs1,
    input logic [4:0] rs2,
    input logic [6:0] funct7,
	 input logic [1:0] lsb_check,
	 
	 output pcmux::pcmux_sel_t pcmux_sel,
    output marmux::marmux_sel_t marmux_sel,
    output regfilemux::regfilemux_sel_t regfilemux_sel,
    output alumux::alumux1_sel_t alumux1_sel,
    output alumux::alumux2_sel_t alumux2_sel,
    output cmpmux::cmpmux_sel_t cmpmux_sel,
    output alu_ops aluop,
	 output branch_funct3_t cmpop,
    output logic load_pc,
    output logic load_ir,
    output logic load_regfile,
    output logic load_mar,
    output logic load_mdr,
    output logic load_data_out,
    output logic mem_read,
	 output logic mem_write,
	 output logic [3:0] mem_byte_enable 
);

/***************** USED BY RVFIMON --- ONLY MODIFY WHEN TOLD *****************/
logic trap;
logic [4:0] rs1_addr, rs2_addr;
logic [3:0] rmask, wmask;

branch_funct3_t branch_funct3;
store_funct3_t store_funct3;
load_funct3_t load_funct3;
arith_funct3_t arith_funct3;

assign arith_funct3 = arith_funct3_t'(funct3);
assign branch_funct3 = branch_funct3_t'(funct3);
assign load_funct3 = load_funct3_t'(funct3);
assign store_funct3 = store_funct3_t'(funct3);
assign rs1_addr = rs1;
assign rs2_addr = rs2;

always_comb
begin : trap_check
    trap = 0;
    rmask = '0;
    wmask = '0;

    case (opcode)
        op_lui, op_auipc, op_imm, op_reg, op_jal, op_jalr:;

        op_br: begin
            case (branch_funct3)
                beq, bne, blt, bge, bltu, bgeu:;
                default: trap = 1;
            endcase
        end

        op_load: begin
            case (load_funct3)
                lw: rmask = 4'b1111;
                lh, lhu: rmask = 4'b0011 /* Modify for MP1 Final */ ;
                lb, lbu: rmask = 4'b0001 /* Modify for MP1 Final */ ;
                default: trap = 1;
            endcase
        end

        op_store: begin
            case (store_funct3)
                sw: wmask = 4'b1111;
                sh: wmask = 4'b0011 /* Modify for MP1 Final */ ;
                sb: wmask = 4'b0001 /* Modify for MP1 Final */ ;
                default: trap = 1;
            endcase
        end

        default: trap = 1;
    endcase
end
/*****************************************************************************/

enum int unsigned {
    /* List of states */
    fetch1, fetch2, fetch3, decode, slti, sltiu, srai, s_imm, br, lw_calc_addr, 
	 ldr1, ldr2, sw_calc_addr, str1, str2, s_auipc, s_lui, s_reg, s_jal, s_jalr 
} state, next_states;

/************************* Function Definitions *******************************/
/**
 *  You do not need to use these functions, but it can be nice to encapsulate
 *  behavior in such a way.  For example, if you use the `loadRegfile`
 *  function, then you only need to ensure that you set the load_regfile bit
 *  to 1'b1 in one place, rather than in many.
 *
 *  SystemVerilog functions must take zero "simulation time" (as opposed to 
 *  tasks).  Thus, they are generally synthesizable, and appropraite
 *  for design code.  Arguments to functions are, by default, input.  But
 *  may be passed as outputs, inouts, or by reference using the `ref` keyword.
**/

/**
 *  Rather than filling up an always_block with a whole bunch of default values,
 *  set the default values for controller output signals in this function,
 *   and then call it at the beginning of your always_comb block.
**/
function void set_defaults();
	 pcmux_sel = pcmux::pc_plus4;
    marmux_sel = marmux::pc_out;
    regfilemux_sel = regfilemux::alu_out;
    alumux1_sel = alumux::rs1_out;
    alumux2_sel = alumux::i_imm;
    cmpmux_sel = cmpmux::rs2_out;
	 aluop = alu_ops'(funct3);
	 cmpop = branch_funct3;
    load_pc = 1'b0;
    load_ir = 1'b0;
    load_regfile = 1'b0;
    load_mar = 1'b0;
    load_mdr = 1'b0;
    load_data_out = 1'b0;   
    mem_read = 1'b0;
    mem_write = 1'b0;
    mem_byte_enable = 4'b1111;
endfunction

/**
 *  Use the next several functions to set the signals needed to
 *  load various registers
**/
function void loadPC(pcmux::pcmux_sel_t sel = pcmux::pc_plus4);
    load_pc = 1'b1;
    pcmux_sel = sel;
endfunction

function void loadRegfile(regfilemux::regfilemux_sel_t sel = regfilemux::alu_out);
    load_regfile = 1'b1;
    regfilemux_sel = sel;
endfunction

function void loadMAR(marmux::marmux_sel_t sel = marmux::pc_out);
    load_mar = 1'b1;
    marmux_sel = sel;
endfunction

function void loadMDR();
    load_mdr = 1'b1;
    mem_read = 1'b1;
endfunction

/**
 * SystemVerilog allows for default argument values in a way similar to
 *   C++.
**/
function void setALU(alumux::alumux1_sel_t sel1,
                               alumux::alumux2_sel_t sel2,
                               logic setop = 1'b0, alu_ops op = alu_add);
    /* Student code here */
    alumux1_sel = sel1;
    alumux2_sel = sel2;

    if (setop)
        aluop = op; // else default value
endfunction

function automatic void setCMP(cmpmux::cmpmux_sel_t sel, branch_funct3_t op);
    cmpmux_sel = sel;
    cmpop = op;
endfunction

/*****************************************************************************/

    /* Remember to deal with rst signal */

always_comb
begin : state_actions
    /* Default output assignments */
    set_defaults();
    /* Actions for each state */
    unique case (state)
	 
        fetch1: loadMAR();

        fetch2: loadMDR();

        fetch3: load_ir = 1'b1;

        decode: ; // NONE

        slti: begin
            loadRegfile(regfilemux::br_en);
            loadPC(); // PC = PC + 4
            setCMP(cmpmux::i_imm, blt);
            setALU(alumux::rs1_out, alumux::i_imm);
        end

        sltiu: begin
            loadRegfile(regfilemux::br_en);
            loadPC();
            setCMP(cmpmux::i_imm, bltu);
            setALU(alumux::rs1_out, alumux::i_imm);
        end

        srai: begin
            loadRegfile();
            loadPC();
            setALU(alumux::rs1_out, alumux::i_imm, 1'b1, alu_sra);
        end

        s_imm: begin
            loadRegfile();
            loadPC();
            setALU(alumux::rs1_out, alumux::i_imm, 1'b1, alu_ops'(funct3));
        end

        br: begin
            if (br_en) 
                loadPC(pcmux::alu_out);
            else 
                loadPC(pcmux::pc_plus4);
            setALU(alumux::pc_out, alumux::b_imm, 1'b1);
        end

        lw_calc_addr: begin
            setALU(alumux::rs1_out, alumux::i_imm, 1'b1);
            loadMAR(marmux::alu_out);
        end

        ldr1: begin
            loadMDR();
        end

        ldr2: begin
            case(load_funct3)
                lw: loadRegfile(regfilemux::lw);
                lh: loadRegfile(regfilemux::lh);
                lhu: loadRegfile(regfilemux::lhu);
                lb:  loadRegfile(regfilemux::lb);
                lbu: loadRegfile(regfilemux::lbu);
            endcase    
            loadPC();
        end

        sw_calc_addr: begin
            setALU(alumux::rs1_out, alumux::s_imm, 1'b1);
            loadMAR(marmux::alu_out);
            load_data_out = 1'b1;
        end

        str1: begin
            mem_write = 1'b1;
            case (store_funct3)
                sb: begin
                    case (lsb_check)
                        0: mem_byte_enable = 4'b0001;
                        1: mem_byte_enable = 4'b0010;
                        2: mem_byte_enable = 4'b0100;
                        3: mem_byte_enable = 4'b1000;
                    endcase
                end

                sh: begin
                    case (lsb_check)
                        2: mem_byte_enable = 4'b1100;
                        default: mem_byte_enable = 4'b0011;
                    endcase
                end

                sw: mem_byte_enable = 4'b1111;
            endcase
        end

        str2: begin
            loadPC();
        end

        s_auipc: begin
            setALU(alumux::pc_out, alumux::u_imm, 1'b1);
            loadRegfile();
            loadPC();
        end

        s_lui: begin
            loadRegfile(regfilemux::u_imm);
            loadPC();
        end

        s_reg: begin
            setALU(alumux::rs1_out, alumux::rs2_out, 1'b1, alu_ops'(funct3));
            loadRegfile();
            loadPC();
        end

        s_jal: begin
            setALU(alumux::pc_out, alumux::j_imm, 1'b1);		  
            loadRegfile(regfilemux::pc_plus4);
            loadPC(pcmux::alu_out);
        end
		  
        s_jalr: begin
            setALU(alumux::rs1_out, alumux::i_imm, 1'b1);		  
            loadRegfile(regfilemux::pc_plus4);
            loadPC(pcmux::alu_mod2);
        end

        default: ;
    endcase
end

always_comb
begin : next_state_logic
    /* Next state information and conditions (if any)
     * for transitioning between states */
    case (state)
        fetch1: next_states = fetch2;

        fetch2: begin
            next_states = (mem_resp == 0) ? fetch2 : fetch3;
        end

        fetch3: next_states = decode;

        decode: begin
            unique case (opcode)
                op_imm: begin
                    case (arith_funct3)
                        add, sll, axor, aor, aand: next_states = s_imm;
                        slt: next_states = slti;
                        sltu: next_states = sltiu;
                        sr: next_states = (funct7[5] == 1'b1) ? srai : s_imm;
                    endcase
                end
					 
                op_br: next_states = br;
                op_load: next_states = lw_calc_addr;
                op_store: next_states = sw_calc_addr;					 
                op_auipc: next_states = s_auipc;
                op_lui: next_states = s_lui;
                op_reg: next_states = s_reg;
                op_jal: next_states = s_jal;
                op_jalr: next_states = s_jalr;
					 
                default: next_states = fetch1;
            endcase
        end

        lw_calc_addr: next_states = ldr1;
        ldr1: next_states = (mem_resp == 0) ? ldr1 : ldr2;
		  
        sw_calc_addr: next_states = str1;
        str1: next_states = (mem_resp == 0) ? str1 : str2;
        default: next_states = fetch1;
    endcase
end

always_ff @(posedge clk)
begin: next_state_assignment
    /* Assignment of next state on clock edge */
    state <= next_states;
end

endmodule : cpu_control
