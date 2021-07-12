/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void i8259_init(void) {
    /* Start the initialization. */
    // Disable interrupts while we are still initializing
    unsigned long eflags;
    cli_and_save(eflags);
    outb(ICW1, MASTER_8259_PORT);
    io_wait();
    outb(ICW1, SLAVE_8259_PORT);
    io_wait();
    outb(ICW2_MASTER, MASTER_8259_DATAPORT);
    io_wait();
    outb(ICW2_SLAVE, SLAVE_8259_DATAPORT);
    io_wait();
    outb(ICW3_MASTER, MASTER_8259_DATAPORT);
    io_wait();
    outb(ICW3_SLAVE, SLAVE_8259_DATAPORT);
    io_wait();
    outb(ICW4, MASTER_8259_DATAPORT);
    io_wait();
    outb(ICW4, SLAVE_8259_DATAPORT);
    io_wait();
    /* Each PIC has recvd all of the command words it was expecting.  The next writes write to the mask.
     * We want to initially mask all of the PIC's interrupt lines. */
    master_mask = 0xFB; // Don't mask the entire slave PIC.
    slave_mask = 0xFF; // This one has no slaves so mask the whole thing.
    outb(master_mask, MASTER_8259_DATAPORT);
    io_wait();
    outb(slave_mask, SLAVE_8259_DATAPORT);
    // Re-enable interrupts since we've masked them on the PIC.
    restore_flags(eflags);
}

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {
    // Hope that no command has been issued that we don't know about
    if (irq_num < SLAVE_IRQOFF) {
        // This is on the master PIC.
        uint8_t demask = ~(0x01 << (irq_num - MASTER_IRQOFF));
        master_mask &= demask;
        outb(master_mask, MASTER_8259_DATAPORT);
    } else {
        // This is on the slave PIC.
        uint8_t demask = ~(0x01 << (irq_num - SLAVE_IRQOFF));
        slave_mask &= demask;
        outb(slave_mask, SLAVE_8259_DATAPORT);
    }
    io_wait();
}

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
    // Hope that no command has been issued that we don't know about
    if (irq_num < SLAVE_IRQOFF) {
        // This is on the master PIC.
        uint8_t mask = (0x01 << (irq_num - MASTER_IRQOFF));
        master_mask |= mask;
        outb(master_mask, MASTER_8259_DATAPORT);
    } else {
        // This is on the slave PIC.
        uint8_t mask = (0x01 << (irq_num - SLAVE_IRQOFF));
        slave_mask |= mask;
        outb(slave_mask, SLAVE_8259_DATAPORT);
    }
    io_wait();
}

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {
    // TODO: Catch spurious interrupts.
    // Form the EOI
    // uint8_t eoi_msg = EOI | irq_num;
    uint8_t eoi_msg = EOI_GENERIC; // Other one didn't seem to work too well.
    // Determine which PIC (or rather, one or both) gets the EOI.
    outb(eoi_msg, MASTER_8259_PORT);
    if (irq_num >= SLAVE_IRQOFF) {
        // INT was from slave PIC, so send a copy to the slave.
        outb(eoi_msg, SLAVE_8259_PORT);
    }
}

/**
 * @brief Get the master mask (so it's RO)
 * 
 * @return uint8_t the master mask
 */
uint8_t get_master_mask()
{
    return master_mask;
}

/**
 * @brief Get the slave mask (so it's RO)
 * 
 * @return uint8_t the slave mask
 */
uint8_t get_slave_mask()
{
    return slave_mask;
}
