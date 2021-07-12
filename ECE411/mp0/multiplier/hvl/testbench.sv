import mult_types::*;

`ifndef testbench
`define testbench
module testbench(multiplier_itf.testbench itf);

add_shift_multiplier dut (
    .clk_i          ( itf.clk          ),
    .reset_n_i      ( itf.reset_n      ),
    .multiplicand_i ( itf.multiplicand ),
    .multiplier_i   ( itf.multiplier   ),
    .start_i        ( itf.start        ),
    .ready_o        ( itf.rdy          ),
    .product_o      ( itf.product      ),
    .done_o         ( itf.done         )
);

assign itf.mult_op = dut.ms.op;
default clocking tb_clk @(negedge itf.clk); endclocking

// DO NOT MODIFY CODE ABOVE THIS LINE

/* Uncomment to "monitor" changes to adder operational state over time */
//initial $monitor("dut-op: time: %0t op: %s", $time, dut.ms.op.name);


// Resets the multiplier
task reset();
    itf.reset_n <= 1'b0;
    ##5;
    itf.reset_n <= 1'b1;
    ##1;
endtask : reset

// error_e defined in package mult_types in file ../include/types.sv
// Asynchronously reports error in DUT to grading harness
function void report_error(error_e error);
    itf.tb_report_dut_error(error);
endfunction : report_error


initial itf.reset_n = 1'b0;
initial begin
    reset();
    /********************** Your Code Here *****************************/

    /* 4.4 2nd bullet of error reporting */
    assert(itf.rdy == 1'b1)
        else begin
            $error("%0d: %0t: NOT_READY error detected", `__LINE__, $time);
            report_error(NOT_READY);
        end

    for (int i = 0; i < 2 ** width_p; i++) begin
        for (int j = 0; j < 2 ** width_p; j++) begin   
            itf.start <= 1'b1;
            itf.multiplicand <= i;
            itf.multiplier <= j;
            ##1 $display("Multiplicand = %b, Multiplier = %b", itf.multiplicand, itf.multiplier);
            itf.start <= 1'b0;
        
            /* Upon entering 'done' state */
            @(tb_clk iff itf.done == 1'b1); 
            $display("Product : %d", itf.product);
            
            /* 4.4 1st bullet of error reporting */
            assert(itf.product == itf.multiplicand * itf.multiplier)
                else begin
                    $error("%0d: %0t: BAD_PRODUCT error detected", `__LINE__, $time);
                    report_error(BAD_PRODUCT);
                end

            /* 4.4 3rd bullet of error reporting */    
            assert(itf.rdy == 1'b1)
                else begin
                    $error("%0d: %0t: NOT_READY error detected", `__LINE__, $time);
                    report_error(NOT_READY);
                end
        end
    end
    
    /* ADD / SHIFT tests; 4 coverage tests*/
    /* reset in SHIFT cover */
    $display("TEST: Asserting reset in SHIFT");
    itf.start <= 1'b1;
    itf.multiplicand <= 7;
    itf.multiplier <= 20;
    ##1
    itf.start = 1'b0;
    @(tb_clk iff itf.mult_op == SHIFT) begin
        reset();
    end
    assert(itf.rdy == 1'b1)
        else begin
            $error("%0d: %0t: NOT_READY error detected", `__LINE__, $time);
            report_error(NOT_READY);
        end    

    /* reset in ADD cover */
    $display("TEST: Asserting reset in ADD");
    itf.start <= 1'b1;
    itf.multiplicand <= 7;
    itf.multiplier <= 20;
    ##1
    itf.start = 1'b0;
    @(tb_clk iff itf.mult_op == ADD) begin
        reset();
    end
    assert(itf.rdy == 1'b1)
        else begin
            $error("%0d: %0t: NOT_READY error detected", `__LINE__, $time);
            report_error(NOT_READY);
        end
    
    /* start in ADD / SHIFT cover */
    $display("TEST: Asserting start in ADD / SHIFT");
    itf.start <= 1'b1;
    itf.multiplicand <= 7;
    itf.multiplier <= 20;
    ##1
    itf.start <= 1'b0;

    /* start in ADD */ 
    @(tb_clk iff itf.mult_op == ADD) begin
        itf.start <= 1'b1;
        ##1;
        itf.start <= 1'b0;
    end

    /* start in SHIFT */ 
    @(tb_clk iff itf.mult_op == SHIFT) begin
        itf.start <= 1'b1;
        ##1;
        itf.start <= 1'b0;
    end

    /* Same as regular multiplication steps as previous codes written above */
    @(tb_clk iff itf.done == 1'b1);
    $display("Product : %d", itf.product);

    assert(itf.product == itf.multiplicand * itf.multiplier)
        else begin
            $error("%0d: %0t: BAD_PRODUCT error detected", `__LINE__, $time);
            report_error(BAD_PRODUCT);
        end
    assert(itf.rdy == 1'b1)
        else begin
            $error("%0d: %0t: NOT_READY error detected", `__LINE__, $time);
            report_error(NOT_READY);
        end

    /*******************************************************************/
    itf.finish(); // Use this finish task in order to let grading harness
                  // complete in process and/or scheduled operations
    $error("Improper Simulation Exit");
end


endmodule : testbench
`endif
