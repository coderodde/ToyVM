#include "toyvm.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct instruction {
    uint8_t   opcode;
    size_t    size;
    bool    (*execute)(TOYVM*);
} instruction;

/*******************************************************************************
* Return 'true' if the stack is empty.                                         *
*******************************************************************************/
static bool StackIsEmpty(TOYVM* vm)
{
    return vm->cpu.stack_pointer >= vm->memory_size;
}

/*******************************************************************************
* Return 'true' if the stack is full.                                          *
*******************************************************************************/
static bool StackIsFull(TOYVM* vm)
{
    return vm->cpu.stack_pointer <= vm->stack_limit;
}

/*******************************************************************************
* Returns the amount of free space in the stack in bytes.                      *
*******************************************************************************/
static int32_t GetAvailableStackSize(TOYVM* vm)
{
    return vm->cpu.stack_pointer - vm->stack_limit;
}

/*******************************************************************************
* Returns the number of bytes occupied by the stack.                           *
*******************************************************************************/
static int32_t GetOccupiedStackSize(TOYVM* vm)
{
    return vm->memory_size - vm->cpu.stack_pointer;
}

/*******************************************************************************
* Returns 'true' if the stack has enough room for pushing all registers to it. *
*******************************************************************************/
static bool CanPerformMultipush(TOYVM* vm)
{
    return GetAvailableStackSize(vm) >= sizeof(int32_t) * N_REGISTERS;
}

/*******************************************************************************
* Returns 'true' if the stack can provide data for all registers.              *
*******************************************************************************/
static bool CanPerformMultipop(TOYVM* vm)
{
    return GetOccupiedStackSize(vm) >= sizeof(int32_t) * N_REGISTERS;
}

/*******************************************************************************
* Returns 'true' if the instructoin does not run over the memory.              *
*******************************************************************************/
static bool InstructionFitsInMemory(TOYVM* vm, uint8_t opcode);

/*******************************************************************************
* Returns the length of the instruction with opcode 'opcode'.                  *
*******************************************************************************/
static size_t GetInstructionLength(TOYVM* vm, uint8_t opcode);

void InitializeVM(TOYVM* vm, int32_t memory_size, int32_t stack_limit)
{
    /* Make sure both 'memory_size' and 'stack_limit' are divisible by 4. */
    memory_size
    += sizeof(int32_t) - (memory_size % sizeof(int32_t));
    
    stack_limit
    += sizeof(int32_t) - (stack_limit % sizeof(int32_t));
    
    vm->memory              = calloc(memory_size, sizeof(uint8_t));
    vm->memory_size         = memory_size;
    vm->stack_limit         = stack_limit;
    vm->cpu.program_counter = 0;
    vm->cpu.stack_pointer   = (int32_t) memory_size;
    
    /***************************************************************************
    * Zero out all status flags.                                               *
    ***************************************************************************/
    vm->cpu.status.BAD_ACCESS       = 0;
    vm->cpu.status.COMPARISON_ABOVE = 0;
    vm->cpu.status.COMPARISON_EQUAL = 0;
    vm->cpu.status.COMPARISON_BELOW = 0;
    
    vm->cpu.status.BAD_INSTRUCTION        = 0;
    vm->cpu.status.INVALID_REGISTER_INDEX = 0;
    vm->cpu.status.STACK_OVERFLOW         = 0;
    vm->cpu.status.STACK_UNDERFLOW        = 0;
    
    /***************************************************************************
    * Zero out the registers and the map mapping opcodes to their respective   *
    * instruction descriptors.                                                 *
    ***************************************************************************/
    memset(vm->cpu.registers, 0, sizeof(int32_t) * N_REGISTERS);
    memset(vm->opcode_map, 0, sizeof(vm->opcode_map));
    
    /***************************************************************************
    * Build the opcode map.                                                    *
    ***************************************************************************/
    vm->opcode_map[ADD] = 1;
    vm->opcode_map[NEG] = 2;
    vm->opcode_map[MUL] = 3;
    vm->opcode_map[DIV] = 4;
    vm->opcode_map[MOD] = 5;
    
    vm->opcode_map[CMP] = 6;
    vm->opcode_map[JA]  = 7;
    vm->opcode_map[JE]  = 8;
    vm->opcode_map[JB]  = 9;
    vm->opcode_map[JMP] = 10;
    
    vm->opcode_map[CALL] = 11;
    vm->opcode_map[RET]  = 12;
    
    vm->opcode_map[LOAD]  = 13;
    vm->opcode_map[STORE] = 14;
    vm->opcode_map[CONST] = 15;
    
    vm->opcode_map[HALT] = 16;
    vm->opcode_map[INT]  = 17;
    vm->opcode_map[NOP]  = 18;
    
    vm->opcode_map[PUSH]     = 19;
    vm->opcode_map[PUSH_ALL] = 20;
    vm->opcode_map[POP]      = 21;
    vm->opcode_map[POP_ALL]  = 22;
    vm->opcode_map[LSP]      = 23;
    
}

void WriteVMMemory(TOYVM* vm, uint8_t* mem, size_t size)
{
    memcpy(mem, vm->memory, size);
}

static int32_t ReadWord(TOYVM* vm, int32_t address)
{
    uint8_t b1 = vm->memory[address];
    uint8_t b2 = vm->memory[address + 1];
    uint8_t b3 = vm->memory[address + 2];
    uint8_t b4 = vm->memory[address + 3];
    
    /* ToyVM is little-endian. */
    return (int32_t)((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}

void WriteWord(TOYVM* vm, int32_t address, int32_t value)
{
    uint8_t b1 =  value & 0xff;
    uint8_t b2 = (value & 0xff00) >> 8;
    uint8_t b3 = (value & 0xff0000) >> 16;
    uint8_t b4 = (value & 0xff000000) >> 24;
    
    vm->memory[address]     = b1;
    vm->memory[address + 1] = b2;
    vm->memory[address + 2] = b3;
    vm->memory[address + 3] = b4;
}

static uint8_t ReadByte(TOYVM* vm, size_t address)
{
    return vm->memory[address];
}

/*******************************************************************************
* Pops a single word from the stack. Used by some instructions that implicitly *
* operate on stack.                                                            *
*******************************************************************************/
static int32_t PopVM(TOYVM* vm)
{
    if (StackIsEmpty(vm))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return 0;
    }
    
    int32_t word = ReadWord(vm, vm->cpu.stack_pointer);
    vm->cpu.stack_pointer += 4;
    return word;
}

/*******************************************************************************
* Pushes a single word to the stack. Used by some instructions. Used           *
* implicitly by some instructions.                                             *
*******************************************************************************/
static void PushVM(TOYVM* vm, uint32_t value)
{
    WriteWord(vm, vm->cpu.stack_pointer -= 4, value);
}

static bool IsValidRegisterIndex(uint8_t byte)
{
    switch (byte)
    {
        case REG1:
        case REG2:
        case REG3:
        case REG4:
            return true;
    }
    
    return false;
}

static int32_t GetProgramCounter(TOYVM* vm)
{
    return vm->cpu.program_counter;
}

static bool ExecuteAdd(TOYVM* vm)
{
    uint8_t source_register_index;
    uint8_t target_register_index;
    
    if (!InstructionFitsInMemory(vm, ADD))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    source_register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    target_register_index = ReadByte(vm, GetProgramCounter(vm) + 2);
    
    if (!IsValidRegisterIndex(source_register_index) ||
        !IsValidRegisterIndex(target_register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return true;
    }
    
    vm->cpu.registers[target_register_index]
    += vm->cpu.registers[source_register_index];
    
    /* Advance the program counter past this instruction. */
    vm->cpu.program_counter += GetInstructionLength(vm, ADD);
    return false;
}

static bool ExecuteNeg(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, NEG))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    
    if (!IsValidRegisterIndex(register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return true;
    }
    
    vm->cpu.registers[register_index] = -vm->cpu.registers[register_index];
    vm->cpu.program_counter += GetInstructionLength(vm, NEG);
    return false;
}

static bool ExecuteMul(TOYVM* vm)
{
    uint8_t source_register_index;
    uint8_t target_register_index;
    
    if (!InstructionFitsInMemory(vm, MUL))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    source_register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    target_register_index = ReadByte(vm, GetProgramCounter(vm) + 2);
    
    if (!IsValidRegisterIndex(source_register_index) ||
        !IsValidRegisterIndex(target_register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return true;
    }
    
    vm->cpu.registers[target_register_index] *=
    vm->cpu.registers[source_register_index];
    /* Advance the program counter past this instruction. */
    vm->cpu.program_counter += GetInstructionLength(vm, MUL);
    return false;
}

static bool ExecuteDiv(TOYVM* vm)
{
    uint8_t source_register_index;
    uint8_t target_register_index;
    
    if (!InstructionFitsInMemory(vm, DIV))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    source_register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    target_register_index = ReadByte(vm, GetProgramCounter(vm) + 2);
    
    if (!IsValidRegisterIndex(source_register_index) ||
        !IsValidRegisterIndex(target_register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return true;
    }
    
    vm->cpu.registers[target_register_index] /=
    vm->cpu.registers[source_register_index];
    /* Advance the program counter past this instruction. */
    vm->cpu.program_counter += GetInstructionLength(vm, DIV);
    return false;
}

static bool ExecuteMod(TOYVM* vm)
{
    uint8_t source_register_index;
    uint8_t target_register_index;
    
    if (!InstructionFitsInMemory(vm, MOD))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    source_register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    target_register_index = ReadByte(vm, GetProgramCounter(vm) + 2);
    
    if (!IsValidRegisterIndex(source_register_index) ||
        !IsValidRegisterIndex(target_register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return true;
    }
    
    vm->cpu.registers[target_register_index] =
    vm->cpu.registers[source_register_index] %
    vm->cpu.registers[target_register_index];
    
    /* Advance the program counter past this instruction. */
    vm->cpu.program_counter += GetInstructionLength(vm, MOD);
    return false;
}

static bool ExecuteCmp(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, CMP))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    uint8_t register_index_1 = ReadByte(vm, GetProgramCounter(vm) + 1);
    uint8_t register_index_2 = ReadByte(vm, GetProgramCounter(vm) + 2);
    
    if (!IsValidRegisterIndex(register_index_1) ||
        !IsValidRegisterIndex(register_index_2))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return true;
    }
    
    int32_t register_1 = vm->cpu.registers[register_index_1];
    int32_t register_2 = vm->cpu.registers[register_index_2];
    
    if (register_1 < register_2)
    {
        vm->cpu.status.COMPARISON_ABOVE = 0;
        vm->cpu.status.COMPARISON_EQUAL = 0;
        vm->cpu.status.COMPARISON_BELOW = 1;
    }
    else if (register_1 > register_2)
    {
        vm->cpu.status.COMPARISON_ABOVE = 1;
        vm->cpu.status.COMPARISON_EQUAL = 0;
        vm->cpu.status.COMPARISON_BELOW = 0;
    }
    else
    {
        vm->cpu.status.COMPARISON_ABOVE = 0;
        vm->cpu.status.COMPARISON_EQUAL = 1;
        vm->cpu.status.COMPARISON_BELOW = 0;
    }
    
    vm->cpu.program_counter += GetInstructionLength(vm, CMP);
    return false;
}

static bool ExecuteJumpIfAbove(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, JA))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    if (vm->cpu.status.COMPARISON_ABOVE)
    {
        vm->cpu.program_counter = ReadWord(vm, GetProgramCounter(vm) + 1);
    }
    else
    {
        vm->cpu.program_counter += GetInstructionLength(vm, JA);
    }
    
    return false;
}

static bool ExecuteJumpIfEqual(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, JE))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    if (vm->cpu.status.COMPARISON_EQUAL)
    {
        vm->cpu.program_counter = ReadWord(vm, GetProgramCounter(vm) + 1);
    }
    else
    {
        vm->cpu.program_counter += GetInstructionLength(vm, JE);
    }
    
    return false;
}

static bool ExecuteJumpIfBelow(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, JB))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    if (vm->cpu.status.COMPARISON_BELOW)
    {
        vm->cpu.program_counter = ReadWord(vm, GetProgramCounter(vm) + 1);
    }
    else
    {
        vm->cpu.program_counter += GetInstructionLength(vm, JB);
    }
    
    return false;
}

static bool ExecuteJump(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, JMP))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    vm->cpu.program_counter = ReadWord(vm, GetProgramCounter(vm) + 1);
    return false;
}

static bool ExecuteCall(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, CALL))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    if (GetAvailableStackSize(vm) < 4)
    {
        vm->cpu.status.STACK_OVERFLOW = 1;
        return true;
    }
    
    /* Save the return address on the stack. */
    uint32_t address = ReadWord(vm, GetProgramCounter(vm) + 1);
    PushVM(vm, (uint32_t)(GetProgramCounter(vm) +
                          GetInstructionLength(vm, CALL)));
    /* Actual jump to the subroutine. */
    vm->cpu.program_counter = address;
    return false;
}

static bool ExecuteRet(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, RET))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    if (StackIsEmpty(vm))
    {
        vm->cpu.status.STACK_UNDERFLOW = 1;
        return true;
    }
    
    vm->cpu.program_counter = PopVM(vm);
    return false;
}

static bool ExecuteLoad(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, LOAD))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    
    if (!IsValidRegisterIndex(register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return true;
    }
    
    uint32_t address = ReadWord(vm, GetProgramCounter(vm) + 2);
    vm->cpu.registers[register_index] = ReadWord(vm, address);
    vm->cpu.program_counter += GetInstructionLength(vm, LOAD);
    return false;
}

static bool ExecuteStore(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, STORE))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    
    if (!IsValidRegisterIndex(register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return true;
    }
    
    uint32_t address = ReadWord(vm, GetProgramCounter(vm) + 2);
    WriteWord(vm, address, vm->cpu.registers[register_index]);
    vm->cpu.program_counter += GetInstructionLength(vm, STORE);
    return false;
}

static bool ExecuteConst(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, CONST))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    int32_t datum = ReadWord(vm, GetProgramCounter(vm) + 2);
    
    if (!IsValidRegisterIndex(register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return true;
    }
    
    vm->cpu.registers[register_index] = datum;
    vm->cpu.program_counter += GetInstructionLength(vm, CONST);
    return false;
}

static void PrintString(TOYVM* vm, uint32_t address)
{
    printf("%s", (const char*)(&vm->memory[address]));
}

static bool ExecuteInterrupt(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, INT))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    uint8_t interrupt_number = ReadByte(vm, GetProgramCounter(vm) + 1);
    
    if (StackIsEmpty(vm))
    {
        vm->cpu.status.STACK_UNDERFLOW = 1;
        return true;
    }
    
    switch (interrupt_number)
    {
        case INTERRUPT_PRINT_INTEGER:
            printf("%d", PopVM(vm));
            break;
            
        case INTERRUPT_PRINT_STRING:
            PrintString(vm, PopVM(vm));
            break;
            
        default:
            return true;
    }
    
    vm->cpu.program_counter += GetInstructionLength(vm, INT);
    return false;
}

static bool ExecutePush(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, PUSH))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    if (StackIsFull(vm))
    {
        return true;
    }
    
    uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    
    if (!IsValidRegisterIndex(register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return true;
    }
    
    WriteWord(vm,
              vm->cpu.stack_pointer - 4,
              vm->cpu.registers[register_index]);
    
    vm->cpu.stack_pointer -= 4;
    vm->cpu.program_counter += GetInstructionLength(vm, PUSH);
    return false;
}

static bool ExecutePushAll(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, PUSH_ALL))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    if (!CanPerformMultipush(vm))
    {
        vm->cpu.status.STACK_OVERFLOW = 1;
        return true;
    }
    
    WriteWord(vm, vm->cpu.stack_pointer -= 4, vm->cpu.registers[REG1]);
    WriteWord(vm, vm->cpu.stack_pointer -= 4, vm->cpu.registers[REG2]);
    WriteWord(vm, vm->cpu.stack_pointer -= 4, vm->cpu.registers[REG3]);
    WriteWord(vm, vm->cpu.stack_pointer -= 4, vm->cpu.registers[REG4]);
    vm->cpu.program_counter += GetInstructionLength(vm, PUSH_ALL);
    return false;
}

static bool ExecutePop(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, POP))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    if (StackIsEmpty(vm))
    {
        return true;
    }
    
    uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    
    if (!IsValidRegisterIndex(register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return true;
    }
    
    int32_t datum = ReadWord(vm, GetProgramCounter(vm) + 2);
    vm->cpu.registers[register_index] = datum;
    vm->cpu.stack_pointer += 4;
    vm->cpu.program_counter += GetInstructionLength(vm, POP);
    return false;
}

static bool ExecutePopAll(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, POP_ALL))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    if (!CanPerformMultipop(vm))
    {
        vm->cpu.status.STACK_UNDERFLOW = 1;
        return true;
    }
    
    vm->cpu.registers[REG4] = ReadWord(vm, vm->cpu.stack_pointer);
    vm->cpu.registers[REG3] = ReadWord(vm, vm->cpu.stack_pointer + 4);
    vm->cpu.registers[REG2] = ReadWord(vm, vm->cpu.stack_pointer + 8);
    vm->cpu.registers[REG1] = ReadWord(vm, vm->cpu.stack_pointer + 12);
    vm->cpu.stack_pointer += 16;
    vm->cpu.program_counter += GetInstructionLength(vm, POP_ALL);
    return false;
}

static bool ExecuteLSP(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, LSP))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    
    if (!IsValidRegisterIndex(register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return true;
    }
    
    vm->cpu.registers[register_index] = vm->cpu.stack_pointer;
    vm->cpu.program_counter += GetInstructionLength(vm, LSP);
    return false;
}

static bool ExecuteNop(TOYVM* vm) {
    if (!InstructionFitsInMemory(vm, NOP))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return true;
    }
    
    vm->cpu.program_counter += GetInstructionLength(vm, NOP);
    return false;
}

static bool ExecuteHalt(TOYVM* vm) {
    return true;
}

void PrintStatus(TOYVM* vm)
{
    printf("BAD_INSTRUCTION       : %d\n", vm->cpu.status.BAD_INSTRUCTION);
    printf("STACK_UNDERFLOW       : %d\n", vm->cpu.status.STACK_UNDERFLOW);
    printf("STACK_OVERFLOW        : %d\n", vm->cpu.status.STACK_OVERFLOW);
    printf("INVALID_REGISTER_INDEX: %d\n",
           vm->cpu.status.INVALID_REGISTER_INDEX);
    
    printf("BAD_ACCESS            : %d\n", vm->cpu.status.BAD_ACCESS);
    printf("COMPARISON_ABOVE      : %d\n", vm->cpu.status.COMPARISON_ABOVE);
    printf("COMPARISON_EQUAL      : %d\n", vm->cpu.status.COMPARISON_EQUAL);
    printf("COMPARISON_BELOW      : %d\n", vm->cpu.status.COMPARISON_BELOW);
}

const instruction instructions[] = {
    { 0,        0, NULL       },
    { ADD,      3, ExecuteAdd },
    { NEG,      2, ExecuteNeg },
    { MUL,      3, ExecuteMul },
    { DIV,      3, ExecuteDiv },
    { MOD,      3, ExecuteMod },
    
    { CMP,      3, ExecuteCmp },
    { JA,       5, ExecuteJumpIfAbove },
    { JE,       5, ExecuteJumpIfEqual },
    { JB,       5, ExecuteJumpIfBelow },
    { JMP,      5, ExecuteJump },
    
    { CALL,     5, ExecuteCall },
    { RET,      1, ExecuteRet },
    
    { LOAD,     6, ExecuteLoad },
    { STORE,    6, ExecuteStore },
    { CONST,    6, ExecuteConst },
    
    { HALT,     1, ExecuteHalt },
    { INT,      2, ExecuteInterrupt },
    { NOP,      1, ExecuteNop },
    
    { PUSH,     2, ExecutePush },
    { PUSH_ALL, 1, ExecutePushAll },
    { POP,      2, ExecutePop },
    { POP_ALL,  1, ExecutePopAll },
    { LSP,      2, ExecuteLSP }
};

static size_t GetInstructionLength(TOYVM* vm, uint8_t opcode)
{
    size_t index = vm->opcode_map[opcode];
    return instructions[index].size;
}

/*******************************************************************************
 * Checks that an instruction fits entirely in the memory.                      *
 *******************************************************************************/
static bool InstructionFitsInMemory(TOYVM* vm, uint8_t opcode)
{
    size_t instruction_length = GetInstructionLength(vm, opcode);
    return vm->cpu.program_counter + instruction_length <= vm->memory_size;
}

void RunVM(TOYVM* vm)
{
    while (true)
    {
        int32_t program_counter = GetProgramCounter(vm);
        
        if (program_counter < 0 || program_counter >= vm->memory_size)
        {
            vm->cpu.status.BAD_ACCESS = 1;
            return;
        }
        
        uint8_t opcode = vm->memory[program_counter];
        size_t index = vm->opcode_map[opcode];
    
        if (index == 0)
        {
            vm->cpu.status.BAD_INSTRUCTION = 1;
            return;
        }
    
        bool (*opcode_exec)(TOYVM*) =
        instructions[index].execute;
    
        if (opcode_exec(vm))
        {
            return;
        }
    }
}

