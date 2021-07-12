/**
 * @file keyboard.h
 * @date 2019-10-18
 */
#pragma once
#include "types.h"
#include "lib.h"
#include "interrupts.h"
#include "console.h"

#define KEYBOARD_DATAPORT 0x60
#define KEYBOARD_CMDPORT 0x64
#define BIT_1 0x02
#define RELEASED_MASK 0x80
#define CODE1_PRIMARYFLAG 0xE0
#define PS2_CONFIGBYTE 0x60
#define PS2_DISABLE 0xAD
#define PS2_ENABLE 0xAE

#define KBD_CAPSLOCK 0x1A
#define KBD_LCTRL 0x11
#define KBD_RCTRL 0x12
#define KBD_LSHIFT 0xE
#define KBD_RSHIFT 0xF
#define KBD_LALT 0x13
#define KBD_RALT 0x14

/* PS2 configuration byte bitfield.
 * Send this to the PS/2 controller to configure it
 * (or some of the basic things about it).
 */
typedef union {
    uint8_t byte;
    struct {
        bool ps2_int1   :   1;
        bool ps2_int2   :   1;
        bool sysflg     :   1;
        bool zero1      :   1;
        bool ps2_clk1   :   1;
        bool ps2_clk2   :   1;
        bool ps2_xlat   :   1;
        bool zero2      :   1;
    } __attribute__((packed)) st;
} ps2_config_t;

/* Scan code set 1 scancodes for interpreting keystrokes.  Only corresponds to en-US. */
#define SCANCODE_SET1_LEN 0x58
#define SCANCODE_SET1_SPECIALLEN 1
extern char scan1_normal[SCANCODE_SET1_LEN];
extern char scan1_special[SCANCODE_SET1_SPECIALLEN];

/* Function */
char interpret_scancode(uint8_t pri, uint8_t sec);
void keyboard_init(void);
void kbd_int(void);
