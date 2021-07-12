#include "types.h"
#include "paging.h"


/*
 * init_paging
 *   DESCRIPTION:   Sets up the first page directory, page table, pages for the first 8 MB of memory, updates the control registers
 *                  accordingly to enable paging.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: modifies, memory in kernel, CR0, CR3, CR4. Enables paging, sets first 4 MB to be 4KB pages, sets 4MB-8MB to be jumbo page for kernel
 */ 

void init_paging(){
    int i;                   /* Loop counter */
    int j;                   /* Outer loop counter for process table */
    unsigned int addr;       /* temp address for table indexing */
    unsigned int pd_addr;    /* Base address of the page directory to be loaded into cr3 */

    asm volatile (
        "mov %%cr4, %%eax                           ;"
        "orl $0x010, %%eax /* Sets PSE Flag */      ;"
        "mov %%eax, %%cr4                           ;"
        : 
        : 
        : "eax"                               
    );


    /* Populate Page table for 0-4MB*/
    vmem_table.pte[0] = 0x0; // Let's make the first 4KB not readable or writeable so then no one sneaks in and makes NULL valid
    for(i = 1; i < TBL_SIZE; i++){
        
        addr = i * 4 * TBL_SIZE; // 1024*4 = 4KB aligned :)
        /* If the page is not for video memory*/
        if(addr != VIDEO_LOC){
            addr &= ADDR_MASK;
            vmem_table.pte[i] = addr + OFF_PG_BITS;
        }
        /* If it is */
        else{
            addr &= ADDR_MASK;
            vmem_table.pte[i] = addr + VMEMPT_BITS;
            vmem_table2.pte[i] = addr + OFF_PG_BITS;
        }

    }
    /* Make all page directories for processes */
    for(j = 0; j < MAX_PROCESS; j++){
        for(i = 0; i < TBL_SIZE; i++){
            if(i == 0){
                addr = (unsigned int) &(vmem_table);
                addr &= ADDR_MASK;
                process_pdir_table[j].pde[i] = addr + VMEMPD_BITS;
            }
            else if(i == 1){
                addr = KENREL_LOC;
                addr &= ADDR_MASK;
                process_pdir_table[j].pde[i] = addr + KERNEL_BITS; 
            }
            else if (i == 30){
                addr = (unsigned int) &(vmem_table2);
                addr &= ADDR_MASK;
                process_pdir_table[j].pde[i] = addr + VMEMPD_BITS;
            }
            else if(i == USER_IDX){
                addr = MB_8 + (j * 0x400000); //0x400000 -> 4MB because that is the size of a jumbo page, each user process starts at 8mb and after in physcial mem
                addr &= ADDR_MASK;
                process_pdir_table[j].pde[i] = addr + USER_BITS;
            }
            else
                process_pdir_table[j].pde[i] = 0;
        }
    }
    
    /* Populate Page directory (for original kernel/cp1) */
    addr = (unsigned int) &(vmem_table);
    addr &= ADDR_MASK;
    page_dir.pde[0] = addr + VMEMPD_BITS;
    addr = KENREL_LOC;
    page_dir.pde[1] = addr + KERNEL_BITS;
    for(i = 2; i < DIR_SIZE; i++){
        page_dir.pde[i] = 0x0;
    }

    /* Load base address of pd into pdbr (cr3) */
    pd_addr = (unsigned int) page_dir.pde;
    pd_addr &= ADDR_MASK;
    asm volatile (
        "mov %%cr3, %%eax                       ;"
        "andl $0x00000FFF, %%eax                 ;"
        "orl %%edx, %%eax   /* Sets CR3 PDBR */ ;"
        "mov %%eax, %%cr3                       ;"
        : 
        : "d" (pd_addr) 
        : "eax"                 
    );

    /* Turn paging on */
    asm volatile (
        "mov %%cr0, %%eax                                               ;"
        "orl $0x80000000, %%eax /* Sets PG Flag to enable paging */     ;"
        "mov %%eax, %%cr0                                               ;"
        : 
        : 
        : "eax"                                    
    );  
}
/*
 * new_process_ptable
 *   DESCRIPTION:   loop through our Page directory tables to find an open table for a new process. 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: Return -1 for failure to make page table for new process, otherwise return the process number
 *   SIDE EFFECTS: reload CR3 to flush the TLB.
 */ 

int new_process_ptable(){
    int i;              /* Loop Counter */
    uint32_t pd_addr;   /* Address of the page directory (for current process) being loaded into cr3 */

    /* Loop through our process page directory table to find an open table (for a new process) */
    /* Yes this can be done without having multiple pd's but this works so we'll leave it for now, functionality first */
    for(i = 0; i < MAX_PROCESS; i ++){
        if(!(process_pdir_table[i].pde[USER_IDX] & 0x1)){
            process_pdir_table[i].pde[USER_IDX] |= 0x1;

            /* Reload CR3 to flush the TLB */
            pd_addr = (uint32_t) process_pdir_table[i].pde;
            pd_addr &= ADDR_MASK;
            asm volatile (
                "mov %%cr3, %%eax                       ;"
                "andl $0x00000FFF, %%eax                ;"
                "orl %%edx, %%eax   /* Sets CR3 PDBR */ ;"
                "mov %%eax, %%cr3                       ;"
                : 
                : "d" (pd_addr) 
                : "eax"                 
            );
            // return process number
            return i;
        }
    }
    return -1;
}

/*
 * return_parent_paging
 *   DESCRIPTION: reload CR3 to parent's paging structure and flush the TLB once again 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: CR3 will be replaced back to parent process
 */ 
void return_parent_paging(){
    uint32_t pd_addr; /* Address of the page directory (for current process) being loaded into cr3 */

    /* Reload CR3 to parent's paging structure and flush the TLB */
    pd_addr = (uint32_t) process_pdir_table[(int)cur_pcb->parent_pid].pde;
    pd_addr &= ADDR_MASK;
    asm volatile (
        "mov %%cr3, %%eax                       ;"
        "andl $0x00000FFF, %%eax                 ;"
        "orl %%edx, %%eax   /* Sets CR3 PDBR */ ;"
        "mov %%eax, %%cr3                       ;"
        : 
        : "d" (pd_addr) 
        : "eax"                 
    );

    /* Clear out the present bit for the pde we used for the child so it can be used again for another process */
    process_pdir_table[(int)cur_pcb->pid].pde[USER_IDX] -= 0x1;

}

/*
 * swap_task_paging
 *   DESCRIPTION: reload CR3 to next task's paging structure and flush the TLB 
 *   INPUTS: target_pid -- pcb paging that we want to return to
 *   OUTPUTS: none
 *   RETURN VALUE: 0 for succes, -1 for failure
 *   SIDE EFFECTS: CR3 will be replaced back to new process
 */ 
int swap_task_paging(int8_t target_pid){
    uint32_t pd_addr;

    // Check that the target pid is valid
    if(target_pid < 0 || target_pid > MAX_PROCESS){
        return -1;
    }

    /* Reload CR3 to next task's paging structure and flush the TLB */
    pd_addr = (uint32_t) process_pdir_table[(int)target_pid].pde;
    pd_addr &= ADDR_MASK;
    asm volatile (
        "mov %%cr3, %%eax                       ;"
        "andl $0x00000FFF, %%eax                 ;"
        "orl %%edx, %%eax   /* Sets CR3 PDBR */ ;"
        "mov %%eax, %%cr3                       ;"
        : 
        : "d" (pd_addr) 
        : "eax"                 
    );

    return 0;
}

/*
 * baby_malloc
 *   DESCRIPTION: Allocate space for a new buffer for the different consoles
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: on success the physical memory location, on failure NULL
 *   SIDE EFFECTS: Modifies paging structures
 */
void* baby_malloc(){
    int i;

    for(i = 1; i < 4; i++){ // not doing this more than 3 times for now...
        if((vmem_table.pte[i] & 0x1) != 0){ // if not occupied
            vmem_table.pte[i] = ((VIDEO_LOC + (0x1000 * i)) + VMEMPT_BITS) | 0x4; // Set this to where we actually want to be writing to
            return (void*)(VIDEO_LOC + (0x1000 * i));
        }
    }

    return NULL;
}
