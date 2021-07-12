/* types.h - Defines to use the familiar explicitly-sized types in this
 * OS (uint32_t, int8_t, etc.).  This is necessary because we don't want
 * to include <stdint.h> when building this OS
 * vim:ts=4 noexpandtab
 */

#ifndef _TYPES_H
#define _TYPES_H

#define NULL 0

#ifndef ASM

/* Types defined here just like in <stdint.h> */
typedef int int32_t;
typedef unsigned int uint32_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef char int8_t;
typedef unsigned char uint8_t;

/* We don't have stdbool.h either so we define bool here too. */
typedef _Bool bool;
#define true 1
#define false 0
typedef uint32_t size_t;

/* Types that are referenced widely */
/* All filesystem data types go in here */
#define BLOCK_SIZE 4096     /* The file system is divided into 4 KB blocks */
#define MAX_DENTRY_NUM 63   /* There can be up to 63 directory entries */
#define MAX_FNAME_SIZE 32   /* File names are up to 32 chars */
#define DENTRY_SIZE 0x40    /* Size in bytes of directory entries */
#define MAX_OPEN_FILES 8    /* Max number of files open at once */

typedef struct dentry {
    char        fname[MAX_FNAME_SIZE];
    uint32_t    ftype;
    uint32_t    inode_num;
    uint32_t    reserved[6];    /* Reserved will be 6 for cp3 */
   // uint32_t    dentry_idx;     /* temp var track this dentry's idx, only used for directory open/read */
    //uint32_t    init_flag;      /* Temp flag for cp2, indicating if a file is open (flag = 1), or closed (flag = 0) */
    //uint32_t    position;        /* temp var used to keep track of byte position in file */
} dentry_t;

typedef struct inode {
    uint32_t length;
    uint32_t data_blocks[1023]; // This is the max number of datablocks in each inode
} inode_t;

typedef struct fd_flags {
    bool in_use         :   1;
    int  pad1           :   31; // If you add more fields make this smaller
} fd_flags_t;

typedef struct file_optbl {
    int32_t (*open)(const uint8_t*);
    int32_t (*read)(int32_t, void *, int32_t);
    int32_t (*write)(int32_t, const void *, int32_t);
    int32_t (*close)(int32_t);
} file_optbl_t;

typedef struct filedesc {
    file_optbl_t * optbl;
    uint32_t inode;
    uint32_t pos;
    fd_flags_t flags;
} filedesc_t;

/* Structure for the Process Control block */
typedef struct pcb{
    filedesc_t  file_array[MAX_OPEN_FILES];     /* File array for file descriptor info for current process */
    int8_t      parent_pid;                     /* Process number for parent process (-1 for initial shell) */
    int8_t      pid;                            /* Process number for current process */
    uint32_t    ret_addr;                       /* Address of the line to return to after halt is called (PCB already switched from child to parent) */
    uint32_t    esp;                            /* Stack pointer before context switches to child */
    uint32_t    ebp;                            /* Stack base pointer before context switches to child */
    char        args[128];                      /* Buffer of arguments, 128 because that is max kbdr buffer size */
    bool        vidmap_check;                   /* Var to check if current process has called vidmap, so halt can teardown user vidmapping */
    int con;                                    /* Pointer to the console this process should read from and write to. */
} pcb_t;

/* Fastcall macro */
#define fastcall __attribute__((fastcall))

#endif /* ASM */

#endif /* _TYPES_H */
