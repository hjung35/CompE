/** 
 * @file interrupts.h
 * @brief Data structures &tc. for handling the IDT, beyond what's in x86-desc.h
 */

#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "syscall.h"

#define IDT_SIZE 8*127
#define KERNEL_SEGMENT 0x0010 // This is probably right.
#define INTERRUPT_DPL 0 // I think this value is actually right
#define IDT_DESCRIPTOR_TYPE 0xE
#define TRAP_DESCRIPTOR_TYPE 0xF

/* Interrupt values: where's the keyboard?
 * These are as the CPU sees them, not as the PICs see them.
 */
#define KBD_INT_NUM 0x21 // TODO: Verify this IRQ 1
#define RTC_INT_NUM 0x28 // TODO: Verify this IRQ 8
#define PIT_INT_NUM 0x20 // IRQ0

/* The following definitions allow us to write interrupt handlers in C,
 * since the version of GCC the class uses is so fing old that it doesn't
 * support the interrupt attribute.  So we have to wrap everything.
 */
/* DECLARE_ISR is a shortcut macro for declaring an ISR and also an assembly
 * wrapper for said function.  We can't have normal functions be ISRs because
 * they don't return right, and they don't save all of the registers.
 * Instead, we have to wrap it in some assembly.  This macro should take care of
 * all of that for us.
 * HOW TO USE:
 * 1. Come up with a fitting ISR name.
 * 2. Write the following:
 * 		```
 *		DECLARE_ISR(my_isr_name)
 *		{
 *			... interrupt service ...
 *		}
 *		```
 * When you need to get a pointer to this, you should be able to use
 * &my_isr_name.  Put it in the header here as void isr_name(void).
 * None of this has been proven yet, so don't shoot me.
 */
#define DECLARE_ISR(isr_name)			\
extern void isr_name(void);				\
void isr_name##_handler(void);			\
asm (									\
	".global "#isr_name"\n"				\
	".align 4\n"							\
	#isr_name":\n\t"					\
	"pusha\n\t"							\
	"cld\n\t"							\
	"call " #isr_name "_handler \n\t"	\
	"popa\n\t"							\
	"iret");							\
void isr_name##_handler(void)

/* Exception handlers */
void divide_exception(void);
void nmi_handler(void);
void bound_exception(void);
void invinstr_exception(void);
void nomath_exception(void);
void df_exception(void);
void tss_exception(void);
void noseg_exception(void);
void ss_exception(void);
void gp_exception(void);
void page_fault(void);
void fpu_exception(void);
/* Not really sure if anything below here is really needed. */
void ac_exception(void);
void xf_exception(void);
/* Interrupt handlers */
void kbd_int(void);
void rtc_int(void);
void pit_interrupt(void);
/* Main functions */
void init_idt(void);
void load_int(uint16_t, void *, uint16_t, uint16_t);
void load_trap(uint16_t, void *, uint16_t, uint16_t);
