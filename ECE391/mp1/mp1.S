
# mp1.S - life-or-death version
# Good luck, and have fun!

.data
	
    # Constants for accessing the fields of a struct keystroke_args
    # struct keystroke_args is defined in rtc.h and mp1.h

	KA_SIZE	  = 14   # 14 bytes when copying
	KA_STACK  = 16   # use 16 bytes on stack for alignment
	DIRECTION = 0
	SELECTOR  = 2
	GUESS     = 4
	HINT      = 9

    # Constants for accessing the fields of a struct game_status
    # struct game_status is defined in rtc.h and mp1.h

	GS_SIZE   = 8	# 8 bytes
	POP 	  = 0
	INF	  	  = 4

    # Constants for screen size

    SCR_WIDTH  = 80
	SCR_HEIGHT = 20
	SCR_SIZE   = ((SCR_WIDTH) * (SCR_HEIGHT))

    # ASCII cell images

    CELL_LIVE = 0x78
	CELL_DEAD = 0x20

	# MODIFIED by hjung35, MAGIC NUMBERS FOR mp1_ioctl_startgame
	INIT_AGGRESSION = 80
	INIT_POPULATION = 8000000


# Data shared between this file and rtc.c helper functions

# This '.globl' directive makes these symbols visible externally
.globl rand_seed, aggression

				.align 8
rand_seed:		.quad 0x0	# Pseudo-random number seed
aggression:		.long 0x0	# Aggression level of virus


# Data private to this file

current_board:	.long 0x0	# Address of current board
next_board:		.long 0x0	# Address of next board
population:		.long 0x0	# Remaining human population
infection:		.long 0x0	# Count of live virus cells
select_image:		.string "==>"	# Picture of the selector
select_blank:		.string "   "	# Picture to overwrite selector
base_array:             .string "ACGT"  # Array of characters for bases


.text

# void mp1_poke(void);
# You can use this function to write to video memory.
#
# Interface: Register-based arguments (not C-style)
#    Inputs: %cl  - The byte you wish to write
#            %eax - Offset from the start of video memory that you wish
#                   to write to
#   Outputs: Text-mode video screen is written to at location %eax with
#            the byte in %cl
# Registers: Clobbers EDX

mp1_poke:
		movl    vmem_base_addr(,1),%edx
		movb    %cl,(%edx,%eax,1)
		ret

#   ----------------- Exported functions ---------------------

# void mp1_rtc_tasklet(unsigned long ignore);
# Performs three tasks:
#	(1) updates the life game board and calculates infection count
#	(2) swaps the two boards
#	(3) applies population reduction based on infection count
#	(4) redraws the screen
#	(5) notifies the user code by calling mp1_notify_user
# Inputs   : none
# Outputs  : none
# Registers: Standard C calling convention

.globl mp1_rtc_tasklet
mp1_rtc_tasklet:
		 pushl %ebp
		 movl %esp, %ebp
		 pushl %ebx
		 pushl %esi
		 pushl %edi

# check the current_board whether it is 0 or not
mp1_rtc_tasklet_check_current_board: 
		 movl (current_board), %ebx  # &(*current_board) -> ebx
		 cmpl $0, %ebx # compare current_board whether it is equal to 0 or not
		 je   mp1_rtc_tasklet_done # if current_board == 0, return immediately


 # initialize the loop with height -> esi, width -> edi
		 xorl %esi, %esi # esi = i 
		 xorl %edi, %edi # edi = j
		 movl $SCR_HEIGHT, %esi
		 movl $SCR_WIDTH, %edi
# have to check all cells except 4 boundaries; 4 boundaries are row 0, 19, column 0, 79 
# 
update_board_loop:

	# check if i is reached 80, must be reached up until 79	
		 cmpl $SCR_WIDTH, %esi
		 je board_loop_done

	# check row number
		 cmpl $0, %esi
		 je  board_loop_inc



board_loop_inc:
		 incl %esi
		 jmp update_board_loop

board_loop_done:


calculate:

# mp1_notify_user


mp1_rtc_tasklet_done:
		 popl %edi
		 popl %esi
		 popl %ebx
		 leave
		ret


# do swap on boards and its contents
# mp1_swap_boards: 
# redraw the screen by calling draw_screen

# population 


# int mp1_ioctl(unsigned long arg, unsigned int cmd)
# The dispatch function for the MP1 ioctls - should use the cmd argument
# and a jumptable to execute one of the specific ioctls implemented below.
# Inputs   : unsigned long arg - parameter to the mp1_ioctl_....
#          : unsigned int cmd  - specifies which mp1_ioctl_... function 
#          :                     to execute
# Outputs  : Returns an integer - depends on which ioctl() function is called
# Registers: Standard C calling convention

.globl mp1_ioctl
mp1_ioctl:

		movl 8(%esp), %ebx # cmd -> ebx
		cmpl $4, %ebx  
		ja  mp1_ioctl_cmd_error
		jmp *jump_table(, %ebx, 4) 

mp1_ioctl_cmd_error:
		movl $-1, %eax

mp1_ioctl_done:
		ret

jump_table:
	.long mp1_ioctl_startgame, mp1_ioctl_endgame, mp1_ioctl_keystroke, mp1_ioctl_getstatus, mp1_ioctl_vaccinate


# ----------------- Functions private to this file -------------------


draw_screen:	
	# For simplicity, your implementation may assume a SCR_WIDTH of 80.
		ret

mp1_ioctl_startgame:

		 # callee-saved 
		 pushl %ebp
		 movl %esp, %ebp
		 pushl %ebx
		 pushl %esi
		 pushl %edi 

# random seed_generator by using seed, which was the argument passed in
		movl 4(%esp), %ebx
		pushl %ebx # push the argument number(a.k.a seed) to the stack
		call seed_generator # random number is now generated
		addl $4, %esp # delete the value we just passed in which is now no more in use

# two game board should be generated now

	# current board allocation
		pushl $SCR_SIZE # push the size of bytes that we have to allocate dynamically for boards
		call mp1_malloc
		cmpl $0, %eax  # if null pointer is returned, malloc failed, go to error step
		je mp1_ioctl_startgame_malloc_error
		movl %eax, current_board # current_board now has its address; holds onto the newly allocated memory for current_board

		movl (current_board), %ebx # &(*current_board) -> ebx
		movl $0, %ebx # fill in zeros into it

	# next_board allocation
		pushl $SCR_SIZE
		call mp1_malloc
		cmpl $0, %eax  
		je mp1_ioctl_startgame_malloc_error
		movl %eax, next_board # next_board holds onto the newly allocated memory for next_board

		movl (next_board), %edi
		movl $0, (%edi)

	# call init_virus to current_board
		pushl current_board
		call init_virus

	# set infection, agression and population			
		movl %eax, infection
		movl $INIT_AGGRESSION, aggression
		movl $INIT_POPULATION, population
		jmp mp1_ioctl_startgame_success

mp1_ioctl_startgame_malloc_error:
		pushl %eax # push the pointer of address that has been allocated just now, but with the error. we have to push this in before calling mp1_free
		call mp1_free # free the address pointer of dynamically allocated, nothing returns
		
mp1_ioctl_startgame_no_arg_error:
		movl $-1, %eax # error always contains -1 as a return value
		jmp mp1_ioctl_startgame_done
	
mp1_ioctl_startgame_success:
		movl $0, %eax
		
mp1_ioctl_startgame_done:
		popl %edi
		popl %esi
		popl %ebx
		leave
		ret


# int mp1_ioctl_endgame(unsigned long ignore)
#    Inputs: unsigned long ignore - treated as nothing comes in 
#   Outputs: NULL-ify both current_board and next_board ;returns 0 if succeeded 
# Registers: nothing gets changed 

mp1_ioctl_endgame:

		# callee-saved 
		pushl %ebp
		movl %esp, %ebp
		pushl %ebx
		pushl %esi
		pushl %edi 
		
mp1_ioctl_endgame_NULLify:
		movl current_board, %ebx
		pushl %ebx
		call mp1_free
		addl $4, %esp # popping what we had pushed == current_board address
	    movl $0x00, current_board # reset the board to back to factory state 
		
		movl next_board, %esi
		pushl %esi
		call mp1_free
		addl $4, %esp
		movl $0x00, next_board

mp1_ioctl_endgame_success:
		movl $0, %eax # return value which is eax that should be set to 0 in case of success

mp1_ioctl_endgame_done:		
		popl %edi
		popl %esi
		popl %ebx
		leave
		ret

mp1_ioctl_keystroke:
		ret



# int mp1_ioctl_getstatus(unsigned long* user_status)
#    Inputs: unsigned long* user_status - treated as nothing comes in 
#   Outputs: NULL-ify both current_board and next_board ;returns 0 if succeeded 
# Registers: nothing gets changed 
mp1_ioctl_getstatus:
		# pushl %ebp
		# movl %esp, %ebp
		# pushl %ebx
		# pushl %esi
		# pushl %edi

		# subl GS_SIZE, %ebp 


mp1_ioctl_getstatus_fail:
		# movl $-1, %eax
		# jmp mp1_ioctl_getstatus_done

mp1_ioctl_getstatus_success:
		# movl $0, %eax

mp1_ioctl_getstatus_done:
		# popl %edi
		# popl %esi
		# popl %ebx
		# leave
		ret

mp1_ioctl_vaccinate:
		ret

