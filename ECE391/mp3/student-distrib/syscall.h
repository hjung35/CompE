/**
 * @file syscall.h
 * MP3.4 added by HDJ 
 */
#pragma once

#include "types.h"
#include "interrupts.h"
#include "console.h"
#include "filesys.h"
#include "rtc.h"

#define NUM_FDS 8
#define SYSCALL_INT 0x80
#define SYSCALL_DPL 3 // System calls should be accessible from user space.

/* MP3.4 added by MJ */
#define FAILURE -1
#define SUCCESS 0
#define BUF_MAX_SIZE 128
#define EMPTY_CHAR '\0'

// Jump table of system call functions
extern void * syscall_jumptbl[16];

// typedef struct dentry dentry_t;

extern void syscall_handler(void);
extern int syscall_add(size_t, void *);
extern int32_t syscall_init();

extern file_optbl_t regfile_optbl;
extern file_optbl_t dir_optbl;
extern file_optbl_t rtc_optbl;

/* MP3.4 added by MJ */
extern int32_t getargs (uint8_t* buf, int32_t nbytes);
extern int32_t vidmap (uint8_t** screen_start);

