`ifndef testbench
`define testbench

import fifo_types::*;

module testbench(fifo_itf itf);

fifo_synch_1r1w dut (
    .clk_i     ( itf.clk     ),
    .reset_n_i ( itf.reset_n ),

    // valid-ready enqueue protocol
    .data_i    ( itf.data_i  ),
    .valid_i   ( itf.valid_i ),
    .ready_o   ( itf.rdy     ),

    // valid-yumi deqeueue protocol
    .valid_o   ( itf.valid_o ),
    .data_o    ( itf.data_o  ),
    .yumi_i    ( itf.yumi    )
);

// Clock Synchronizer for Student Use
default clocking tb_clk @(negedge itf.clk); endclocking

task reset();
    itf.reset_n <= 1'b0;
    ##(10);
    itf.reset_n <= 1'b1;
    ##(1);
endtask : reset

function automatic void report_error(error_e err); 
    itf.tb_report_dut_error(err);
endfunction : report_error

// DO NOT MODIFY CODE ABOVE THIS LINE

int current_data;

/* task enqueue */
task enqueue(word_t data);
    ##(1);
    itf.data_i <= data;
    itf.valid_i <= 1'b1;
    ##(1) $display("Enqueued data: %d", itf.data_i);
    itf.valid_i <= 1'b0;
    ##(1);
endtask : enqueue

/* task dequeue */
task dequeue();
    ##(1);
    itf.yumi <= 1'b1;
    ##(1) $display("Dequeued data: %d", itf.data_o);
    itf.yumi <= 1'b0;
    ##(1);
endtask : dequeue

/* enqueue and dequeue simnultaenously */
task enDequeue(word_t data);
    ##(1);
    itf.data_i <= data;
    itf.valid_i <= 1'b1;
    itf.yumi <= 1'b1;
    ##(1) $display("Simultaneously Enqueue and Dequeue the data. Enqueued_data: %d, Dequeued_data: %d", itf.data_i, itf.data_o);
    itf.valid_i <= 1'b0;
    itf.yumi <= 1'b0;
    ##(1);
endtask : enDequeue

/* data validity assertion, if data isnt what we expected, data assertion error should pop up */
task data_validity_assertion(word_t data);
    assert(itf.data_o == data)
        else begin
            $error("%0d: %0t: INCORRECT_DATA_O_ON_YUMI_I error detected", `__LINE__, $time);
            $error("Expected Data(Supposed to be): %d, Dequeued Data(Actual output): %d", data, itf.data_o);
            report_error (INCORRECT_DATA_O_ON_YUMI_I);
        end
endtask : data_validity_assertion

initial begin
    reset();
    /************************ Your Code Here ***********************/
    // Feel free to make helper tasks / functions, initial / always blocks, etc.

    @(posedge itf.clk) begin
        assert(itf.rdy == 1'b1)
            else begin
                $error("%0d: %0t: RESET_DOES_NOT_CAUSE_READY_O error detected", `__LINE__, $time);
                report_error (RESET_DOES_NOT_CAUSE_READY_O);
            end
    end

    /* signal initialization */
    itf.valid_i <= 1'b0;
    itf.yumi <= 1'b0;

    /* Enqueue words */
    for (int i = 0; i < cap_p; i++) begin
        enqueue(i);
    end

    current_data <= 0;
    /* data_validity_assertion is needed to make sure we dequeue something that was actually in the fifo queue */
    data_validity_assertion(current_data);
    dequeue();
    current_data = current_data + 1;

    for (int i = 0; i < cap_p-1; i++) begin
        data_validity_assertion(current_data);
        enDequeue(i);
        current_data = current_data + 1;
        data_validity_assertion(current_data);
        dequeue();
        current_data = current_data + 1;
        if (current_data == cap_p) 
            current_data = current_data - cap_p;
    end

    data_validity_assertion(current_data);
    dequeue();

    /***************************************************************/
    // Make sure your test bench exits by calling itf.finish();
    itf.finish();
    $error("TB: Illegal Exit ocurred");
end

endmodule : testbench
`endif

