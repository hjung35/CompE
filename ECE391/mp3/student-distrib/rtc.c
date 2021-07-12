/* RTC device  
 * Author:	    Hyun Do Jung(MJ)
 * Version:	    1
 * Creation Date:   Fri Oct 18 17:05:17 2019
 * Filename:	    rtc.c
 * History:
 *	MJ	1	Fri Oct 18 16:43:17 2019
 *		First written.
 *  MJ  2   Wed Oct 23 21:00:13 2019
 *      basic File system fucntions are added.
 */

#include "rtc.h"
#include "lib.h"
#include "i8259.h"
#include "interrupts.h"

/*
 * init_rtc
 *   DESCRIPTION: initializes rtc driver 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: initializes rtc driver accordingly
 *   REFERRENCE: 1. https://wiki.osdev.org/RTC
 */   
void init_rtc(void){
    /* disable interrupts */
    disable_irq(RTC_IRQ_NUMBER);

    /* following as the reference says so */
    outb(RTC_NMI_DISABLED_REG_B, RTC_REG_PORT);
    char prev_data = inb(RTC_DATA_PORT);
    outb(RTC_NMI_DISABLED_REG_B, RTC_REG_PORT);
    outb( (prev_data | BIT_SIX_BITMASK) ,RTC_DATA_PORT);
    load_int(RTC_IRQ_NUMBER, &rtc_interrupt, KERNEL_SEGMENT, INTERRUPT_DPL);

    /* enable interrupts */
    enable_irq(RTC_IRQ_NUMBER);
}

/*
 * rtc_interrupt_handler
 *   DESCRIPTION: helper function which handles rtc_interrupt / mp3.2 added 'CLEARING' interrupt flag for rtc_read
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: helps when the clock changes / interrupts generated, 
 *                 handles the rtc to set the clock accordingly
 *                 !!Important!! if Register C is not read after an IRQ8, the interrupt will not happen again.
 *   REFERRENCE: 1. https://wiki.osdev.org/RTC
 */   
DECLARE_ISR(rtc_interrupt) {
    /* critical section started */
    cli();
    
    /* following as the reference says so */
    outb(RTC_STATUS_REG_C, RTC_REG_PORT);
    inb(RTC_DATA_PORT);
    
    /* make sure we have to acknowledge that our interrupt is done by doing this */
    send_eoi(RTC_IRQ_NUMBER);
    
    /* clear the rtc_interupt_flag */
    rtc_interrupt_flag = 0;

    /* critical section ended*/
    sti();
}

/*
 * rtc_open
 *   DESCRIPTION: helper function which handles file system OPEN
 *   INPUTS: filename -- sepcific file name we want to open
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on sucess
 *   SIDE EFFECTS: RTC intterrupt raet should be set to a default value of 2 Hz when the RTC device is opened.
 *                 Thus, RTC intterrupts should remain on at all times.              
 */   
int32_t rtc_open (const uint8_t* filename){
    int fdnum;
    filedesc_t * fdptr = get_free_fd(&fdnum);
    if (!fdptr) return -1;
    fdptr->flags.in_use = true;
    fdptr->optbl = &rtc_optbl;
    /* when the rtc device is opened, default value of 2 Hz */
    rtc_set_frequency(2);
    return fdnum;
}

/*
 * rtc_close
 *   DESCRIPTION: helper function which handles file system CLOSE
 *   INPUTS: fd -- file descriptor that is supposed to be closed
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on sucess
 *   SIDE EFFECTS: actually do nothing -- reference: piazza post     
 */   
int32_t rtc_close (int32_t fd){
    return 0;
}

/*
 * rtc_read
 *   DESCRIPTION: helper function which handles file system READ; this actually does not read RTC frequnecy.
 *                It  should block until next inerrupt, and return 0.
 *   INPUTS: fd-- file descripter, buf -- pointer to the the buffer, nbytes -- number of bytes to be written 
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on sucess
 *   SIDE EFFECTS: sets interrupt flag to 0 and interrupt hanlder clears it after interrupt is handled        
 */  
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes){
    rtc_interrupt_flag = 1; // lock taken. just think as "toilet" spin lock
    
    while(rtc_interrupt_flag){        
    
    }
    
    return 0;
}

/*
 * rtc_write
 *   DESCRIPTION: helper function which handles file system WRITE; 
 *                It should be able to change the frequency. freuqncy must be power of 2
 *   INPUTS: fd-- file descripter, buf -- pointer to the the buffer, nbytes -- number of bytes to be written 
 *   OUTPUTS: number of bytes written
 *   RETURN VALUE: 0 on sucess, -1 on error
 *   SIDE EFFECTS: change the RTC frequency, which should be in power of 2.       
 */  
int32_t rtc_write (int32_t fd, const void* buf, int32_t nbytes){

    /*  Sanity Check: NULL pointer buf or number of bytes is not 4, return error */
    if(buf == NULL || nbytes != 4) 
        return -1;

    int32_t target_frequency; // local variable 

    /* type casting needed. this is because we get the target frequency in the buf but this buff is void pointer. 
      * we have ot make sure we cast it to int32_t* pointer and then dereference it into the actual int32_t value 
      */
    target_frequency = *(int32_t*) buf;

    /* when we change the rtc frequency, when we change the clock frequency, short critical section needed */
    cli();
    if (rtc_set_frequency(target_frequency)) nbytes = -1;
    sti();

    return nbytes;
}



/*
 * rtc_set_freq
 *   DESCRIPTION: helper function to set the rtc frequency of specific number inputted
 *   INPUTS: integer value that is a target rtc frequncy 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: set RTC frequency as we want    
 *   REFERRENCE : 1. https://courses.engr.illinois.edu/ece391/secure/references/mc1461818.pdf  old version
 *                2. https://wiki.osdev.org/RTC
 */   
int rtc_set_frequency(int32_t frequency){

    /* local variables */
    char freq, prev_data;
    int retval = 0;
    /* Values are defined in the datasheet(OLD version) */
    switch(frequency){
        case 8192:
        case 4096:
        case 2048:
            return 0;
        case 1024:
            freq = TEN_TWENTY_FOUR;
            break; 
        case 512: 
            freq = FIVE_TWELVE;
            break;
        case 256:
            freq = TWO_FIFTY_SIX;
            break;
        case 128:
            freq = ONE_TWENTY_EIGHT;
            break;
        case 64:
            freq = SIXTY_FOUR;
            break;
        case 32:
            freq = THIRTY_TWO;
            break;
        case 16:
            freq = SIXTEEN;
            break;
        case 8:
            freq = EIGHT;
            break;
        case 4:
            freq = FOUR;
            break;
        case 2:
            freq = TWO;
            break;
        default:
            return 1;
    }

    /* referring to OSDEV, disable irq */
    disable_irq(RTC_IRQ_NUMBER);

    /* get initial vlaue of register A */
    outb(RTC_NMI_DISABLED_REG_A, RTC_REG_PORT);
    prev_data = inb(RTC_DATA_PORT);
    /* SET the frequency by set correct RS values of Register A; 
      * reset index to A and write only our frequncy to A, frequency is the bottom 4 bits*/
    outb(RTC_NMI_DISABLED_REG_A, RTC_REG_PORT);
    outb( (prev_data & RS_MASK) | freq, RTC_DATA_PORT);

    /* enable irq */
    enable_irq(RTC_IRQ_NUMBER);
    return retval;
}
