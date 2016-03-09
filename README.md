# ToyVM
A simple virtual machine written in C.

## Instruction set specification 
Just like Intel-based computers, ToyVM is little-endian.

### Registers
ToyVM features four 32-bit registers: **`REG1, REG2, REG3, REG4`**. Each of them is represented as a single byte with values `0x0`, `0x1`, `0x2`, `0x3`, respectively.

### Data types
ToyVM has only one data type: 32-bit signed integers.

### Instruction format
The first byte is always an opcode specifying the instruction. It is immediately followed by arguments. If the instruction has both registers and immediate data as arguments, registers are specified before the immediate data.

### Arithmetic 
* **`0x01`**: **`ADD REGi REGj`** - adds the integer in **`REGi`** to **`REGj`**.
* **`0x02`**: **`NEG REGi`** - negates the integer stored in **`REGi`**.
* **`0x03`**: **`MUL REGi REGj`** - copies the product of **`REGi`** and **`REGj`** to **`REGj`**.
* **`0x04`**: **`DIV REGi REGj`** - copies the ratio of **`REGi`** and **`REGj`** to **`REGj`**
* **`0x05`**: **`MOD REGi REGj`** - divides **`REGi`** by **`REGj`** and stores the remainder in **`REGj`**.

### Conditionals
* **`0x10`**: **`CMP REGi REGj`** - compare the value in **`REGi`** to **`REGj`**.
* **`0x11`**: **`JA ADDRESS`** - jumps to address **`ADDRESS`** only if the **`vm.cpu.status`** has flag **`COMPARISON_ABOVE`** on.
* **`0x12`**: **`JE ADDRESS`** - jumps to address **`ADDRESS`** only if the **`vm.cpu.status`** has flag **`COMPARISON_EQUAL`** on.
* **`0x13`**: **`JB ADDRESS`** - jumps to address **`ADDRESS`** only if the **`vm.cpu.status`** has flag **`COMPARISON_BELOW`** on.
* **`0x14`**: **`JMP ADDRESS`** - jumps unconditionally to address **`ADDRESS`**.

#### Subroutines
* **`0x20`**: **`CALL ADDRESS`** - pushes the return address (i.e., **`ADDRESS + 5`**) into the stack and jumps to the instruction at the specified address. 
* **`0x21`**: **`RET`** - pops the return address and sets it to the program counter, hence returning from a subroutine.

### Memory and data
* **`0x30`**: **`LOAD REGi ADDRESS`** - stores the value at address **`ADDRESS`** to the register **`REGi`**.
* **`0x31`**: **`STORE REGi ADDRESS`** - stores the value in **`REGi`** in the memory at address **`ADDRESS`**.
* **`0x32`**: **`CONST REGi DATA`** - stores the value **`DATA`** in the register **`REGi`**.

### Auxiliary
* **`0x40`**: **`HALT`** - halts the virtual machine. 
* **`0x41`**: **`INT INTERRUPT_NUMBER`** - issues an interrupt with number **`INTERRUPT_NUMBER`**.
* **`0x42`**: **`NOP`** - a no-op instruction, does nothing else but increase the program counter towards the next instruction.

### Stack
* **`0x50`**: **`PUSH REGi`** - pushes the contents of register **`REGi`** to the stack.
* **`0x51`**: **`PUSH_ALL`** - pushes all four registers to the stack.
* **`0x52`**: **`POP REGi`** - pops the stack into register **`REGi`**.
* **`0x53`**: **`POP_ALL`** - pops all values of registers from the stack back to the registers.
* **`0x54`**: **`LSP REGi`** - loads the value of the stack pointer to the register **`REGi`**.
