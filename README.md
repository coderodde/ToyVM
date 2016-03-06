# ToyVM
A simple virtual machine written in C.

## Instuction set specification 
Just like Intel-based computers, ToyVM is little-endian.

### Registers
ToyVM features four 32-bit registers: **`REG1, REG2, REG3, REG4`**.

### Arithmetic 
* **`ADD`**: **`ADD REGi REGj`** - adds the integer in **`REGi`** to **`REGj`**.
* **`NEG`**: **`NEG REGi`** - negates the integer stored in **`REGi`**.
* **`MUL`**: **`MUL REGi REGj`** - copies the product of **`REGi`** and **`REGj`** to **`REGj`**.
* **`DIV`**: **`DIV REGi REGj`** - copies the ratio of **`REGi`** and **`REGj`** to **`REGj`**
* **`MOD`**: **`MOD REGi REGj`** - divides **`REGi`** by **`REGj`** and stores the remainder in **`REGj`**.

### Memory and data
* **`CONST`**: **`CONST REGi WORD`** - stores the value **`WORD`** in the register **`REGi`**.
* **`LOAD`**: **`LOAD REGi ADDRESS`** - stores the value at address **`ADDRESS`** to the register **`REGi`**.
* **`STORE`**: **`STORE REGi ADDRESS`** - stores the value in **`REGi`** in the memory at address **`ADDRESS`**.

### Conditionals

### Subroutines

### Stack

### Auxiliary
