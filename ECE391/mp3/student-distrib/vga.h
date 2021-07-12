/**
 * @file vga.h
 * @version 0.1
 */
#pragma once
#include "x86_desc.h"
#include "lib.h"
#include "types.h"
/* VGA control ports */
#define VGA_CRTC_ADDRPORT 0x3D4
#define VGA_CRTC_DATAPORT 0x3D5

/* VGA register addresses: all are in CRTC */
#define VGA_CURSORHIGH 0x0E
#define VGA_CURSORLOW 0x0F
#define VGA_CSR 0x0A
#define VGA_CER 0x0B
#define VGA_MSL 0x09
#define VGA_CLH 0x0E
#define VGA_CLL 0x0F
#define VGA_MAXSCAN 0x1F

/* Bitfield for the VGA cursor start register. */
typedef struct vga_csr {
    uint8_t cursor_start    :   5;
    uint8_t cursor_disable  :   1;
    int                     :   2;
}__attribute__((packed)) vga_csr_t;

/* Bitfield for the VGA cursor end register */
typedef struct vga_cer {
    uint8_t cursor_end      :   5;
    uint8_t cursor_skew     :   2;
    int                     :   1;
}__attribute__((packed)) vga_cer_t;


/* Enumerated Types */
typedef enum vga_cursortype {
    CURSOR_UNDERLINE,
    CURSOR_HALFBLOCK,
    CURSOR_BLOCK
} vga_cursortype_t;

/* Function Prototypes */
void vga_enable_cursor(bool cursor_state);
void vga_set_cursor_pos(uint16_t x, uint16_t y);
void vga_set_cursor_type(vga_cursortype_t type);
