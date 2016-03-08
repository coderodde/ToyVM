#include <stdio.h>
#include "toyvm.h"

int main(int argc, const char * argv[]) {
    TOYVM vm;
    INIT_VM(&vm, 1024, 1008);
    
    /*
    // Load 100 to reg1.
    vm.memory[0] = CONST;
    vm.memory[1] = REG1;
    WRITE_WORD(&vm, 2, 100);
    
    // Load 23 to reg2.
    vm.memory[6] = CONST;
    vm.memory[7] = REG2;
    WRITE_WORD(&vm, 8, 23);
    
    // Add reg1 to reg2.
    vm.memory[12] = ADD;
    vm.memory[13] = REG1;
    vm.memory[14] = REG2;
    vm.memory[15] = HALT;
     */
    
    /*
    vm.memory[0] = CONST;
    vm.memory[1] = REG3;
    WRITE_WORD(&vm, 2, 234);
    vm.memory[6] = PUSH_ALL;
    vm.memory[7] = CONST;
    vm.memory[8] = REG3;
    WRITE_WORD(&vm, 9, -100);
    vm.memory[13] = POP_ALL;
    vm.memory[14] = HALT;*/
    
    /*
    vm.memory[0] = CONST;
    vm.memory[1] = REG1;
    WRITE_WORD(&vm, 2, 1001);
    vm.memory[6] = CONST;
    vm.memory[7] = REG2;
    WRITE_WORD(&vm, 8, 1002);
    vm.memory[12] = CONST;
    vm.memory[13] = REG3;
    WRITE_WORD(&vm, 14, 1003);
    vm.memory[18] = CONST;
    vm.memory[19] = REG4;
    WRITE_WORD(&vm, 20, 3000);
    vm.memory[24] = MUL;
    vm.memory[25] = REG1;
    vm.memory[26] = REG2;
    vm.memory[27] = DIV;
    vm.memory[28] = REG1;
    vm.memory[29] = REG3;
    vm.memory[30] = MOD;
    vm.memory[31] = REG1;
    vm.memory[32] = REG4;*/
    
/*    vm.memory[0] = CONST;
    vm.memory[1] = REG3;
    WRITE_WORD(&vm, 2, 12345678);
    vm.memory[6] = PUSH;
    vm.memory[7] = REG3;
    vm.memory[8] = INT;
    vm.memory[9] = INTERRUPT_PRINT_INTEGER;*/
    
    vm.memory[0] = CONST;
    vm.memory[1] = REG1;
    WRITE_WORD(&vm, 2, 10);
    vm.memory[6] = PUSH;
    vm.memory[7] = REG1;
    vm.memory[8] = INT;
    vm.memory[9] = INTERRUPT_PRINT_STRING;
    memcpy(&vm.memory[10], "Funky yeah!", 11);
    
    RUN_VM(&vm);
    
/*    printf("REG1: %u\n", vm.cpu.registers[REG1]);
    printf("REG2: %u\n", vm.cpu.registers[REG2]);
    printf("REG3: %u\n", vm.cpu.registers[REG3]);
    printf("REG4: %u\n", vm.cpu.registers[REG4]);*/
    return 0;
}
