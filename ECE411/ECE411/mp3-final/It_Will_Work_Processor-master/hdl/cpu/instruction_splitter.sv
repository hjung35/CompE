import rv32i_types::*;

module instruction_splitter(
  input logic [31:0] instruction,
  output logic [2:0] funct3,
  output logic [6:0] funct7,
  output rv32i_opcode opcode,
  output logic [31:0] i_imm,
  output logic [31:0] s_imm,
  output logic [31:0] b_imm,
  output logic [31:0] u_imm,
  output logic [31:0] j_imm,
  output logic [4:0] rs1,
  output logic [4:0] rs2,
  output logic [4:0] rd
);

logic [31:0] data;
assign data = instruction;

always_comb begin
  funct3 = data[14:12];
  funct7 = data[31:25];
  opcode = rv32i_opcode'(data[6:0]);
  i_imm = {{21{data[31]}}, data[30:20]};
  s_imm = {{21{data[31]}}, data[30:25], data[11:7]};
  b_imm = {{20{data[31]}}, data[7], data[30:25], data[11:8], 1'b0};
  u_imm = {data[31:12], 12'h000};
  j_imm = {{12{data[31]}}, data[19:12], data[20], data[30:21], 1'b0};
  rs1 = data[19:15];
  rs2 = data[24:20];
  rd = data[11:7];

  case (opcode)
    op_lui: begin // Handles LUI without changing datapath
      rs1 = 5'b0;
      end
  default: ;
  endcase
end

endmodule