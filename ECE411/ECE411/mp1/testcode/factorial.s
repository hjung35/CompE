# Factorial Program 
# Input is an integer value that should be set in the input variable(in the label in the rodata section)
# it iterates the addition step and gives the correct answer in the result label

# Register Usage Table
# x0 -- predefined NULL, x0 == 0. No change, but using it as it is.
# x1 -- input value 'n'. we are finding this value's factorial with this assembly code (I set it as 4 here)
# x2 -- loop1 counter(inner)
# x3 -- loop2 counter(outer)
# x4 -- the current value of result 
# x5 -- calculating result; middle stage result that will temporarily hold the result value of loops 
# x6 -- holds the address of the label 'result' in rodata section. 

factorial.s:
.align 4
.section .text
.globl _start

_start:
    lw  x1, input            # x1 <= input 
    la  x6, result           # x6 <= Addr[result]
    sw  x1, 0(x6)            # [result] <= input 
    lw  x3, result           # x3 <= result ; 
    addi x3, x3, -3          # x3 <= x3 - 1 ; set loop2 counter(outer)

loop2:
    addi x2, x3, 0           # x2 <= x3 + 0 ; set loop1 counter(inner)
    lw  x4, result           # x4 <= result
    addi x5, x4, 0           # x5 <= x4 + 0 
    blt x2, x0, loop_done    # if x2 < x0 then goto loop_done, else continue 

loop1:
    add x5, x5, x4           # X5 <= X5 + X4
    addi x2, x2, -1          # x2 <= x2 - 1 ; decrement loop1 counter(inner)
    bge x2, x0, loop1        # if x2 >= x0 then goto loop1, else continue (if x2 is smaller than x0, continue)

loop_done:
    sw x5, 0(x6)             # [result] <= x5
    addi x3, x3, -1          # x3 <= x3 - 1 ; decrement loop2 counter(outer)
    bge x3, x0, loop2        # if x3 >= x0 then goto loop2, else continue (if x3 is smaller than x0, continue)

# below is referrenced from riscv_mp1test.s, the given file inside /mp1/testcode
halt:                 # Infinite loop to keep the processor
    beq x0, x0, halt  # from trying to execute the data below.
                      # Your own programs should also make use
                      # of an infinite loop at the end.

deadend:
    lw x8, bad     # X8 <= 0xdeadbeef
deadloop:
    beq x8, x8, deadloop

.section .rodata

input:      .word 0x00000004
bad:        .word 0xdeadbeef
result:     .word 0x00000000
