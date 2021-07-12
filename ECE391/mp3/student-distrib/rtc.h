/* RTC device  
 * Author:	    Hyun Do Jung(MJ)
 * Version:	    1
 * Creation Date:   Fri Oct 18 16:43:17 2019
 * Filename:	    rtc.h
 * History:
 *	MJ	1	Fri Oct 18 16:43:17 2019
 *		First written.
 *  MJ  2   Fri Oct 26 19:49:20 2019
 *      MP3.2 file system basic functions(read, write, open close)
 */


/* MP 3.1
 * Reference: OSDev RTC website 
 * 0x70 : specify an index or "register number"
 * 0x71 : Read / write from or to that byte of CMOS configuration space
 * 
 */
#include "types.h"

#define RTC_IRQ_NUMBER		        0x28 // 0x20 should be for pit interrupt vector

#define	RTC_REG_PORT	            0x70
#define RTC_DATA_PORT	            0x71  

#define RTC_NMI_DISABLED_REG_A      0x8A 
#define RTC_NMI_DISABLED_REG_B      0x8B 
#define RTC_NMI_DISABLED_REG_C      0x8A 

#define RTC_STATUS_REG_A	        0X0A
#define RTC_STATUS_REG_B	        0x0B
#define RTC_STATUS_REG_C	        0x0C

#define BIT_SIX_BITMASK             0x40 

/* MP 3.2
 * BIT representation of setting Reg A for rtc_set_frequency 
 */

#define TEN_TWENTY_FOUR             0x06
#define FIVE_TWELVE                 0x07
#define TWO_FIFTY_SIX               0x08
#define ONE_TWENTY_EIGHT            0x09
#define SIXTY_FOUR                  0x0A
#define THIRTY_TWO                  0x0B
#define SIXTEEN                     0x0C
#define EIGHT                       0x0D
#define FOUR                        0x0E
#define TWO                         0x0F

#define RS_MASK                     0xF0

/* MP3.1 initialize rtc device */
void init_rtc(void);

/* MP3.1 rtc helper function; rtc_interrupt handler on clock */
void rtc_interrupt(void);
void rtc_interrupt_handler(void);

/* MP3.2 simple volatile flag used in tc_read to prevent any ohter interrupts happen during it */
volatile int rtc_interrupt_flag;

/* MP3.2 RTC basic file system functions*/
int32_t rtc_open (const uint8_t* filename); 

int32_t rtc_close (int32_t fd);

/* CAUTION!!! Mike(me) has some questions related to RTC. below comments are my notes/questions

looking into open and close functions right now, that we dont actually open and close any related rrc fiels. 
instead we just open the rtc driver and close it.

few questions are elft i just put them in here so i dont forget to ask these questions later and solve it
1. rtc_open close do we need input values just like given prototypes?
2. rtc close probably doesnt do anything but does that mean do we reset the rtc to default frequency? or not?
3. when we set the rtc_clock do we consider it as interrupt? since rtc interrupt is being considered to be continuous, do we have to use
critical section for it?
4. rtc_write. Do we need to use short cirtical secttino for chaging freq?
5. how do we virtualize the rtc so as the document mentioned for better future usage. 
*/

int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);

int32_t rtc_write (int32_t fd, const void* buf, int32_t nbytes);



/* MP3.2 helper function to set the rtc frequency att specific frequency input */
int rtc_set_frequency(int32_t frequency);







