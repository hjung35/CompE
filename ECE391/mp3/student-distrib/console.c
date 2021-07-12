/**
 * @file console.c
 * @author Logan Power (lmpower2@illinois.edu)
 * @brief Console driver for the 391 OS
 * @date 2019-10-24
 */

#include "console.h"

// Global state variables
console_t * current_console = NULL;
// console_state_t constate = {.condrv_en = false};
// buffered_input_t current_input;
kbd_input_t kbd_state;
// Vterm variables
console_t vterms[MAX_VTERMS];
volatile int cur_vterm = 0;
volatile vconsole_override_t con_ovr = {0, false};


int console_init()
{
    // Clear the screen
    clear();
    // Set up the console status struct
    console_state_t constate = vterms[0].constate;
    constate.vcur_x = 0;
    constate.vcur_y = 0;
    constate.pcur_en = true;
    constate.kbd_echo_en = true;
    constate.vcur_autoinc_en = true;
    constate.con_scroll = true;
    constate.con_wrap = true;
    constate.con_bkspstop_col = 0;
    // Set the attribute of the default text style to something sane
    constate.charstyle.bg_color = 0;
    constate.charstyle.fg_color = 0x3; // This is only used once.  It makes the text yellow.
    constate.charstyle.fg_intensity = 1;
    constate.charstyle.blink = 0;
    constate.charstyle.underln = 0;
    vga_set_cursor_type(CURSOR_BLOCK);
    vga_enable_cursor(true);
    // TODO: Maybe do something else.
    constate.condrv_en = true;
    vterms[0].constate = constate;
    console_puts("[INFO] Console Initialized.\n", 29);
    return 0;
}

inline void console_refresh_pcur()
{
    console_state_t constate = current_console->constate;
    vga_set_cursor_pos(constate.vcur_x, constate.vcur_y);
}

/**
 * @brief Get the current position of the virtual cursor.
 * 
 * @param x Pointer to where to put x coordinate
 * @param y Pointer to where to put y coordinate
 * @return int Always 0.
 */
int console_getcursorpos(int * x, int * y)
{
    console_t * con = (cur_pcb)? &vterms[cur_pcb->con] : &vterms[0];
    *x = con->constate.vcur_x;
    *y = con->constate.vcur_y;
    return 0;
}

/**
 * @brief Forcibly set the virtual cursor to a location
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @return int 0 on success, nonzero on invalid inputs.
 * This function DOES perform bounds checking
 */
int console_setcursorpos(int x, int y)
{
    if (x < 0 || x >= NUM_COLS) return 1;
    if (y < 0 || y >= NUM_ROWS) return 2;
    console_t * con = (cur_pcb)? &vterms[cur_pcb->con] : &vterms[0];
    con->constate.vcur_x = x;
    con->constate.vcur_y = y;
    return 0;
}
/**
 * @brief Scroll lines off of the top of the video buffer
 * 
 * @param lines Number of lines to scroll
 * @param force_current Force scrolling of the current console.
 * 
 * force_current is useful for keyboard echoing but shouldn't be used for
 * anything else.
 * 
 * This function enables autoincrement and doesn't put it back.
 */
static fastcall void console_scroll(unsigned int lines, bool force_current)
{
    int ln;
    console_state_t * constate = &(vterms[cur_pcb->con].constate);
    console_t * con = (cur_pcb)? &vterms[cur_pcb->con] : &vterms[0];
    if (force_current) {
        // Replace the above pointer with ones to the current console.
        con = current_console;
        constate = &(current_console->constate);
    }
    if (!constate->con_scroll) return;
    for (ln=lines; ln>0; ln--) {
        int i;
        vga_char_t * start_addr = (vga_char_t*)(con->vid_buf + LINE_MEM_LEN);
        // From this point (first character of second line) copy all up
        memmove((int *)con->vid_buf, start_addr, LINE_MEM_LEN * (NUM_ROWS - 1));
	// If this terminal is on the screen, we need to also move the screen contents up one.
	if (con->current) {
        start_addr = (vga_char_t*)(VIDEO + LINE_MEM_LEN);
	    memmove((int*)VIDEO, start_addr, LINE_MEM_LEN * (NUM_ROWS - 1));
	}
        int oldx, oldy;
        // Set the last line to be empty in a mildly hacky way
        if (force_current) {
            oldx = constate->vcur_x;
            oldy = constate->vcur_y;
            constate->vcur_x = 0;
            constate->vcur_y = NUM_ROWS - 1;
        } else {
            console_getcursorpos(&oldx, &oldy);
            console_setcursorpos(0, NUM_ROWS - 1);
        }
        constate->vcur_autoinc_en = true;
        constate->con_scroll = false;
        for (i=0; i<NUM_COLS; i++) {
            if (force_current) {
                console_echo(' ');
            } else {
                console_putchar(' ');
            }
        }
        if (force_current) {
	    constate->vcur_x = oldx;
	    constate->vcur_y = oldy;
	} else {
	    console_setcursorpos(oldx, oldy);
	}
	if (con->current || force_current) {
        vga_set_cursor_pos(constate->vcur_x, constate->vcur_y);
	}
        constate->con_scroll = true;
    }
}

/**
 * @brief Increment the virtual cursor.
 * This function updates the 
 * @param inc Type of increment. CONINC_LINE, _LINRERETURN, or _SCREEN
 * @param force_current Force the cursor of the current console to be updated.
 */
static fastcall void console_inccur(con_inc_t inc, bool force_current)
{
    int num_chars = 1;
    int nc;
    // Use a different console state variable than the current one,
    // in case this is not current.
    console_state_t constate;
    if (!force_current) {
        constate = (cur_pcb)? vterms[cur_pcb->con].constate : vterms[0].constate;
    } else {
        constate = current_console->constate;
    }
    if (inc == CONINC_LINE) num_chars += NUM_COLS;
    if (inc == CONINC_LINERETURN) num_chars += (NUM_COLS - constate.vcur_x - 1);
    else if (inc == CONINC_SCREEN) num_chars += NUM_COLS * NUM_ROWS;
    // Make the cursor invisible for the time being
    // vga_enable_cursor(false); This screws with the multiterm stuff
    for (nc=0; nc<num_chars; nc++) {
        /* Check to see if, when we increment the cursor, it goes off of the screen. */
        if (constate.vcur_x + 1 >= NUM_COLS) {
            // We would go off of the screen.  Increment the line and see what happens.
            if (constate.vcur_y + 1 >= NUM_ROWS) {
                /* We would scroll off of the bottom of the screen.
                 * In order to rectify this, we need to scroll everything on the screen
                 * up by a line. */
                if (constate.con_wrap) {
                    if (constate.con_scroll) console_scroll(1, force_current);
                    // Move the cursor to the last line on the screen and to the beginning.
                    constate.vcur_y = NUM_ROWS - 1;
                    constate.vcur_x = 0;
                }
            } else {
                // We wouldn't scroll off the screen so just write it
                if (constate.con_wrap) {
                    constate.vcur_y += 1;
                    constate.vcur_x = 0;
                }
            }
        } else {
            // We wouldn't go off the screen, so just write it
            constate.vcur_x += 1;
        }
    }
    // Save our temporary console state back to the process' console.
    if (!force_current) {
        *((cur_pcb)? &vterms[cur_pcb->con].constate : &vterms[0].constate) = constate;
    } else {
        current_console->constate = constate;
    }
    if (force_current || 
        ((cur_pcb)? &vterms[cur_pcb->con] : &vterms[0]) == current_console)
    {
        console_refresh_pcur();
        vga_enable_cursor(true);
    }
}

/**
 * @brief Move the cursor back one space.
 */
fastcall void console_bksp()
{
    // Abstract our constate to be the one for the current terminal
    console_state_t constate = current_console->constate;
    // Check to see if we can move backwards
    if (constate.vcur_x <= 0) {
        // Can't go backwards, so try to go up
        if (constate.vcur_y <= 0) {
            // Can't go up either.  Stay here.
            constate.vcur_x = 0;
            constate.vcur_y = 0;
        } else {
            // Can go up.  Do so if we are allowed to.
            if (constate.con_wrap) constate.vcur_y--;
            constate.vcur_x = NUM_COLS - 1;
        }
    } else {
        // Can move backwards.  Do so if we are allowed to.
        if (!constate.con_bkspstop_col || constate.vcur_x > constate.con_bkspstop_col )
            constate.vcur_x--;
    }
    // Clear the character where the cursor is, but don't increment.
    current_console->constate.vcur_autoinc_en = false;
    // console_putchar(' ');
    current_console->constate = constate;
    console_echo(' ');
    current_console->constate.vcur_autoinc_en = true;
    // Resync constate
    current_console->constate = constate;
    console_refresh_pcur();
}

/**
 * @brief Write a character to the current vcur position.
 * Character assumes style as given in the console config.
 * @param c Character to write
 * @return int Nonzero on failure.
 */
fastcall int console_putchar(char c)
{
    /* Handle some stuff */
    if (c == '\n') {
        console_inccur(CONINC_LINERETURN, false);
        return 0;
    }
    // Disable interrupts as our constate should not be changed in the middle.
    cli();
    // Desync ourselves from the real console state.
    console_state_t constate = (cur_pcb)? vterms[cur_pcb->con].constate : vterms[0].constate;
    vga_char_t outchar = {
        .codept = c,
        .underln = constate.charstyle.underln,
        .fg_color = constate.charstyle.fg_color,
        .fg_intensity = constate.charstyle.fg_intensity,
        .bg_color = constate.charstyle.bg_color,
        .blink = constate.charstyle.blink
    };
    vga_char_t * outaddr = (vga_char_t *)(vterm_get_buffer() + ((constate.vcur_x + (constate.vcur_y * NUM_COLS))));
    *outaddr = outchar;
    if (!cur_pcb || cur_pcb->con == current_console->id) {
        // Write through to the screen
        outaddr = (vga_char_t *)(VIDEO + ((constate.vcur_x + (constate.vcur_y * NUM_COLS)) << 1));
        *outaddr = outchar;
    }
    // Resync with the process' console
    *((cur_pcb)? &vterms[cur_pcb->con].constate :& vterms[0].constate) = constate;
    sti();
    // If we are supposed to increment after writing, do so.
    if (constate.vcur_autoinc_en) console_inccur(CONINC_CHAR, false); // This changes the process buffer directly.
    if (!cur_pcb || cur_pcb->con == current_console->id) {
        // Update the VGA cursor position using the write-through.
        constate = (cur_pcb)? vterms[cur_pcb->con].constate : vterms[0].constate;
        vga_set_cursor_pos(constate.vcur_x, constate.vcur_y);
    }
    return 0; // Failure is not an option.
}

/**
 * @brief Write a character to whatever console is current.
 * This function is essentially identical to console_putchar except
 * that it always writes out to the current console,
 * no matter what process is current.
 * @param c Character to write to the terminal.
 */
fastcall int console_echo(char c)
{
    /* Handle some stuff */
    if (c == '\n') {
        console_inccur(CONINC_LINERETURN, true);
        return 0;
    }
    // Disable interrupts as our constate should not be changed in the middle.
    cli();
    // Desync ourselves from the real console state.
    console_state_t constate = current_console->constate;
    vga_char_t outchar = {
        .codept = c,
        .underln = constate.charstyle.underln,
        .fg_color = constate.charstyle.fg_color,
        .fg_intensity = constate.charstyle.fg_intensity,
        .bg_color = constate.charstyle.bg_color,
        .blink = constate.charstyle.blink
    };
    vga_char_t * outaddr = (vga_char_t *)(current_console->vid_buf + ((constate.vcur_x + (constate.vcur_y * NUM_COLS))));
    *outaddr = outchar;
    // Write through to the screen
    outaddr = (vga_char_t *)(VIDEO + ((constate.vcur_x + (constate.vcur_y * NUM_COLS)) << 1));
    *outaddr = outchar;
    // Resync with the process' console
    current_console->constate = constate;
    sti();
    // If we are supposed to increment after writing, do so.
    if (constate.vcur_autoinc_en) console_inccur(CONINC_CHAR, true); // This changes the process buffer directly.
    // Update the VGA cursor position using the write-through.
    constate = current_console->constate;
    vga_set_cursor_pos(constate.vcur_x, constate.vcur_y);
    current_console->constate = constate;
    return 0; // Failure is not an option
}

/**
 * @brief Write n characters to the console (until one fails)
 * Unlike console_puts this function does not check for NULL termination. 
 * @param buf Buffer to pull characters from
 * @param n Number of characters to write
 * @return int Number of characters written.  -1 on failure.
 */
fastcall int console_putn(const char * buf, uint32_t n)
{
    int i;
    for (i=0; i<n; i++) {
        if (console_putchar(buf[i])) return i;
    }
    return i;
}

/**
 * @brief Echo a keyboard keypress.  Handles control combos but not SHIFT.
 * 
 * @param kbdin Raw keypress in (with processed character)
 * @return int Nonzero on failure.
 */
fastcall int console_echokbd(kbd_input_t kbdin)
{
    current_console->constate.vcur_autoinc_en = true;
    if (kbdin.mod.ctrl || kbdin.mod.alt) {
        console_echo('^');
        if (kbdin.char_pressed >= 'a' && kbdin.char_pressed <= 'z') { // Capitalize the letter.
            kbdin.char_pressed -= ASCII_CAPOFF;
        }
        if (kbdin.char_pressed == 'L') {
            console_clrsc();
            console_refresh_pcur();
            return 0;
        }
    }
    if (kbdin.mod.alt || kbdin.char_pressed == '\e') {
        console_echo('[');
    }
    if (kbdin.char_pressed != '\e') console_echo(kbdin.char_pressed);
    return 0;
}

/**
 * @brief Write a string of >= n characters to the console.
 * Note: This function enables cursor autoincrement and doesn't put it back.
 * @param str String to write.  Should be null-terminated.
 * @param n Maximum number of characters to write.
 * 
 * @return int number of characters written
 */
fastcall int console_puts(char * str, int n)
{
    int c = 0;
    // constate.vcur_autoinc_en = true;
    vga_enable_cursor(false);
    while (str[c] != NULL && c < n) {
        console_putchar(str[c]);
        c++;
    }
    vga_enable_cursor(true);
    console_refresh_pcur();
    return c;
}

/**
 * @brief Clear the screen and reset the vcur to the top left.
 */
fastcall void console_clrsc()
{
    // Actually clear the screen
    clear();
    // Set the cursor position to 0,0
    current_console->constate.vcur_x = 0;
    current_console->constate.vcur_y = 0;
    console_refresh_pcur();
}

/**
 * @brief Set up a buffered input structure to prepare to get input.
 * 
 * @param bufin Pointer to buffered input structure
 * @return int Number of characters read
 */
int console_getinput(buffered_input_t * bufin)
{
    bufin->next = 0;
    bufin->complete = false;
    // Stop the user from backspacing beyond this point in the line
    vterms[cur_pcb->con].constate.con_bkspstop_col = vterms[cur_pcb->con].constate.vcur_x;
    // Poll until the interrupt handler makes this true
    sti();
    while (!bufin->complete);
    // Absolutely ensure the buffer is null-terminated
    // EDITED 
    bufin->buffer[bufin->buffer_length - 1] = '\0';
    // Restore backspace
    vterms[cur_pcb->con].constate.con_bkspstop_col = 0;
    return bufin->next;
}

/**
 * @brief Wrapper function for console_getinput
 * 
 * @param cbuf Character buffer
 * @param n Length in bytes of character buffer
 * @return int Number of characters read
 */
int console_readline(char * cbuf, int n)
{
    if (!cbuf) return 0;
    if (!n) return 0;
    vterms[cur_pcb->con].current_input.buffer = cbuf;
    vterms[cur_pcb->con].current_input.buffer_length = n;
    return console_getinput(&(vterms[cur_pcb->con].current_input));
}

/**
 * @brief Get a single character from the console.
 * 
 * @param c Pointer to where to put the character.
 * @return int 0 on success, 1 on failure.
 */
inline int console_getchar(char * c)
{
    return !console_readline(c, 1);
}
/**
 * @brief Handle keyboard input (after the scancode interpreter)
 * 
 * @param raw Raw representation of keypress out of the scancode interp
 * @return void
 * 
 * This function applies modifier keys to characters, and uses console calls
 * to apply some kinds of modifier key (like BKSP).
 * It also updates the state of modifier keys like CAPSLOCK.  
 */
fastcall void console_inputhandler(char raw)
{
    bool printable = true;
    kbd_state.char_pressed = '\0';
    // Desync our local current_input from the global, to handle vterms
    buffered_input_t current_input = current_console->current_input;
    // Handle alpha keys first, since they're simple.  Raw will always be lowercase.
    if (raw >= 'a' && raw <= 'z') {
        // Alpha keys
        if (kbd_state.mod.sft || kbd_state.mod.capslock) {
            kbd_state.char_pressed = raw - ASCII_CAPOFF;
        } else {
            kbd_state.char_pressed = raw;
        }
    } else if (raw >= '0' && raw <= '9') {
        // Numerics: if SHIFT is pressed, use lookup table to translate to special characters
        if (kbd_state.mod.sft) {
            kbd_state.char_pressed = us_numtosym[raw - ASCII_NUMOFF];
        } else {
            kbd_state.char_pressed = raw;
        }
    } else if (raw >= '[' && raw <= ']') {
        // Brackets and backslash group
        if (kbd_state.mod.sft) {
            kbd_state.char_pressed = us_symgroup1[raw - '\x5B'];
        } else {
            kbd_state.char_pressed = raw;
        }
    // Handle the one-offs that don't have a convenient ASCII pattern.
    } else if (raw == ' ') {
        kbd_state.char_pressed = ' ';
    } else if (raw == ';') {
        kbd_state.char_pressed = (kbd_state.mod.sft)? ':' : ';';
    } else if (raw == '\'') {
        kbd_state.char_pressed = (kbd_state.mod.sft)? '\"' : '\'';
    } else if (raw == '`') {
        kbd_state.char_pressed = (kbd_state.mod.sft)? '~' : '`';
    } else if (raw == '-') {
        kbd_state.char_pressed = (kbd_state.mod.sft)? '_' : '-';
    } else if (raw == '=') {
        kbd_state.char_pressed = (kbd_state.mod.sft)? '+' : '=';
    } else if (raw == ',') {
        kbd_state.char_pressed = (kbd_state.mod.sft)? '<' : ',';
    } else if (raw == '.') {
        kbd_state.char_pressed = (kbd_state.mod.sft)? '>' : '.';
    } else if (raw == '/') {
        kbd_state.char_pressed = (kbd_state.mod.sft)? '?' : '/';
    // Handle toggleing CAPSLOCK.  The others depend on knowing press and release and have to be handled elsewhere.
    } else if (raw == '\x1A') { // CAPSLOCK
        kbd_state.mod.capslock = !kbd_state.mod.capslock;
        printable = false;
    // Handle terminal backspace. 
    } else if (raw == '\b') {
        // Rather than print a backspace character (not really a real thing) move the cursor back one and write a blank there.
        console_bksp();
        printable = false;
        // Deal with the input if we have to
        if (!current_input.complete && current_input.next > 0) {
            current_input.next--;
        }
    } else if (raw == '\n') {
        // Move to the next line
        console_inccur(CONINC_LINERETURN, true);
        // If we're recording input, we're done!
        if (current_input.next == current_input.buffer_length) {
            // We've run out of buffer.  NULL and LF the end and return anyway.
            if (current_input.buffer_length > 1) {
                current_input.buffer[current_input.next - 1] = '\0';
                current_input.buffer[current_input.next - 2] = '\n';
            }
            current_input.complete = true;
            printable = false;
        } else {
            if (current_input.buffer_length > 1) {
                current_input.buffer[current_input.next] = '\n';
                current_input.buffer[current_input.next + 1] = '\0'; // Ensure we have a null terminator.
            }
            current_input.complete = true;
            printable = false;  // Technically it is printable but eh
        }
    } else {
        // It clearly wasn't printable.
        printable = false;
    }
    // printf(" in:  %x (%c)  %u as %x (%c)", raw, raw, printable, kbd_state.char_pressed, kbd_state.char_pressed);
    // Now that we know what was pressed, do something with it.
    if (printable && kbd_state.char_pressed != '\0') {
        // Echo it to the screen
        console_echokbd(kbd_state);
        // If we are reading into a buffer, put the data there.
        if (!current_input.complete) {
            if (current_input.next == current_input.buffer_length - 2) {
                // Run out of buffer.  NULL and LF the end, then mark complete.
                if (current_input.buffer_length > 1) {
                    current_input.buffer[current_input.next + 1] = '\0';
                    current_input.buffer[current_input.next] = '\n';
                }
                current_input.complete = true;
            } else {
                current_input.buffer[current_input.next] = kbd_state.char_pressed;
                current_input.next++;
            }
        }
    }
    // Resync current_input to the correct vterm
    current_console->current_input = current_input;
}

inline bool console_isinit()
{
    return vterms[0].constate.condrv_en;
}

/**
 * @brief Wrapper for a console read, using an abstracted input buffer
 * 
 * @param fd File descriptor.  Should be 0, or this call will fail.
 * @param buf Buffer to write in to (eventually)
 * @param nbytes Number of bytes to read.  Should be less than 128.
 * @return int32_t Number of characters read, or -1 on error.
 */
int32_t console_readline_w(int32_t fd, void * buf, int32_t nbytes)
{
    char kbuf[129] = {'\0'};
    int nb = (nbytes >= 128)? 128 : nbytes;
    if (fd != 0) return -1;
    // Read into the kernel stack allocated buffer
    nb = console_readline(kbuf, nb);
    // TODO: Copy this string safely if necessary.
    strncpy(buf, kbuf, nb+1); // +1 here and below is for \n
    kbuf[126] = '\n';
    return nb+1;
}

/**
 * @brief Wrapper for a console write, using a not-abstracted buffer at all
 * 
 * @param fd File descriptor.  Should be 1, or this call will fail.
 * @param buf Buffer to write out of
 * @param nbytes Number of bytes to write.  Can be as big as you want.
 * @return int32_t Number of characters written, or -1 on error.
 */
int32_t console_write_w(int32_t fd, const void * buf, int32_t nbytes)
{
    if (fd != 1) return -1;
    return console_putn(buf, nbytes);
}

/**
 * @brief A function that always returns -1.
 * Don't try to close the console device...
 * @param fd Doesn't matter at all.
 * @return int32_t It's -1, like it says on the tin.
 */
int32_t console_close(int32_t fd)
{
    return -1;
}

/* Lookup table to translate numbers to their corresponding symbols */
char us_numtosym[10] = {
    ')', '!', '@', '#', '$', '%', '^', '&', '*', '('
};

char us_symgroup1[3] = {
    '{', '|', '}'
};


/**
 * @brief Get the address of the screen buffer for the current process.
 * This function reads the current PCB from the kernel stack and returns
 * a pointer to the process' virtual terminal buffer.
 * 
 * @return vga_char_t* Address of the buffer, or NULL on error.
 */
vga_char_t * vterm_get_buffer(void)
{
    // if (!cur_pcb) return NULL;
    if (!cur_pcb) {
        // We have no applications running.
        // Write to the first terminal.
        return vterms[0].vid_buf;
    }
    return vterms[cur_pcb->con].vid_buf;
}
/**
 * @brief Update the actual screen with the contents of the buffer.
 * This checks if you're the real screen or not before it does anything.
 * You should do this after every buffer write, but if you want to be double-
 * buffered you could do it only at the end.
 * 
 * @return void
 */
 /*
fastcall void vterm_blit(void)
{
    // Get the current buffer and screen buffer.
    vga_char_t * tbuf = vterm_get_buffer();
    vga_char_t * sbuf = (vga_char_t*)VIDEO;
    int term_idx = (cur_pcb)? cur_pcb->con : 0;
    // Ensure we have the screen vidmapped and we own the screen.
    if (cur_pcb) {
        if (!cur_pcb->vidmap_check) return;
        if (!vterms[cur_pcb->con].current) return;
    }
    // Write every byte to video memory.
    int i;
    for (i=0; i<NUM_ROWS * NUM_COLS; i++) {
        sbuf[i] = tbuf[i];
    }
    // Sync back the console state
    constate = vterms[term_idx].constate;
    // Update the screen's cursor.
    vga_set_cursor_pos(vterms[term_idx].constate.vcur_x, vterms[term_idx].constate.vcur_y);
}
*/
/**
 * @brief Swap virtual consoles to a new console.
 * 
 * @param new_idx Index of new virtual console to swap to.
 * @return int Nonzero on failure.
 */
int vterm_swap_console(int new_idx)
{
    console_t * cur_con = current_console;
    console_t * new_con;
    // Bounds check the index
    if (new_idx < 0 || new_idx >= MAX_VTERMS) return 1;
    // Check to see if there is a vterm present at that ID.
    if (!vterms[new_idx].present) return 2; // TODO: Do something fancy.
    new_con = &vterms[new_idx];
    // Deactivate the current buffer and save its console state.
    cur_con->current = false;
    // cur_con->constate = constate;
    // Get the new buffer and update the constate from it
    // vga_char_t * sbuf = (vga_char_t*)VIDEO;
    // Save the current console's video memory to the buffer, to ensure it is current.
    memcpy((int*)cur_con->vid_buf, (int*)VIDEO, LINE_MEM_LEN * NUM_ROWS);
    new_con->current = true;
    current_console = new_con;
    // Copy the new buffer to video memory, erasing the old data.
    memcpy((int*)VIDEO, (int*)new_con->vid_buf, LINE_MEM_LEN * NUM_ROWS);
    // Update the cursor
    vga_set_cursor_pos(new_con->constate.vcur_x, new_con->constate.vcur_y);
    vga_enable_cursor(true);
    return 0;
}
/**
 * @brief Blank out and "create" a new virtual terminal.
 * 
 * @param idx Index to create new vterm in.  Fails if out of bounds or occupied.
 * @return int Nonzero on failure.
 */
int vterm_new(int idx)
{
    // Check index
    if (idx < 0 || idx >= MAX_VTERMS) return 1;
    if (vterms[idx].present) return 2;
    // Get a pointer to it for easy manipulation
    console_t * new_con = &vterms[idx];
    // Initialize its constate
    new_con->id = idx;
    new_con->constate.vcur_x = 0;
    new_con->constate.vcur_y = 0;
    new_con->constate.pcur_en = true;
    new_con->constate.kbd_echo_en = true;
    new_con->constate.vcur_autoinc_en = true;
    new_con->constate.con_scroll = true;
    new_con->constate.con_wrap = true;
    new_con->constate.con_bkspstop_col = 0;
    // Set the attribute of the default text style to something sane
    new_con->constate.charstyle.bg_color = 0;
    new_con->constate.charstyle.fg_color = 0x3; // This is only used once.  It makes the text yellow.
    new_con->constate.charstyle.fg_intensity = 1;
    new_con->constate.charstyle.blink = 0;
    new_con->constate.charstyle.underln = 0;
    new_con->constate.charstyle.codept = ' ';
    new_con->constate.condrv_en = true;
    // Clear out buffers.
    new_con->kbuf[0] = '\0';
    new_con->current_input.buffer = '\0';
    new_con->current_input.buffer_length = 0;
    // Fill screen buffer with spaces.
    int i;
    vga_char_t * sbuf = (vga_char_t*)VIDEO;
    for (i=0; i<NUM_ROWS*NUM_COLS; i++) {
        sbuf[i] = new_con->constate.charstyle;
    }
    // Reposition the cursor
    // console_setcursorpos(0, 0); THIS DOESN'T DO WHAT YOU WANT and is also not needed.
    // Should be good to go now.
    new_con->present = true;
    return 0;
}
/**
 * @brief Initialize the first virtual terminal
 * Since the first vterm is just the same terminal we've always had, 
 * formalize it into the vterm system.
 * @return int Nonzero on failure.
 */
int vterm_init(void)
{
    vterm_new(0);
    vterms[0].current = true;
    current_console = &vterms[0];
    return 0;
}
