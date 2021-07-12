/**
 * @file console.h
 * @date 2019-10-24
 */

#pragma once
#include "lib.h"
#include "types.h"
#include "vga.h"

extern pcb_t * cur_pcb;

#define ASCII_CAPOFF 0x20
#define ASCII_NUMOFF 0x30
#define KBUF_LEN 129
#define NUM_ROWS 25
#define NUM_COLS 80

typedef enum con_inc {
    CONINC_CHAR,
    CONINC_LINE,
    CONINC_LINERETURN,
    CONINC_SCREEN
} con_inc_t;

/**
 * @brief Struct to control input coordination between the kbd int and synchronous stuff
 * 
 * We use this struct to get input when we'd like to read a line or character from the terminal.
 * console_getinput sets up this record 
 * (assigns the correct pointer and length, and marks it incomplete).
 * console_inputhandler, if we are actively buffering input, will throw characters 
 * into the buffer, in addition to echoing them to the screen.  If the length of the buffer is
 * reached, a newline will be automatically placed at the end.
 * When this occurs, or if the user presses return, the buffer will be marked as complete.
 * The getinput function will poll the complete field until it's marked true, and then return what's
 * in the buffer.
 * 
 * THIS DATA STRUCTURE IS ONLY TO BE USED WITHIN KERNEL SPACE!
 */
typedef struct buffered_input {
    size_t buffer_length;
    char * buffer;
    size_t next;
    bool complete;
} buffered_input_t;

/**
 * @brief Current state of the modifier keys.
 * 
 * When modifier keys are pressed or released, a global
 * instance of this data structure should be modified.
 */
typedef struct kbd_mod_state {
    bool ctrl       :   1;
    bool alt        :   1;
    bool sft        :   1;
    bool capslock   :   1;
} kbd_mod_state_t;

/**
 * @brief Representation of the keyboard state (modifiers + key that was just pressed)
 * 
 */
typedef struct kbd_input {
    char raw;
    kbd_mod_state_t mod;
    char char_pressed;
} kbd_input_t;

/* Bitfield for each character written to the screen.
 * This is special so that we don't have to think too hard
 * when we do formatting. */
typedef struct vga_char {
    char codept         :   8;
    bool underln        :   1;
    uint8_t fg_color    :   2;
    bool fg_intensity   :   1;
    uint8_t bg_color    :   3;
    bool blink          :   1;
}__attribute__((packed)) vga_char_t;

typedef struct console_state {
    /// Is the console driver initialized?
    bool condrv_en;
    /// X location of virtual cursor
    int vcur_x;
    /// Y location of virtual cursor
    int vcur_y;
    /// Is the physical cursor visible at the virtual cursor location?
    bool pcur_en;
    /// Should the keyboard input be echoed to the screen?
    bool kbd_echo_en;
    /// Should the virtual cursor be auto-incremented on text writes?
    bool vcur_autoinc_en;
    /// Should the curor be allowed to wrap lines?
    bool con_wrap;
    /// What happens when we hit the bottom of the screen?  Do we scroll or wrap?
    bool con_scroll;
    /// Where should we stop allowing the user to backspace? (0 = no restriction)
    uint8_t con_bkspstop_col;
    /// Character style of newly written characters (code point is ignored)
    struct vga_char charstyle;
} console_state_t;

typedef struct console {
    /// Is this console available for use?
    bool present;
    /// ID of the console.  Used to determine which application belongs where.
    int id;
    /// Should the contents of this console be forwarded to video memory?
    bool current;
    /// State of the current console.  Not kept current while console is active, use the global instead.
    console_state_t constate;
    /// Buffered input object for this console
    buffered_input_t current_input;
    /// Contents of this console's buffered input buffer.
    char kbuf[KBUF_LEN];
    /// Contents of this console's screen buffer.  Perfect analog of the video memory.
    vga_char_t vid_buf[NUM_ROWS * NUM_COLS];
} console_t;

extern buffered_input_t current_input;
extern kbd_input_t kbd_state;
extern console_state_t constate;

extern char us_numtosym[10];
extern char us_symgroup1[3];

/* Function prototypes */
int console_init();
int console_getcursorpos(int * x, int * y);
int console_setcursorpos(int x, int y);
fastcall int console_putchar(char c);
fastcall int console_echo(char c);
fastcall void console_clrsc();
fastcall int console_puts(char * str, int n);
fastcall int console_putn(const char * buf, uint32_t n);
int console_getinput(buffered_input_t * bufin);
fastcall void console_inputhandler(char raw);
fastcall int console_echokbd(kbd_input_t kbdin);
int console_getchar(char * c);
int console_readline(char * cbuf, int n);
inline bool console_isinit();
fastcall void console_bksp();
int32_t console_readline_w(int32_t fd, void * buf, int32_t nbytes);
int32_t console_write_w(int32_t fd, const void * buf, int32_t nbytes);
int32_t console_close(int32_t fd);

#define MAX_VTERMS 3

extern console_t vterms[MAX_VTERMS];
typedef struct vconsole_override {
    int idx;
    bool flag;
} vconsole_override_t;

extern volatile vconsole_override_t con_ovr;

// Function prototypes
vga_char_t * vterm_get_buffer(void);
int vterm_init(void);
int vterm_new();
int vterm_swap_console(int new_idx);
fastcall void vterm_blit(void);
