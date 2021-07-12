import cam_types::*;

module testbench(cam_itf itf);

cam dut (
    .clk_i     ( itf.clk     ),
    .reset_n_i ( itf.reset_n ),
    .rw_n_i    ( itf.rw_n    ),
    .valid_i   ( itf.valid_i ),
    .key_i     ( itf.key     ),
    .val_i     ( itf.val_i   ),
    .val_o     ( itf.val_o   ),
    .valid_o   ( itf.valid_o )
);

default clocking tb_clk @(negedge itf.clk); endclocking

task reset();
    itf.reset_n <= 1'b0;
    repeat (5) @(tb_clk);
    itf.reset_n <= 1'b1;
    repeat (5) @(tb_clk);
endtask

// DO NOT MODIFY CODE ABOVE THIS LINE

task write(input key_t key, input val_t val);
    ##(1);
    itf.key <= key;
    itf.val_i <= val;
    itf.rw_n <= 1'b0;
    itf.valid_i <= 1'b1;
endtask : write

task read(input key_t key, output val_t val);
    ##(1);
    itf.key <= key;
    itf.rw_n <= 1'b1;
    itf.valid_i <= 1'b1;
    ##(1);
    itf.valid_i <= 1'b0;
    val = itf.val_o;
endtask : read

/* LOCAL VARIABLES */
val_t current_value;
key_t [camsize_p-1:0] keys;
val_t [camsize_p-1:0] values;

initial begin
    $display("Starting CAM Tests");

    reset();
    /************************** Your Code Here ****************************/
    // Feel free to make helper tasks / functions, initial / always blocks, etc.
    // Consider using the task skeltons above
    // To report errors, call itf.tb_report_dut_error in cam/include/cam_itf.sv

    /* write, filling in CAM */
    for (int i = 0; i < camsize_p; i++) begin
        write(i, i);
        keys[i] = i;
        values[i] = i;
    end

    
    for (int i = 0; i < camsize_p; i++) begin
        write(camsize_p + i, camsize_p + i);
        keys[i] = camsize_p + i;
        values[i] = camsize_p + i;

        /* TEST: READ-HIT */
        /* TEST: CONSECUTIVE WRITE-READ ON THE SAME KEY */
        read(keys[i], current_value);

        /* READ ERROR REPORT */
        assert(current_value == values[i])
            else begin
                $error("%0t TB: Read %0d, expected %0d", $time, current_value, values[i]);
                itf.tb_report_dut_error(READ_ERROR);
            end
    end
    
    /**********************************************************************/

    itf.finish();
end

endmodule : testbench
