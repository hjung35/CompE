/**
 * @file interrupts.c
 * @brief Routines for initializing and handling entries in the IDT.
 */

#include "interrupts.h"

/* Write some exception handlers.
 * Right now, these don't even read their error codes.
 * They just sort of print something out and that's it.
 */
DECLARE_ISR(divide_exception)
{
	printf("DIVIDE BY 0 EXCEPTION!\n");
	while(1);
}

DECLARE_ISR(nmi_handler)
{
	printf("Non-maskable interrupt!\n");
	while(1);
}

DECLARE_ISR(bound_exception)
{
	printf("BOUND EXCEPTION!\n");
	while(1);
}

DECLARE_ISR(invinstr_exception)
{
	printf("INVALID INSTRUCTION FAULT!\n");
	while(1);
}

DECLARE_ISR(nomath_exception)
{
	printf("NO FPU PRESENT FAULT! (how..?)\n");
	while(1);
}

DECLARE_ISR(df_exception)
{
	printf("DOUBLE FAULT!\n");
	while(1);
}

DECLARE_ISR(tss_exception)
{
	printf("TSS FAULT!\n");
	while(1);
}

DECLARE_ISR(noseg_exception)
{
	printf("NO SEGMENT FAULT!\n");
	while(1);
}

DECLARE_ISR(ss_exception)
{
	printf("STACK SEGMENT FAULT!\n");
	while(1);
}

DECLARE_ISR(gp_exception)
{
	printf("GENERAL PROTECTION FAULT!\n");
	while(1);
}

DECLARE_ISR(page_fault)
{
	int addr;
	cli();

	printf("PAGE FAULT!\n");
	asm volatile (
		"mov %%cr2, %%eax                       ;"
		: "=r" (addr)
		:  
		: "eax"                 
	); 
	printf("Page fault line: %x\n", addr);
	while(1);
}

DECLARE_ISR(unknown_fault)
{
	printf("UNKNOWN FAULT!\n");
	while(1);
}

DECLARE_ISR(assertion_fault)
{
	printf("ASSERTION FAILED!\n"); // It be safe to return from this one since it is just used in testing :)
}

DECLARE_ISR(fpu_exception)
{
	printf("FPU FAULT!\n");
	while(1);
}

DECLARE_ISR(unhandled_int)
{
	printf("UNHANDLED PIC INTERRUPT!\n");
	while(1);
}

/**
 * @brief Initialize the IDT with exceptions.
 * 
 * This function takes exception handlers (defined above)
 * and puts them into the IDT, before pointing IDTR at the
 * IDT in memory.
 * Faults that are marked as reserved in the range 1-10 
 * are handled by the unknown fault handler since
 * a) they're super unlikely to ever encounter and
 * b) they have to be defined for the test suite to pass.
 */
void init_idt(void)
{
	/* The memory for the IDT is already allocated.
	 * Just register all of the exceptions first, then
	 * enable the IDT (I think registering all of the 
	 * exceptions should be enough for the IDT not to
	 * hate us when we point something to it. )*/
	load_int(0, &divide_exception, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(1, &unknown_fault, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(2, &nmi_handler, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(3, &unknown_fault, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(4, &unknown_fault, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(5, &bound_exception, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(6, &invinstr_exception, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(7, &nomath_exception, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(8, &df_exception, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(9, &unknown_fault, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(10, &tss_exception, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(11, &noseg_exception, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(12, &ss_exception, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(13, &gp_exception, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(14, &page_fault, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(15, &assertion_fault, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(16, &fpu_exception, KERNEL_SEGMENT, INTERRUPT_DPL);
	/* Load the PIC interrupts all with something. */
	load_int(0x20, &unhandled_int, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(0x21, &unhandled_int, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(0x23, &unhandled_int, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(0x24, &unhandled_int, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(0x25, &unhandled_int, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(0x26, &unhandled_int, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(0x27, &unhandled_int, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(0x28, &unhandled_int, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(0x29, &unhandled_int, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(0x2A, &unhandled_int, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(0x2B, &unhandled_int, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(0x2C, &unhandled_int, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(0x2D, &unhandled_int, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(0x2E, &unhandled_int, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_int(0x2F, &unhandled_int, KERNEL_SEGMENT, INTERRUPT_DPL);
	load_trap(SYSCALL_INT, syscall_handler, KERNEL_SEGMENT, SYSCALL_DPL);
	/* Exceptions are loaded so we should be able to
	 * enable this as the interrupt table now.
	 * I think.
	 */
	idt_desc_ptr.size = NUM_VEC * sizeof(idt_desc_t) - 1;
	idt_desc_ptr.addr = (uint32_t)(&idt[0]);
	/*
	printf("Registering IDT with address 0x%x, size 0x%x\n", idt_desc_ptr.addr, idt_desc_ptr.size);
	printf("Size of desc ptr: 0x%x.\n Low bytes = 0x%x, \n High byte = 0x%x\n", sizeof(x86_desc_t), 
		(*(uint16_t*)&idt_desc_ptr),(*(unsigned long long*)&idt_desc_ptr) >> 16); */
	lidt(idt_desc_ptr);
	/* x86_desc_t current_idt;
	sidt(current_idt);
	printf("Current IDT value: address 0x%x, size 0x%x\n", current_idt.addr, current_idt.size);
	*/
}

/**
 * @brief Load an ISR into the interrupt table.
 * @param num Interrupt vector number
 * @param offset Address of the ISR
 * @param seg Segment of the ISR
 * @param dpl Descriptor privilege level of ISR.
 *
 * There's some stuff that this function does not initialize
 * in the descriptor because the struct def calls them "reserved"
 * even though according to the docs, they aren't.
 */
void load_int(uint16_t num, void * offset, uint16_t seg, uint16_t dpl)
{
	/* The IDT itself is already allocated, and it already exists. 
	 * Just chuck stuff into the table. */
	idt_desc_t idt_descriptor = { .st={
		.seg_selector = seg,
		.dpl = dpl,
		.present = 0,
		.type = IDT_DESCRIPTOR_TYPE,
		.s = 0,
		.zero = 0
	}};
	idt_descriptor.st.offset_15_00 = (uint32_t)offset & 0x0000FFFF;
	idt_descriptor.st.offset_31_16 = ((uint32_t)offset & 0xFFFF0000) >> 16;
	/* It might help something to not mark it as present until the
	 * whole descriptor is (ostensibly) copied into the table. */
	// printf("Registering interrupt 0x%x to address 0x%x: segment 0x%x type 0x%x dpl 0x%x\n",
	//		num, offset, idt_descriptor.st.seg_selector, idt_descriptor.st.type, idt_descriptor.st.dpl);
	idt[num] = idt_descriptor;
	idt[num].st.present = 1;
}

/**
 * @brief Load a trap vector into the interrupt table.
 * @param num Trap vector number
 * @param offset Address of the ISR
 * @param seg Segment of the ISR
 * @param dpl Descriptor privilege level of ISR.
 *
 * There's some stuff that this function does not initialize
 * in the descriptor because the struct def calls them "reserved"
 * even though according to the docs, they aren't.
 */
void load_trap(uint16_t num, void * offset, uint16_t seg, uint16_t dpl)
{
	/* The IDT itself is already allocated, and it already exists. 
	 * Just chuck stuff into the table. */
	idt_desc_t idt_descriptor = { .st={
		.seg_selector = seg,
		.dpl = dpl,
		.present = 0,
		.type = TRAP_DESCRIPTOR_TYPE,
		.s = 0,
		.zero = 0
	}};
	idt_descriptor.st.offset_15_00 = (uint32_t)offset & 0x0000FFFF;
	idt_descriptor.st.offset_31_16 = ((uint32_t)offset & 0xFFFF0000) >> 16;
	/* It might help something to not mark it as present until the
	 * whole descriptor is (ostensibly) copied into the table. */
	// printf("Registering interrupt 0x%x to address 0x%x: segment 0x%x type 0x%x dpl 0x%x\n",
	//		num, offset, idt_descriptor.st.seg_selector, idt_descriptor.st.type, idt_descriptor.st.dpl);
	idt[num] = idt_descriptor;
	idt[num].st.present = 1;
}
