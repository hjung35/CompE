/* PCB structure  
 * Author:	    Hyun Do Jung(MJ)
 * Version:	    1
 * Creation Date:   Tue Nov 12 22:05:34 2019
 * Filename:	    pcb.h
 * History:
 *	MJ	1	Tue Nov 12 22:05:34 2019
 *		First written
 */


#define MAX_FD_OPEN_FILES 8 // previously max_open_files in execute.h


/* process control block structure */
/* this should be done to organize the whole structure thing so we can keep track of pcb here and there */
/*
typedef struct pcb_t {

    filedesc_t file_array[MAX_FD_OPEN_FILES];

    uint8_t* program;
    uint8_t arguments[128];

    int8_t pid; // process id 
    int32_t parent; // parent process id
    int32_t child; // child process id 

    uint32_t parent_ebp; 
    uint32_t parent_esp; 
    uint32_t parent_eip; 

    uint32_t process_starting_addr; // process starting addess 
    uint32_t page_addr;  // physical address of page

} pcb_t;
*/
extern pcb_t* current_pcb; 
