/* Scheduling  
 * Author:	    Hyun Do Jung(MJ)
 * Version:	    1
 * Creation Date:   Wed Nov 27 18:58:43 2019
 * Filename:	    scheduling.h
 * History:
 *	MJ	1	Wed Nov 27 18:58:43 2019
 *		First written.
 */

#include "types.h"
#include "syscall.h"
#include "filesys.h"
#include "paging.h"
#include "x86_desc.h"
#include "lib.h"
#include "interrupts.h"

#define TASK_SUCCESS            0
#define TASK_FAIL              -1
#define QUEUE_SIZE	            16

#define PIT_DATA_REG			0x40
#define PIT_CMD_REG				0x43
#define PIT_CLOCK				1193180	// Hz
#define QUANTUM_RATE			2		// hz 20 normally, making it big for testing
#define PIT_CMD_DATA			0x34	// Fields of the reg are listed below
/*	7  6  5 4  3 2 1  0 
	cntr  rw   mode   bcd 
	0 0	  11   010	  0
*/


/* basic task struct */
typedef struct task{
	void* esp;
	void* ebp;
    int8_t state;
	uint32_t terminal_num;
	bool enabled;			/* If a shell spawns a child we don't want that shell getting cpu time, false=asleep */
    pcb_t* pcb;
} task_t;


/* Basic task/scheduling function prototypes */
int32_t init_schedule(void);
int32_t task_schedule(int32_t ebp, int32_t esp);
int32_t switch_context_task(int32_t ebp, int32_t esp);
int32_t clear_struct(task_t* current_task);

int32_t task_push(task_t* current_task);
int32_t task_pop(task_t* current_task);
//int32_t task_front(task_t** element);

void task_grab(task_t* current_task);

task_t task_queue[QUEUE_SIZE];
extern volatile int terminal_schedule;
