#include <stdio.h>
#include "toyvm.h"

int main(int argc, const char * argv[]) {
    TOYVM vm;
    InitializeVM(&vm, 1024, 512);
    
    // CONST REG4 100
    vm.memory[0] = CONST;
    vm.memory[1] = REG4;
    WriteWord(&vm, 2, 100);
    
    // CONST REG2 1
    vm.memory[6] = CONST;
    vm.memory[7] = REG2;
    WriteWord(&vm, 8, 1);
    
    // [12] ADD REG2 REG1
    vm.memory[12] = ADD;
    vm.memory[13] = REG2;
    vm.memory[14] = REG1;
    
    // CMP REG1 REG4
    vm.memory[15] = CMP;
    vm.memory[16] = REG1;
    vm.memory[17] = REG4;
    
    // JA 200
    vm.memory[18] = JA;
    WriteWord(&vm, 19, 200);
    
    // PUSH_ALL
    vm.memory[23] = PUSH_ALL;
    
    // CALL 50
    vm.memory[24] = CALL;
    WriteWord(&vm, 25, 50);
    
    // POP_ALL
    vm.memory[29] = POP_ALL;
    
    // JMP
    vm.memory[30] = JMP;
    WriteWord(&vm, 31, 12);
    
//[50] CONST REG2 15
    vm.memory[50] = CONST;
    vm.memory[51] = REG2;
    WriteWord(&vm, 52, 15);
    
    // MOD REG1 REG2
    vm.memory[56] = MOD;
    vm.memory[57] = REG1;
    vm.memory[58] = REG2;
    
    // CONST REG3 0
    vm.memory[59] = CONST;
    vm.memory[60] = REG3;
    WriteWord(&vm, 61, 0);
    
    // CMP REG2 REG3
    vm.memory[65] = CMP;
    vm.memory[66] = REG2;
    vm.memory[67] = REG3;
    
    // JB 93
    vm.memory[68] = JB;
    WriteWord(&vm, 69, 93);
    
    // JA 93
    vm.memory[73] = JA;
    WriteWord(&vm, 74, 93);
    
    // CONST REG3 300
    vm.memory[78] = CONST;
    vm.memory[79] = REG3;
    WriteWord(&vm, 80, 340);
    
    // PUHS REG3
    vm.memory[84] = PUSH;
    vm.memory[85] = REG3;
    
    // INT 2
    vm.memory[86] = INT;
    vm.memory[87] = INTERRUPT_PRINT_STRING;
    
    // JMP 193
    vm.memory[88] = JMP;
    WriteWord(&vm, 89, 193);
    
////////////////////////////
//[93] CONST REG2 5
    vm.memory[93] = CONST;
    vm.memory[94] = REG2;
    WriteWord(&vm, 95, 5);
    
    // MOD REG1 REG2
    vm.memory[99] = MOD;
    vm.memory[100] = REG1;
    vm.memory[101] = REG2;
    
    // CONST REG3 0
    vm.memory[102] = CONST;
    vm.memory[103] = REG3;
    WriteWord(&vm, 104, 0);
    
    // CMP REG2 REG3
    vm.memory[108] = CMP;
    vm.memory[109] = REG2;
    vm.memory[110] = REG3;
    
    // JB 136
    vm.memory[111] = JB;
    WriteWord(&vm, 112, 136);
    
    // JA 136
    vm.memory[116] = JA;
    WriteWord(&vm, 117, 136);
    
    // CONST REG3 320
    vm.memory[121] = CONST;
    vm.memory[122] = REG3;
    WriteWord(&vm, 123, 320);
    
    // PUHS REG3
    vm.memory[127] = PUSH;
    vm.memory[128] = REG3;
    
    // INT 2
    vm.memory[129] = INT;
    vm.memory[130] = 2;
    
    // JMP 193
    vm.memory[131] = JMP;
    WriteWord(&vm, 132, 193);
    
//////////////////////////
//[136] CONST REG2 3
    vm.memory[136] = CONST;
    vm.memory[137] = REG2;
    WriteWord(&vm, 138, 3);
    
    // MOD REG1 REG2
    vm.memory[142] = MOD;
    vm.memory[143] = REG1;
    vm.memory[144] = REG2;
    
    // CONST REG3 0
    vm.memory[145] = CONST;
    vm.memory[146] = REG3;
    WriteWord(&vm, 147, 0);
    
    // CMP REG2 REG3
    vm.memory[151] = CMP;
    vm.memory[152] = REG2;
    vm.memory[153] = REG3;
    
    // JB 179
    vm.memory[154] = JB;
    WriteWord(&vm, 155, 179);
    
    // JA 179
    vm.memory[159] = JA;
    WriteWord(&vm, 160, 179);
    
    // CONST REG3 340
    vm.memory[164] = CONST;
    vm.memory[165] = REG3;
    WriteWord(&vm, 166, 300);
    
    // PUHS REG3
    vm.memory[170] = PUSH;
    vm.memory[171] = REG3;
    
    // INT 2
    vm.memory[172] = INT;
    vm.memory[173] = 2;
    
    // JMP 193
    vm.memory[174] = JMP;
    WriteWord(&vm, 175, 193);
    ////
    
//[179]: PUSH REG1
    vm.memory[179] = PUSH;
    vm.memory[180] = REG1;
    
    // INT 1
    vm.memory[181] = INT;
    vm.memory[182] = INTERRUPT_PRINT_INTEGER;
    
    // CONST REG4 350
    vm.memory[183] = CONST;
    vm.memory[184] = REG4;
    WriteWord(&vm, 185, 350);
    
    // PUSH REG4
    vm.memory[189] = PUSH;
    vm.memory[190] = REG4;
    
    // INT INTEGER
    vm.memory[191] = INT;
    vm.memory[192] = INTERRUPT_PRINT_STRING;
    
//[193]:
    vm.memory[193] = RET;
    vm.memory[200] = HALT;
    
    memcpy(&vm.memory[300], "Fizz\n", strlen("Fizz\n"));
    memcpy(&vm.memory[320], "Buzz\n", strlen("Buzz\n"));
    memcpy(&vm.memory[340], "FizzBuzz\n", strlen("FizzBuzz\n"));
    memcpy(&vm.memory[350], "\n", strlen("\n"));
    
    RunVM(&vm);
    
    puts("\n--- MACHINE STATUS ---");
    PrintStatus(&vm);
    return 0;
}
