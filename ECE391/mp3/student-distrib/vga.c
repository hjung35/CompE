/**
 * @file vga.c
 * @author Logan Power (lmpower2@illinois.edu)
 * @brief Utilities for manipulating text-mode VGA parameters
 * @date 2019-10-24
 */

#include "vga.h"

/**
 * @brief Set a VGA CRTC register to a value.
 * 
 * @param reg Register to set
 * @param val Pointer to value to set it to
 */
static void vga_set_crtc_reg(uint8_t reg, uint8_t * val)
{
    // The CRTC registers are programmed by writing the address to port 0x3D4 and data to 0x3D5
    outb(reg, VGA_CRTC_ADDRPORT);
    io_wait();
    outb(*val, VGA_CRTC_DATAPORT);
    io_wait();
}

/**
 * @brief Get the value of a VGA CRTC register.
 * 
 * @param reg Register to get the value of.
 * @param valbuf Pointer to where to put the data.
 */
static void vga_get_crtc_reg(uint8_t reg, uint8_t * valbuf)
{
    outb(reg, VGA_CRTC_ADDRPORT);
    io_wait();
    *valbuf = inb(VGA_CRTC_DATAPORT);
}

/**
 * @brief Enable or disable the VGA cursor
 * 
 * @param cursor_state True=on, False=off
 */
void vga_enable_cursor(bool cursor_state)
{
    // Preserve the old CSR, except for what we want to change.
    vga_csr_t new_csr;
    vga_get_crtc_reg(VGA_CSR, (uint8_t*)&new_csr);
    new_csr.cursor_disable = !cursor_state;
    vga_set_crtc_reg(VGA_CSR, (uint8_t*)&new_csr);
}

/**
 * @brief Set the type of the cursor.
 * 
 * @param type Cursor type.  One of CURSOR_UNDERLINE, CURSOR_BLOCK, or CURSOR_HALFBLOCK
 */
void vga_set_cursor_type(vga_cursortype_t type)
{
    uint8_t scanline_end, max_scanline;
    vga_csr_t new_csr;
    vga_cer_t new_cer;
    vga_get_crtc_reg(VGA_MSL, &max_scanline);
    max_scanline &= VGA_MAXSCAN;
    switch(type) {
    case CURSOR_UNDERLINE:
        scanline_end = 1;
        break;
    case CURSOR_BLOCK:
        scanline_end = max_scanline - 1;
        break;
    case CURSOR_HALFBLOCK:
        scanline_end = max_scanline / 2; // I'm going to leave off the -1 since the /2 is likely to truncate anyway.
        break;
    default:
        break;
    }
    // Preserve the old CSR and CER, except for what we want to change.
    vga_get_crtc_reg(VGA_CSR, (uint8_t*)&new_csr);
    vga_get_crtc_reg(VGA_CER, (uint8_t*)&new_cer);
    new_csr.cursor_start = 0; // For all of these options we start at scanline 0
    new_cer.cursor_end = scanline_end;
    // Write back
    vga_set_crtc_reg(VGA_CSR, (uint8_t*)&new_csr);
    vga_set_crtc_reg(VGA_CER, (uint8_t*)&new_cer);
}

void vga_set_cursor_pos(uint16_t x, uint16_t y)
{
    //changed from int to uint8_t
    uint8_t cval_temp;
    if (x > NUM_COLS || y > NUM_ROWS) return;
    // Convert the X and Y coordinates into a cell address
    uint16_t cell_addr = (y * NUM_COLS) + x;
    // outb((cell_addr & 0xFF00) >> 8, VGA_CURSORHIGH); 
    cval_temp = (cell_addr & 0xFF00) >> 8; // Not really magic numbers, just getting the high byte
    vga_set_crtc_reg(VGA_CURSORHIGH, &cval_temp);
    io_wait();
    // outb((cell_addr & 0x00FF), VGA_CURSORLOW);
    cval_temp = (cell_addr & 0x00FF);
    // added typecast
    vga_set_crtc_reg((uint8_t)VGA_CURSORLOW, &cval_temp);
    io_wait();
}
