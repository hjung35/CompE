module cache_control (
    input clk,

    /* port to CPU */
    input logic mem_read,
                mem_write,
    output logic mem_resp,

    /* port to Physical memory */
    input pmem_resp,
    output logic pmem_read,
                 pmem_write,

    /* Internal signals */
    input logic         lru,
                        hit_0,
                        hit_1,
                        hit,
                        miss_0,
                        miss_1,
                        miss,

    input logic [1:0]   dirty_signal,                        

    output logic        read_data,
    output logic [1:0]  sel_dirty,
                        set_valid,
                        set_dirty,
                        flush_dirty,
                        load_tag,
                        load_data,
                        cache_read
);


enum int unsigned {
    IDLE, HIT_DETECTED, LOAD, STORE
} state, next_state;

function void set_defaults();
    mem_resp = 1'b0;
    pmem_read = 1'b0;
    pmem_write = 1'b0;
    read_data = 1'b0;
    sel_dirty = 2'b0;
    set_valid = 2'b0;
    set_dirty = 2'b0;
    flush_dirty = 2'b0;
    load_tag = 2'b0;
	load_data = 2'b0;
	cache_read = 2'b0;

	next_state = state;
endfunction

always_comb
begin
    set_defaults();
    case (state)

        /* IDLE STATE; initial state of the cache.
            Unless theres a read or write singal riased, it remains its IDLE state. 
            Otherwise, */ 
        IDLE: begin
            if (~(mem_read|mem_write))
                next_state = IDLE;

            else begin
                read_data = 1'b1;
                next_state = HIT_DETECTED;
            end
        end

        /* HIT_DETECTED STATE; 2nd state. determines when hit signal is detected. (hit == 1)
            Next, check the mem signal, whether mem_read or mem_write. 
            mem_read: sets cache_read signal based on hit_0 signal
            mem_write: sets load_data signal based on hit_0 signal. we count dirty bits here also. 
            Whole prcoess here is in the case of 'hit' signal (either hit_0 or hit_1 is raised) */ 
        HIT_DETECTED: begin
            if (hit == 1'b1) begin
                
                /* memory read */
                if (mem_read) begin
                    mem_resp = 1'b1;
                        case (hit_0)
                            0: cache_read[1] = 1'b1;
                            1: cache_read[0] = 1'b1;
                        endcase
                    next_state = IDLE;
                end
                
                /* memory write */
                if (mem_write) begin
                    mem_resp = 1'b1;
                    case (hit_0)
                        0: begin
                            set_dirty[1] = 1'b1;
                            load_data[1] = 1'b1;
                        end
                        1: begin
                            set_dirty[0] = 1'b1;
                            load_data[0] = 1'b1;
                        end
                    endcase
                    next_state = IDLE;
                end
            end 

            /* this is when hit is not detected, meaning both hit sginals are not raised.
                 when dirty bit isnt set, it shouldnt be replaced, but only follows on the lru bit.
                 Otherwise, we should replace it and flush_dirty bit has to be set. Thus, setting up on the physmem write signal.              
                  */
            else begin
                if (dirty_signal == 2'b00) begin
                    case (lru)
                        0: set_valid[0] = 1'b1;
                        1: set_valid[1] = 1'b1;
                    endcase
                    next_state = LOAD;
                end 

                else begin
                    case (lru)
                        0: flush_dirty[0] = 1'b1;
                        1: flush_dirty[1] = 1'b1;
                    endcase
                    pmem_write = 1'b1;
                    next_state = STORE;
                end
            end
        end

        /* LOAD STATE; 3rd state. simple load state. load state deals with reading the physical memory into the cache.
            pmem_resp is key signal here. wait until the pmem_read and pmem_resp is raised. when raised, load data and tag
            based on lru bit accordingly */ 
        LOAD: begin
            case (pmem_resp)
                0: begin
                    pmem_read = 1'b1;
                    next_state = LOAD;
                end

                1: begin
                    case (lru)
                        0: begin
                            load_data[0] = 1'b1;
                            load_tag[0] = 1'b1;
                        end
                        1: begin
                            load_data[1] = 1'b1;
                            load_tag[1] = 1'b1;
                        end
                    endcase
                    next_state = IDLE;
                end
            endcase
        end


        /* STORE STATE; 4th state. */ 
        STORE: begin
            case (pmem_resp)
                0: begin
                    pmem_write = 1'b1;
                    next_state = STORE;
                end

                1: begin
                    pmem_read = 1'b1;
                    next_state = LOAD;
                end
            endcase
        end

        /* Default state should be always in IDEL state */ 
        default: next_state = IDLE;
    endcase
end

always_ff @(posedge clk)
begin
    state <= next_state;
end

endmodule : cache_control
