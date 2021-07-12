#ifndef _PAGING_H
#define _PAGING_H

#include "types.h"
#include "filesys.h"

#define ADDR_MASK   0xFFFFF000      /* Clears last 12 bits of an an addr for metadata */
#define TBL_SIZE    1024            /* Number of indices in a page table */
#define DIR_SIZE    1024            /* Number of indicies in a page directory */
#define VIDEO_LOC   0xB8000         /* Physical (and virtual/linear) memory addr for video mem */
#define KENREL_LOC  0x00400000      /* Physical (and virtual/linear) mem addr for the kernel */
#define USER_LOC    0x08000000      /* Virtual/linear mem address for user processes */
#define KERNEL_BITS 0x0193          /* Data bits for kernel PDE */
#define VMEMPT_BITS 0x0107          /* Data bits for vmem PTE */
#define VMEMPD_BITS 0x7             /* Data bits for vmem PDE */
#define OFF_PG_BITS 0x6             /* Data bits for non-present PTEs */
#define USER_BITS   0x96            /* Data bits for user PDE */
#define USER_IDX    32              /* Index into PD to get to the user page location (128 MB) */
#define MAX_PROCESS 6               /* Max number of processes running */

/* Structure for Page Directories */
typedef struct page_dir_t {
    uint32_t pde[DIR_SIZE];
} page_dir_t;

/* Structure for Page Tables */
typedef struct page_table_t {
    uint32_t pte[TBL_SIZE];
} page_table_t;

/* Initialize Page Directory, aligned to 4 KB */
page_dir_t page_dir __attribute__((aligned (1024*4)));

/* Initialize Page table for 0-4 M, aligned to 4 KB */
page_table_t vmem_table __attribute__((aligned (1024*4)));
page_table_t vmem_table2 __attribute__((aligned (1024*4)));

/* Table of page tables for user processes (up to 8), aligned to 4kB */
page_dir_t process_pdir_table[MAX_PROCESS] __attribute__((aligned (1024*4)));

/* Function that initializes paging, the kernel page, and the pages for the first 4 MB, see function header for details */
void init_paging();

/* Functions to swap page table to switch processes */
int new_process_ptable();
void return_parent_paging();
int swap_task_paging(int8_t target_pid);


#endif
