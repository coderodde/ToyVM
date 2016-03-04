#include <stdio.h>
#include "toyvm.h"

int main(int argc, const char * argv[]) {
    TOYVM vm;
    INIT_VM(&vm, 1024);
    
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
    
    RUN_VM(&vm);
    
    printf("Hello, World! %u\n", vm.cpu.reg2);
    return 0;
}
