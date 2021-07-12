import rv32i_types::*;

module mp3_tb;
`timescale 1ns/10ps

`define RVFIMONITOR_ON 1


/********************* Do not touch for proper compilation *******************/
// Instantiate Interfaces
tb_itf itf();
rvfi_itf rvfi(itf.clk, itf.rst);

// Instantiate Testbench
source_tb tb(
    .magic_mem_itf(itf),
    .mem_itf(itf),
    .sm_itf(itf),
    .tb_itf(itf),
    .rvfi(rvfi)
);
/****************************** End do not touch *****************************/

/************************ Signals necessary for monitor **********************/
// This section not required until CP3

`ifdef RVFIMONITOR_ON
    if (`RVFIMONITOR_ON == 1) begin
        typedef struct packed {
            logic rvfi_valid;
            logic [63:0] rvfi_order;
            logic [31:0] rvfi_inst;
            logic rvfi_trap;
            logic rvfi_halt;
            logic [4:0] rvfi_rs1_addr;
            logic [4:0] rvfi_rs2_addr;
            logic [31:0] rvfi_rs1_rdata;
            logic [31:0] rvfi_rs2_rdata;
            logic load_regfile;
            logic [4:0] rvfi_rd_addr;
            logic [31:0] rvfi_rd_wdata;
            logic [31:0] rvfi_pc_rdata;
            logic [31:0] rvfi_pc_wdata;
            logic [31:0] rvfi_mem_addr;
            logic [3:0] rvfi_mem_rmask;
            logic [3:0] rvfi_mem_wmask;
            logic [31:0] rvfi_mem_rdata;
            logic [31:0] rvfi_mem_wdata;
        }RVFIMonPacket;

        RVFIMonPacket mon_pack_IF, mon_pack_ID, mon_pack_EX, mon_pack_MEM, mon_pack_WB;
        int order;
        int timeout = 100000000;

        initial begin
            // Make sure every instrcution is committed only when it's retired
            //mon_pack_IF.rvfi_valid = 1'b1;
            mon_pack_IF.rvfi_valid = 1'b0;
            mon_pack_ID.rvfi_valid = 1'b0;
            mon_pack_EX.rvfi_valid = 1'b0;
            mon_pack_MEM.rvfi_valid = 1'b0;
            mon_pack_WB.rvfi_valid = 1'b0;
            order = 0;
        end

        always @(posedge itf.clk) begin

            if (dut.cpu.stage_reg_load) begin
                // Default cases
                mon_pack_WB <= mon_pack_MEM;
                mon_pack_MEM <= mon_pack_EX;
                mon_pack_EX <= mon_pack_ID;
                mon_pack_ID <= mon_pack_IF;
                //Monitor's metadata
                mon_pack_WB.rvfi_valid <= (dut.cpu.WB.stage_reg_in.instruction != 0);
                mon_pack_IF.rvfi_order <= order;
                order <= order + 1;
                mon_pack_WB.rvfi_inst <= dut.cpu.WB.stage_reg_in.instruction;
                mon_pack_IF.rvfi_trap <= 1'b0;
                // 7'b1100011 is op_br
                mon_pack_EX.rvfi_halt <= (dut.cpu.EX.opcode == op_br && dut.cpu.EX.br_en && 
                                    (dut.cpu.EX.pc_adder_mux_out == 32'd0));
                //Monitor's regfile read/write
                mon_pack_WB.rvfi_rs1_addr <= dut.cpu.WB.isp.rs1;
                if (dut.cpu.WB.isp.opcode == op_br || dut.cpu.WB.isp.opcode == op_reg || dut.cpu.WB.isp.opcode == op_store)
                    mon_pack_WB.rvfi_rs2_addr <= dut.cpu.WB.isp.rs2;
                else
                     mon_pack_ID.rvfi_rs2_addr <= 5'd0;
                     
                mon_pack_EX.rvfi_rs1_rdata <= dut.cpu.EX.rs1_out;
                if (dut.cpu.EX.opcode == op_br || dut.cpu.EX.opcode == op_reg || dut.cpu.EX.opcode == op_store)
                    mon_pack_EX.rvfi_rs2_rdata <= dut.cpu.EX.rs2_out;
                else
                    mon_pack_EX.rvfi_rs2_rdata <= 32'd0;
                mon_pack_WB.load_regfile <= dut.cpu.WB.stage_reg_in.control_word.regfile_load;
                mon_pack_WB.rvfi_rd_addr <= dut.cpu.WB.rd;
                mon_pack_WB.rvfi_rd_wdata <= (dut.cpu.WB.rd ? dut.cpu.WB.regfile_data : 32'd0);
                //Monitor's PC
                mon_pack_WB.rvfi_pc_wdata <= dut.cpu.MEM.stage_reg_in.pc_reg;
                mon_pack_WB.rvfi_pc_rdata <= dut.cpu.WB.stage_reg_in.pc_reg;
                
                //Monitor's memory access
                mon_pack_MEM.rvfi_mem_rmask <= (dut.cpu.MEM.mem_read ? dut.cpu.MEM.rmask : 4'd0);
                mon_pack_MEM.rvfi_mem_wmask <= (dut.cpu.MEM.mem_write ? dut.cpu.MEM.wmask : 4'd0);
                mon_pack_MEM.rvfi_mem_wdata <= dut.cpu.MEM.mem_wdata;

                mon_pack_WB.rvfi_mem_addr <= dut.cpu.WB.mem_addr;
                mon_pack_WB.rvfi_mem_rdata <= dut.cpu.WB.stage_reg_in.mem_rdata;

    
            end
            else begin
                mon_pack_WB <= mon_pack_WB;
                mon_pack_MEM <= mon_pack_MEM;
                mon_pack_EX <= mon_pack_EX;
                mon_pack_ID <= mon_pack_ID;
                mon_pack_IF <= mon_pack_IF; 

                mon_pack_WB.rvfi_valid <= 1'b0;
            end   


        end

        assign rvfi.commit = mon_pack_WB.rvfi_valid;
        assign rvfi.order = mon_pack_WB.rvfi_order;
        assign rvfi.inst = mon_pack_WB.rvfi_inst;
        assign rvfi.trap = mon_pack_WB.rvfi_trap;
        assign rvfi.halt = mon_pack_WB.rvfi_halt;
        assign rvfi.rs1_addr = mon_pack_WB.rvfi_rs1_addr;
        assign rvfi.rs2_addr = mon_pack_WB.rvfi_rs2_addr;
        assign rvfi.rs1_rdata = mon_pack_WB.rvfi_rs1_rdata;
        assign rvfi.rs2_rdata = mon_pack_WB.rvfi_rs2_rdata;
        assign rvfi.load_regfile = mon_pack_WB.load_regfile;
        assign rvfi.rd_addr = mon_pack_WB.rvfi_rd_addr;
        assign rvfi.rd_wdata = mon_pack_WB.rvfi_rd_wdata;
        assign rvfi.pc_rdata = mon_pack_WB.rvfi_pc_rdata;
        assign rvfi.pc_wdata = mon_pack_WB.rvfi_pc_wdata;
        assign rvfi.mem_addr = mon_pack_WB.rvfi_mem_addr;
        assign rvfi.mem_rmask = mon_pack_WB.rvfi_mem_rmask;
        assign rvfi.mem_wmask = mon_pack_WB.rvfi_mem_wmask;
        assign rvfi.mem_rdata = mon_pack_WB.rvfi_mem_rdata;
        assign rvfi.mem_wdata = mon_pack_WB.rvfi_mem_wdata;      

    end
`endif
/**************************** End RVFIMON signals ****************************/

/********************* Assign Shadow Memory Signals Here *********************/
// This section not required until CP2
assign itf.inst_addr = dut.cpu.inst_addr;
assign itf.inst_rdata = dut.cpu.inst_rdata;
assign itf.inst_read = dut.cpu.inst_read;
assign itf.inst_resp = dut.cpu.inst_resp;

assign itf.data_addr = dut.cpu.data_addr;
assign itf.data_rdata = dut.cpu.data_rdata;
assign itf.data_wdata = dut.cpu.data_wdata;
assign itf.data_read = dut.cpu.data_read;
assign itf.data_write = dut.cpu.data_write;
assign itf.data_resp = dut.cpu.data_resp;
assign itf.data_mbe = dut.cpu.data_mbe;

/*********************** End Shadow Memory Assignments ***********************/

// Set this to the proper value
assign itf.registers = '{default: '0};

/*********************** Instantiate your design here ************************/
mp3 dut(
    .clk(itf.clk),
    .rst(itf.rst),
    .mem_resp(itf.mem_resp),
    .mem_rdata(itf.mem_rdata),
    .mem_read(itf.mem_read),
    .mem_write(itf.mem_write),
    .mem_addr(itf.mem_addr),
    .mem_wdata(itf.mem_wdata)
);
/***************************** End Instantiation *****************************/

endmodule