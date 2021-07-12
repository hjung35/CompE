#ifndef _FILESYS
#define _FILESYS

#include "types.h"
#include "syscall.h"
#include "console.h"

/* MAGIC NUMBERS FOR file_to_mem  */
/* Magic numbers for executable check */
#define EXECUTABLE_FILE_ONE   0x7f
#define EXECUTABLE_FILE_TWO   0x45
#define EXECUTABLE_FILE_THREE 0x4C
#define EXECUTABLE_FILE_FOUR  0x46

/* Number of bytes of meta data in files */
#define MAX_TEMP_BUF          40
/* Contants of hex values of memory sizes, used for accessing different virtual memory addresses */
#define MB_8                  0x800000
#define KB_8                  0x2000
#define LOAD_LOC    0x08048000  /* Virtual addr that executable should be loaded to */
#define STACK_OFF   2           /* Offset when creating a new process kernel stack so that it does not overlap with the parent's PCB */
#define USER_STACK  0x8400000   /* Virtual address of base of stack for all user processes */

// Won't actually want this to be in here, remove after this quick test ???
//void parse_exec(char* string, char* filename, char* argstr);

/* Pointer to the pcb for the current process, pointer value changes in accordance to process change */
extern pcb_t* cur_pcb;

/* File and directory operations, see their function headers for details */
void init_dir(uint32_t* addr);
int file_open(const uint8_t* fname);
int dir_open(const uint8_t* dirname);
int file_close(int fd);
int dir_close(int fd);
int file_write(int fd, const void * buf, int nbytes);
int dir_write(int fd, const void * buf, int nbytes);
int file_read(int fd, void* buf, int32_t nbytes);
int dir_read(int fd, void* buf, int32_t nbytes);
int file_to_mem(uint8_t* addr, char* fname);
int read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
extern filedesc_t * syscall_getfdptr(uint32_t fd);
extern file_optbl_t stdio_optbl;
filedesc_t * get_free_fd(int * fdnum);
/* Execute and halt system calls! (Not in syscalls.c for sake of readability and concurrent people working) */
int execute(char* strname);
int halt();
uint32_t* filesys_addr;      /* Addr of the filesystem in memory */
#endif
