#include <stdio.h>
#include "toyvm.h"

int main(int argc, const char * argv[]) {
    TOYVM vm;
    INIT_VM(&vm, 1024, 512);
    
    /* REG4 holds the maximum allowed loop counter value. */
    vm.memory[0] = CONST;
    vm.memory[1] = REG4;
    WRITE_WORD(&vm, 2, 100);
    
    /* REG2 holds the value one (1) for incrementing the value of loop 
       couter. */
    vm.memory[6] = CONST;
    vm.memory[7] = REG2;
    WRITE_WORD(&vm, 8, 1);
    
    /* Increment REG1 by one. */
    vm.memory[12] = ADD;
    vm.memory[13] = REG2;
    vm.memory[14] = REG1;
    
    /* Compare the value of REG1 and REG4. */
    vm.memory[15] = CMP;
    vm.memory[16] = REG1;
    vm.memory[17] = REG4;
    /* If done, jump to a HALT instruction. */
    vm.memory[18] = JA;
    WRITE_WORD(&vm, 19, 100);
    
    /* Increment REG3. */
    vm.memory[23] = ADD;
    vm.memory[24] = REG2;
    vm.memory[25] = REG3;
    vm.memory[26] = JMP;
    WRITE_WORD(&vm, 27, 12);
    
    vm.memory[100] = HALT;
    RUN_VM(&vm);
    
    printf("REG3: %d.\n", vm.cpu.registers[REG3]);
    PRINT_STATUS(&vm);
    return 0;
}
