package pcmux;
typedef enum bit [1:0] {
  pc_plus4 = 2'b00,
  alu_out = 2'b01,
  alu_mod2 = 2'b10
} pcmux_sel_t;
endpackage

package alumux;
typedef enum bit [2:0] {
    i_imm    = 3'b000,
    u_imm   = 3'b001,
    b_imm   = 3'b010,
    s_imm   = 3'b011,
    j_imm   = 3'b100,
    rs2_out = 3'b101
} alumux_sel_t;
endpackage

package cmpmux;
typedef enum bit {
  i_imm = 1'b0,
  rs2_out = 1'b1
} cmpmux_sel_t;
endpackage

package wbmux;
typedef enum bit [2:0] {
  alu_out = 3'b000,
  br_en = 3'b001,
  pc_plus4 = 3'b010,
  mem_rdata = 3'b011,
  pc_plus_reg = 3'b100
} wbmux_sel_t;
endpackage

package pcaddermux;
typedef enum bit [1:0] {
  i_imm = 2'b00,
  j_imm = 2'b01,
  u_imm = 2'b10,
  b_imm = 2'b11
} pcaddermux_sel_t;
endpackage