/**
 * @file keyboard.c
 * @author Logan Power (lmpower2@illinois.edu)
 * @brief Keyboard driver for the MP3 OS.
 * 
 * @date 2019-10-18
 */

#include "keyboard.h"

/**
 * @brief Interpret a keyboard scancode from set 1.
 * 
 * @param pri Primary byte.  This is the one that's different between all keys.
 * @param sec Secondary byte. Either KBD_LSHIFT0 for special keys or 0 for normal ones.
 * @return char OS keycode for the key (ASCII if it's a character) otherwise '\0' if no key is down now.
 */
char interpret_scancode(uint8_t pri, uint8_t sec)
{
    // Take from the proper scancode bank, depending on the value of sec.
    char ascii;
    static char last;
    if ((pri & ~RELEASED_MASK) > SCANCODE_SET1_LEN) return '\0'; // Throw out invalid scancodes.
    if (!sec) {
        ascii = scan1_normal[pri & ~RELEASED_MASK] ;
    } else {
        ascii = scan1_special[pri * 0]; 
    }
    // printf("\n[DEBUG] %x (%c) pri %x.", ascii, ascii, pri);
    if (pri > RELEASED_MASK) {
        // Whatever key this was was released.  Handle if it's a modifier, otherwise do nothing.
        if (ascii == KBD_LSHIFT || ascii == KBD_RSHIFT) {
            // SHIFT was released.  Turn it off.
            kbd_state.mod.sft = 0;
        } else if (ascii == KBD_LCTRL || ascii == KBD_RCTRL) {
            // CTRL was released.  Turn it off.
            kbd_state.mod.ctrl = 0;
        } else if (ascii == KBD_LALT || ascii == KBD_RALT) {
            // ALT was released.  Turn it off.
            kbd_state.mod.alt = 0;
        } else {
            // Clear out last if it matches.
            if (ascii == last) last = '\0';
        }
    } else {
        // This is a newly pressed key.  Handle it.
        // If it's a modifier, catch it.
        if (ascii == KBD_LSHIFT || ascii == KBD_RSHIFT) {
            // SHIFT was pressed.  Turn it on.
            kbd_state.mod.sft = 1;
        } else if (ascii == KBD_LCTRL || ascii == KBD_RCTRL) {
            // CTRL was pressed.  Turn it on.
            kbd_state.mod.ctrl = 1;
        } else if (ascii == KBD_LALT || ascii == KBD_RALT) {
            // ALT was pressed.  Turn it on.
            kbd_state.mod.alt = 1;
        } else if (ascii == '\b') {
            // Allow for backspace key repetition
            // console_bksp();
            console_inputhandler(ascii);
        } else if (ascii >= '\xF1' && ascii <= '\xFA') {
            // Handle function key presses.
            if (kbd_state.mod.alt) {
                // This is ALT+Fx.  Attempt to change vconsole.
                if (ascii < MAX_VTERMS) {
		    unsigned int console = (ascii & 0x0F) - 1; // Discard the upper half of the byte, as it's just there to prevent collisions.
                    if (vterms[console].present) {
                        vterm_swap_console(console);
                    }
                }
            }
        } else if (ascii != last) {
            console_inputhandler(ascii);
            last = ascii;
        }
    }
    /* 
    if (ascii != last) {
        last = ascii;
        return ascii;
    } else {
        if (pri > 0x80) {
            // Key was released.
            last = '\0';
            return '\0';
        } else {
            // Dunno how you got here.
            return ascii;
        }
    }
    */
    return ascii;
}
/**
 * @brief Keyboard interrupt service routine
 * If this function is called, a key has been pressed
 * for the first PS/2 channel on the controller.
 */
DECLARE_ISR(kbd_int)
{
    // Take the key pressed, interpret it, and print it out (for checkpoint 1)
    // Read the key pressed from the keyboard
    uint8_t keycode_primary;
    uint8_t keycode_secondary;
    keycode_primary = inb(KEYBOARD_DATAPORT);
    if (keycode_primary == CODE1_PRIMARYFLAG) { // This is because we are operating on scan code 1 (the translation bit on the controller is set.)
        // This is a special key, so read the second byte.
        io_wait();
        keycode_secondary = keycode_primary;
        keycode_primary = inb(KEYBOARD_DATAPORT);
    } else {
        keycode_secondary = 0;
    }
    interpret_scancode(keycode_primary, keycode_secondary);
    // char key = scan1_normal[keycode_primary];
    send_eoi(KBD_INT_NUM); // FIXME: We might have to be Linux and send EOI at the beginning.
}

/**
 * @brief Initialize the PS/2 controller (which is effectively the keyboard)
 * 
 */
void keyboard_init(void)
{
    uint8_t in_garbage;
    // In case we are re-initializing, mask the PIC
    disable_irq(KBD_INT_NUM);
    // Disable the PS/2 controller while we work on it.
    outb(PS2_DISABLE, KEYBOARD_CMDPORT);
    io_wait();
    // Read something from the data port to make sure the buffer is empty.
    in_garbage = inb(KEYBOARD_DATAPORT);
    io_wait();
    // Don't assume that the keyboard controller is in the state we want it.
    ps2_config_t config_byte = { .st= {
        .ps2_int1 = 1, // Enable interrupts for the keyboard.
        .ps2_int2 = 0, // Don't for the mouse (we don't handle them)
        .sysflg = 0, // Dunno what this is supposed to do
        .zero1 = 0, 
        .ps2_clk1 = 1, // Enable clock on keyboard
        .ps2_clk2 = 0, // but not for the mouse
        .ps2_xlat = 1, // Translate the keyboard's scancodes into code 1
        .zero2 = 0
    }};
    // We're supposed to make sure we don't write to the controller until it's ready.
    while (inb(KEYBOARD_CMDPORT) & BIT_1);
    outb(PS2_CONFIGBYTE, KEYBOARD_CMDPORT); // This says I want to write to the config byte
    while (inb(KEYBOARD_CMDPORT) & BIT_1);
    outb(config_byte.byte, KEYBOARD_DATAPORT); // This sends the command byte.
    // Register the keyboard interrupt.
    load_int(KBD_INT_NUM, &kbd_int, KERNEL_SEGMENT, INTERRUPT_DPL);
    // Re-enable the PS/2 controller now that we have configured it.
    while (inb(KEYBOARD_CMDPORT) & BIT_1);
    outb(PS2_ENABLE, KEYBOARD_CMDPORT);
    // Finally, unmask its interrupt.
    enable_irq(KBD_INT_NUM);
}
/**
 * @brief Table of scancodes and equivalent keycodes.
 * Keys that don't have characters are encoded as the following:
 * ENTER: \n
 * F1-F12: 0x1-0xA
 * CAPSLOCK: 0x1A (SUB)
 * NUMLOCK: 0x1C (FS)
 * SCROLLOCK: 0x16 (SYN)
 * LCTRL: KBD_LCTRL (DC1)
 * RCTRL: KBD_RCTRL (DC2)
 * LSHIFT: KBD_LSHIFT (SO)
 * RSHIFT: KBD_RSHIFT (SI)
 * LALT: KBD_LALT (DC3)
 * RALT: KBD_RALT (DC4)
 */
char scan1_normal[SCANCODE_SET1_LEN] = {
    '\0', '\e',
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']', '\n', '\x11',
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    '\xE', '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 
    '\xF', '*', '\x13', ' ', '\x1A',
    '\xF1', '\xF2', '\xF3', '\xF4', '\xF5', '\xF6', '\xF7', '\xF8', '\xF9', '\xFA',
    '\x1C', '\x16', 
    '7', '8', '9', '-', 
    '4', '5', '6', '+',
    '1', '2', '3', '0', '.',
    '\xB', '\xC'
};
// TODO: IMPLEMENT ME (maybe)
char scan1_special[SCANCODE_SET1_SPECIALLEN] = {
    '\0'
};
