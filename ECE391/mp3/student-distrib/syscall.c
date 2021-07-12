/**
 * @file syscall.c
 * @author Logan Power (lmpower2@illinois.edu) / Hyun Do Jung (hjung35@illinois.edu)
 * @brief System call linkage and handling
 * @version 0.2
 * @date 2019-10-25 / 2019-11-13(working on getargs, vidmap)
 */

#include "syscall.h"
#include "filesys.h"
#include "pcb.h"
#include "paging.h"
#ifndef NO_SYSCALL

/* MP3.4 added by MJ, current pcb initialization */
pcb_t* current_pcb; 

/// Jump table for system call functions.  Put in pointers.
void * syscall_jumptbl[16] = {0x0};

/* These following variables are the operations tables for the 3
 * kinds of file: regular, terminal/character, or RTC.
 * Pointers to these are what should be put inside file descriptor
 * entries. */
/// Operations table for a regular file
file_optbl_t regfile_optbl = {
    .open = file_open,
    .read = file_read,
    .write = file_write,
    .close = file_close,
};
// Operations table for the directory
file_optbl_t dir_optbl = {
    .open = dir_open,
    .read = dir_read,
    .write = dir_write,
    .close = dir_close,
};
/// Operations table for an RTC device file
file_optbl_t rtc_optbl = {
    .open = rtc_open,
    .read = rtc_read,
    .write = rtc_write,
    .close = rtc_close,
};
/// Operations table for a terminal device (FDs 0, 1)
file_optbl_t stdio_optbl = {
    .read = console_readline_w,
    .write = console_write_w,
    .close = console_close,
};

/**
 * System calls are invoked by an INT 0x80 instruction, so the
 * system call is an interrupt handler.
 * The system call number is stored in eax, and the arguments
 * to it are stored in ebx, ecx, and edx.
 * What we'd like to do is translate this calling convention
 * to look like CDECL (and of course use eax to call the right function)
 * It'd be kind of hard to do this in C without everything breaking.
 */

extern void syscall_handler(void);
asm (
    ".global syscall_handler\n"
    ".align 4\n"
    "syscall_handler:\n\t"
    // "pusha\n\t" NO because we don't want to pop everything at the end.
    "pushl %esp\n\t"
    "pushl %ecx\n\t"
    "pushl %edx\n\t"
    "pushl %ebx\n\t"
    "pushl %ebp\n\t"
    "pushl %esi\n\t"
    "pushl %edi\n\t"
    "cld\n\t"
    "cmpl $0, %eax\n\t"
    "jle invalid_syscall\n\t"
    "cmpl $10, %eax\n\t"
    "jg invalid_syscall\n\t"
    "movl syscall_jumptbl(, %eax, 4), %edi\n\t"
    "cmpl $0, %edi\n\t"
    "je invalid_pointer\n\t"
    "push %edx\n\t"
    "push %ecx\n\t"
    "push %ebx\n\t"
    "call *syscall_jumptbl(, %eax, 4)\n\t"
    // "popl %eax\n\t" // Pop the return value into EAX.
    "addl $12, %esp\n\t"
    // "popa\n\t" NO, because it'll overwrite EAX.
    "jmp cleanup\n"
    "invalid_syscall:\n"
    "invalid_pointer:\n\t"
    "movl $-1, %eax\n"
    "cleanup:\n\t"
    "popl %edi\n\t"
    "popl %esi\n\t"
    "popl %ebp\n\t"
    "popl %ebx\n\t"
    "popl %edx\n\t"
    "popl %ecx\n\t"
    "popl %esp\n\t"
    "iret\n"
);

/**
 * @brief Register a system call with the system
 * 
 * @param id ID of the system call.  For now, 0<ID<16
 * @param fx Pointer to function to call.  NULL is allowed.
 * @return int 0 on success, nonzero on failure
 */
int syscall_add(size_t id, void * fx)
{
    if (id < 0 || id > 16) return 1;
    syscall_jumptbl[id] = fx;
    return 0;
}

/**
 * @brief Call read for filetype given by the fd
 * 
 * @param fd filedescriptor to be read
 * @param buf Pointer to buffer to be filled by read.
 * @param nbytes Number of bytes to be read
 * @return int 0 on success, nonzero on failure
 */
int32_t read(int32_t fd, void * buf, int32_t nbytes)
{
    // Get the FD and just call the table function.
    filedesc_t * fdesc = syscall_getfdptr(fd);
    if (!fdesc || !fdesc->flags.in_use) return -1;
    return (fdesc->optbl->read)(fd, buf, nbytes);
}

/**
 * @brief Call write for filetype given by the fd
 * 
 * @param fd filedescriptor to be read
 * @param buf Pointer to buffer to be written.
 * @param nbytes Number of bytes to be written
 * @return int 0 on success, nonzero on failure
 */
int32_t write(int32_t fd, void * buf, int32_t nbytes)
{
    // Get the FD and just call the table function.
    filedesc_t * fdesc = syscall_getfdptr(fd);
    if (!fdesc || !fdesc->flags.in_use) return -1;
    return (fdesc->optbl->write)(fd, buf, nbytes);
}

/**
 * @brief System call to close a file
 * 
 * @param fd FIle descriptor to close
 * @return int32_t 
 */
int32_t close(int32_t fd)
{
    // Get the FD and just call the table function.
    filedesc_t * fdesc = syscall_getfdptr(fd);
    if (!fdesc || !fdesc->flags.in_use) return -1;
    int retval = (fdesc->optbl->close)(fd);
    if (retval == -1) return -1;
    // Mark the descriptor as not in use if we successfully closed it.
    else {
        fdesc->flags.in_use = false;
        return 0;
    }
}

/**
 * @brief Open a file or device
 * 
 * @param filename Filename of file to open.
 * @return int32_t File descriptor of opened file, or -1 on error.
 */
int32_t open(const uint8_t * filename)
{
    int fd;
    //filedesc_t * fdptr; 
    dentry_t cur_dentry;
    if (!filename) return -1;
    // Find the file by filename.  If there's nothing, return.
    if (read_dentry_by_name((uint8_t *)filename, &cur_dentry) == -1) return -1;
    // Use the dentry information to figure out what to do.
    switch(cur_dentry.ftype) {
        case 0:{
            // This is the RTC
            fd = rtc_open(filename);
        }break;
        case 1: {
            // Type 1 is directory.
            fd = dir_open(filename);
        }break;
        case 2: {
            fd = file_open(filename);
        }break;
        default:
            return -1; // Unknown file type.
        break;
    }
    return fd;

}

/**
 * @brief Initialize system calls.
 * 
 * @return int32_t 0 on success, -1 on failure.
 */
int32_t syscall_init()
{
    // Put the system calls into the table, skipping 0.
    syscall_jumptbl[1] = halt;
    syscall_jumptbl[2] = execute;
    syscall_jumptbl[3] = read;
    syscall_jumptbl[4] = write;
    syscall_jumptbl[5] = open;
    syscall_jumptbl[6] = close;
    syscall_jumptbl[7] = getargs;
    syscall_jumptbl[8] = vidmap;
    // Register the system call in to the IDT
    return 0;
}


/*
 * getargs
 *   DESCRIPTION: getargs call reads the program's command line arguments into a user-level buffer 
 *   INPUTS: buf-- pointer to user-level-buffer that our arguments to be inputted 
 *           nbytes-- number of bytes to read from the agruments buffer
 *   OUTPUTS: none
 *   RETURN VALUE: return -1 when fails, 0 for success
 *   SIDE EFFECTS: parsing arguments is what getargs has to do mainly and put them into the buffer we get.               
 */  

int32_t getargs (uint8_t* buf, int32_t nbytes){

int i, arg_length;

/* Sanity Check#1: Null pointer, or zero bytes */
if((buf == NULL) || (nbytes == 0) || (uint32_t)buf < USER_LOC || (uint32_t)buf > USER_LOC + (MB_8/2))
    return FAILURE;

/* Sanity Check#2: Arguments check whether exists or not */
if(cur_pcb->args == NULL)
    return FAILURE;

/* get pcb argument length */
arg_length = strlen((int8_t*)cur_pcb->args);

/* Sanity Check#3: if argument legnth is actually zero or read bytes are shorter than argument length */
if((arg_length == 0) || (nbytes < arg_length))
    return FAILURE;

/* if all sanity cheks are fine, we empty the buffer and copy arguments into the buffer */    
else{

    /* clear the buffer */
    for(i=0; i < nbytes; i++){
        buf[i] = EMPTY_CHAR;
    }

    /* copy into the buffer */
    strncpy((int8_t*)buf, (int8_t*)cur_pcb->args, arg_length);
    
}

return SUCCESS;
}

/*
 * vidmap
 *   DESCRIPTION: vidmap call maps the text-mode video memory into user-space at a pre-set virtual address 
 *   INPUTS: screen_start -- double poitner to the starting address of the screen 
 *   OUTPUTS: none
 *   RETURN VALUE: return -1 when fails, 0 for success
 *   SIDE EFFECTS: gives permission to modify/manipulate video memory. Location is already setup and used by user stack, thus it looks like "doubly pointed"
 */ 
int32_t vidmap (uint8_t** screen_start){
    /* Check if input arg is NULL, in the kernel, or at the original kernel video mapping */
    if(!screen_start || screen_start == (uint8_t**)VIDEO_LOC || screen_start == (uint8_t**)KENREL_LOC){
        return FAILURE;
    }
    /* Turn on the User bit (0x4) in addition to the regular pt bits for video memory */
    vmem_table2.pte[(int)cur_pcb->pid] = (VIDEO_LOC + VMEMPT_BITS) | 0x4;
    /* Make sure we are clearing this mapping for each halt */
    if(cur_pcb->vidmap_check != false){
        return FAILURE;
    }
    cur_pcb->vidmap_check = true;

    /* We load this video memory location in at 120 MB virtual memory, for no reason other than it is in between the kernel and user pages */
    *screen_start = (uint8_t*) USER_LOC - MB_8 + (cur_pcb->pid * 0x1000); // 0x1000 is 4kb

    return SUCCESS;
}

// This function remaps video memory for a nonactive terminal
// return -1 for failure, 0 for success;
// We assume..........
// This is only done when vidmap is already enabled???
int nonactive_vidmap(void* buffer){
    if(buffer == NULL){
        return -1;
    }
    vmem_table2.pte[(int)cur_pcb->pid] = ((((int)buffer) & ADDR_MASK) + VMEMPT_BITS) | 0x4;     // We set the mapping to go to the buffer we want to fill

    // Then we reload CR3 to flush the TLB
    asm volatile(
        "mov %%cr3, %%eax       ;"
        "mov %%eax, %%cr3       ;"
        :
        :
        : "eax"
    );

    return 0;
}


#endif /* NO_SYSCALL */
