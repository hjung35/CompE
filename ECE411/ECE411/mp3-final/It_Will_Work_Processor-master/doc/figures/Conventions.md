# Naming Conventions

## Registers
Primary registers should always be assigned a name based on the RISC-V documentation followed by `'_reg'`. For example, the module instance for the PC register should be named `PC_reg`.

Input registers will use the module instance name followed by `_in` and output signals will use `_out` as the suffix. For example, the PC input signal will be called `PC_reg_in` and the output signal will be called `PC_reg_out`.

## Stage Registers
For the registers that pass down values along the different stages, we will use many instances of the same module. This provides more uniformity throughout the different stages. All registers within this module will use a single load signal, since all values are passed down to the next stage when pipelining.
