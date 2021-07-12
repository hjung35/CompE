#include "scheduling.h"
// TODO: add check somewhere for writitng to not actual video memory for not focused tasks


/*
 *  Declares interrupt service routine for PIT
 */
DECLARE_ISR(pit_interrupt){
    task_t temp; // temporarily hold current task struct to pop then push

    // Save current context (regs, eip, ebp, esp) already pushed by isr...
    //task_queue[0].ebp = ebp;
    //task_queue[0].esp = esp;
    asm volatile (
        "movl %%ebp, %%eax      ;"
        "movl %%esp, %%ebx      ;"
        : "=a" (task_queue[0].ebp), "=b" (task_queue[0].esp)
        :
        : "memory"//"eax", "ebx"
    );
    task_queue[0].state = 0; // set to no longer running

    // If we haven't made a shell yet, execute one! (do this 3 times)
    static int con_idx = 0;
    if(cur_pcb == NULL || task_queue[2].pcb == NULL){
        send_eoi(PIT_INT_NUM); // Make sure we say the interrupt is done
        sti(); // We want interrupts to actually come in after this execute, they won't because the interrupt won't ret
        //if (!con_ovr.flag) {
            con_ovr.idx = con_idx;
            con_ovr.flag = true;
            con_idx++;
            execute("shell");
        //}
        goto end_of_interrupt; // Not needed? will the above execute be ok never getting to the end?
    }


    //push the current process to the end of the queue
    //pop, then push until we get a runnable task
    temp = task_queue[0];
    do
    {
        task_pop(&temp); // Should we error check?
        task_push(&temp);
        temp = task_queue[0];
    } while (temp.enabled == false);
    

    // Set the new process state to running (1?)
    task_queue[0].state = 1;

    //restore tss (we only change esp0?)
    tss.esp0 = ((uint32_t)task_queue[0].pcb) + KB_8 - STACK_OFF;

    // Switch paging structure, flush tlb. If it fails don't actually go to next task
    if(swap_task_paging(task_queue[0].pcb->pid) == -1){
        goto end_of_interrupt;
    }

    // Set the taskt to the current process
    cur_pcb = task_queue[0].pcb;
    // May not be needed because piazza post is saying by restoring next tasks esp, ebp then we can iret just like that
    // So for now we'll just restore esp and ebp
    asm volatile (
        "popl %%ecx             ;" // DOES NOTHING, REMOVE
        "movl %%eax, %%esp      ;"
        "movl %%ebx, %%ebp      ;"
        :
        : "eax" (task_queue[0].esp), "ebx" (task_queue[0].ebp) //task_queue[0].esp, task_queue[0].ebp <- 0 is placeholder for compilation
        : "ecx"  // We want ebp,esp to stay so do we not put that here right?? idk, using ecx to pop eip off from call within interrupt wrapper
    );

    // Signal that we are done with the interrupt
end_of_interrupt:
    send_eoi(PIT_INT_NUM);
}

/*
 * int32_t init_schedule
 *   DESCRIPTION: Initializes scheduling and empty task queue, ensuring IDT is registered correctly
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: TASK_SUCCESS on success
 *   SIDE EFFECTS: Enables PIT by setting command reg, temporarily disables IRQ for PIT interrupts,
 *   registers interrupt into IDT, initializes an empty task queue
 */
int32_t init_schedule(void){
    int i; // Loop Counter
    // Make sure we don't get PIT interrupts before we initilialize it
    disable_irq(PIT_INT_NUM);

    // Enable the pit by setting command reg, then setting data with rate of interrupts
    outb(PIT_CMD_DATA, PIT_CMD_REG);
    outb(PIT_DATA_REG, (PIT_CLOCK/QUANTUM_RATE) & 0xFF); //lsb first then msb explaining the bitmask and shifting
    outb(PIT_DATA_REG, (PIT_CLOCK/QUANTUM_RATE) >> 8);

    // Register in idt
    load_int(PIT_INT_NUM, &pit_interrupt, KERNEL_SEGMENT, INTERRUPT_DPL);

    //Initialize task queue (empty)
    for(i = 0; i < MAX_PROCESS; i++){
        task_queue[i].pcb = NULL; // All we need to signify it is empty
        task_queue[i].enabled = true;
    }

    // Enable the PIT irq line again
    enable_irq(PIT_INT_NUM);

    return TASK_SUCCESS;
}

/*
 * int32_t task_push
 *   DESCRIPTION: Helper function that pushes a specified task to the task queue
 *   INPUTS: current_task -- pointer to the current task struct
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure 
 *   SIDE EFFECTS: task gets pushed onto task queue
 */
int32_t task_push(task_t* current_task){
    int i;  // Loop counter

    // Make sure parameter is valid
    if(current_task == NULL){
        return -1;
    }

    // Place at the end of the queue
    // Assuming pcb == NULL if no task there
    for(i = 0; i < MAX_PROCESS; i++){
        if(task_queue[i].pcb == NULL){
            task_queue[i] = *current_task; // Assumining this current task has all members initialized
            break;
        }
    }

    // Return Success
    return 0;
}

/*
 * int32_t task_pop
 *   DESCRIPTION: helper funcion that pops a specified task struct out of the task_queue
 *   INPUTS: current_task -- pointer to the current task struct
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure 
 *   SIDE EFFECTS: Task gets pulled out of the task_queue after calling task_dequeue
 */
int32_t task_pop(task_t* current_task){
    int i;  // Loop counter

    // Make sure parameter is valid
    if(current_task == NULL){
        return -1;
    }

    // Check to see if the task is already in the queue
    for(i = 0; i < MAX_PROCESS; i++){
        // Remove the current task from the queue, then shift everything over
        if(task_queue[i].pcb == current_task->pcb){
            //task_queue[i].pcb = NULL;   // Assuming everything else will be overwritten when a new task is put into the queue
            for(i = i; i < MAX_PROCESS - 1; i++){
                task_queue[i] = task_queue[i+1]; // Shifts everything over one
            }
            task_queue[MAX_PROCESS - 1].pcb = NULL; // makes the last task in the queue empty
            return 0;
        }
    }

    // If we never found the task in the queue then we can't pop it
    return -1;
}

/*
 * int32_t task_grab
 *   DESCRIPTION: Grabs task from queue
 *   INPUTS: current_task -- pointer to the current task struct that will be filled
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Sets current_task's stack pointers to task struct with
 *   matching pcb and changes state to 1
 */
// No error checking because this has a very specific use, so invalid inputs won't happen
void task_grab(task_t* current_task){
    int i;

    for(i = 0; i < MAX_PROCESS; i++){
        if(task_queue[i].pcb == current_task->pcb){
            current_task->ebp = task_queue[i].ebp;
            current_task->esp = task_queue[i].esp;
            current_task->state = 1;
            return;
        }
    }
    current_task->pcb = NULL; // avoids needing a ret val
}
