#ifndef TOYVM_H
#define TOYVM_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* Arithmetics */
const uint8_t ADD = 0x01;
const uint8_t NEG = 0x02;
const uint8_t MUL = 0x03;
const uint8_t DIV = 0x04;
const uint8_t MOD = 0x05;

/* Conditionals */
const uint8_t JUMP_IF_ZERO = 0x10;
const uint8_t JUMP_IF_NEG  = 0x11;
const uint8_t JUMP_IF_POS  = 0x12;

/* Subroutines */
const uint8_t CALL = 0x20;
const uint8_t RET  = 0x21;

/* Moving data */
const uint8_t LOAD  = 0x30;
const uint8_t STORE = 0x31;
const uint8_t CONST = 0x32;

/* Auxiliary */
const uint8_t HALT = 0x40;
const uint8_t INT  = 0x41;
const uint8_t NOP  = 0x42;

/* Stack operations */
const uint8_t PUSH     = 0x50;
const uint8_t PUSH_ALL = 0x51;
const uint8_t POP      = 0x5a;
const uint8_t POP_ALL  = 0x5b;

/* Register indices */
const uint8_t REG1 = 0x1;
const uint8_t REG2 = 0x2;
const uint8_t REG3 = 0x3;
const uint8_t REG4 = 0x4;

/* Status codes */
const uint8_t STATUS_HALT_OK                = 0x1;
const uint8_t STATUS_HALT_BAD_INSTRUCTION   = 0x2;
const uint8_t STATUS_STACK_OVERFLOW         = 0x4;
const uint8_t STATUS_STACK_UNDERFLOW        = 0x8;
const uint8_t STATUS_INVALID_REGISTER_INDEX = 0x16;

/* Interrupts */
const uint8_t INTERRUPT_PRINT_INTEGER = 0x1;
const uint8_t INTERRUPT_PRINT_STRING  = 0x2;

typedef struct VM_CPU {
    int32_t reg1;
    int32_t reg2;
    int32_t reg3;
    int32_t reg4;
    size_t  program_counter;
    int32_t stack_pointer;
    uint8_t status;
} VM_CPU;

typedef struct TOYVM {
    uint8_t* memory;
    size_t   memory_size;
    int32_t  stack_limit;
    VM_CPU   cpu;
} TOYVM;

static bool STACK_IS_EMPTY(TOYVM* vm)
{
    return vm->cpu.stack_pointer >= vm->memory_size;
}

static bool STACK_IS_FULL(TOYVM* vm)
{
    return vm->cpu.stack_pointer <= vm->stack_limit;
}

static bool CAN_PERFORM_MULTIPUSH(TOYVM* vm)
{
    /* 16 = 4 (registers) * 4 (bytes each) */
    return vm->cpu.stack_pointer - vm->stack_limit >= 16;
}

static bool CAN_PERFORM_MULTIPOP(TOYVM* vm)
{
    return vm->cpu.stack_pointer <= vm->memory_size - 16;
}

void INIT_VM(TOYVM* vm, size_t memory_size, int32_t stack_limit)
{
    /* Make sure both 'memory_size' and 'stack_limit' are divisible by 4. */
    memory_size             -= (memory_size % 4);
    stack_limit             -= (stack_limit % 4);
    vm->memory              = calloc(memory_size, 1);
    vm->memory_size         = memory_size;
    vm->stack_limit         = stack_limit;
    vm->cpu.program_counter = 0;
    vm->cpu.stack_pointer   = (int32_t) memory_size;
    vm->cpu.reg1            = 0;
    vm->cpu.reg2            = 0;
    vm->cpu.reg3            = 0;
    vm->cpu.reg4            = 0;
}

void WRITE_VM_MEMORY(TOYVM* vm, uint8_t* mem, size_t size)
{
    memcpy(mem, vm->memory, size);
    vm->memory_size = size;
}

static int32_t READ_WORD(TOYVM* vm, size_t address)
{
    uint8_t b1 = vm->memory[address];
    uint8_t b2 = vm->memory[address + 1];
    uint8_t b3 = vm->memory[address + 2];
    uint8_t b4 = vm->memory[address + 3];
    
    /* ToyVM is little-endian. */
    return (int32_t)((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}

static void WRITE_WORD(TOYVM* vm, size_t address, int32_t value)
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

static uint8_t READ_BYTE(TOYVM* vm, size_t address)
{
    return vm->memory[address];
}

static bool IS_VALID_REGISTER_INDEX(uint8_t byte)
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

/*******************************************************************************
* Checks that an instruction of 'instruction_length' bytes fits in the memory  *
* if start counting its bytes from the memory address pointed to by program    *
* counter.                                                                     *
*******************************************************************************/
static bool INSTRUCTION_FITS_IN_MEMORY(TOYVM* vm, size_t instruction_length)
{
    return vm->cpu.program_counter + instruction_length < vm->memory_size + 1;
}

static size_t PROGRAM_COUNTER(TOYVM* vm)
{
    return vm->cpu.program_counter;
}

static bool EXECUTE_ADD(TOYVM* vm)
{
    uint8_t source_register_index;
    uint8_t target_register_index;
    
    if (!INSTRUCTION_FITS_IN_MEMORY(vm, 3))
    {
        return false;
    }
    
    source_register_index = vm->memory[vm->cpu.program_counter + 1];
    target_register_index = vm->memory[vm->cpu.program_counter + 2];
    uint32_t DATUM;
    
    switch (source_register_index)
    {
        case REG1:
            DATUM = vm->cpu.reg1;
            break;
            
        case REG2:
            DATUM = vm->cpu.reg2;
            break;
            
        case REG3:
            DATUM = vm->cpu.reg3;
            break;
            
        case REG4:
            DATUM = vm->cpu.reg4;
            break;
            
        default:
            return false;
    }
    
    switch (target_register_index)
    {
        case REG1:
            vm->cpu.reg1 += DATUM;
            break;
            
        case REG2:
            vm->cpu.reg2 += DATUM;
            break;
            
        case REG3:
            vm->cpu.reg3 += DATUM;
            break;
            
        case REG4:
            vm->cpu.reg4 += DATUM;
            break;
            
        default:
            return false;
    }
    
    /* Advance the program counter past this instruction. */
    vm->cpu.program_counter += 3;
    return true;
}

static bool EXECUTE_NEG(TOYVM* vm)
{
    if (!INSTRUCTION_FITS_IN_MEMORY(vm, 2))
    {
        return false;
    }
    
    switch (vm->memory[vm->cpu.program_counter + 1])
    {
        case REG1:
            vm->cpu.reg1 = - vm->cpu.reg1;
            break;
            
        case REG2:
            vm->cpu.reg2 = - vm->cpu.reg2;
            break;
            
        case REG3:
            vm->cpu.reg3 = - vm->cpu.reg3;
            break;
            
        case REG4:
            vm->cpu.reg4 = - vm->cpu.reg4;
            break;
            
        default:
            return false;
    }
    
    vm->cpu.program_counter += 2;
    return true;
}

static bool EXECUTE_MUL(TOYVM* vm)
{
    uint8_t source_register_index;
    uint8_t target_register_index;
    
    if (!INSTRUCTION_FITS_IN_MEMORY(vm, 3))
    {
        return false;
    }
    
    source_register_index = vm->memory[vm->cpu.program_counter + 1];
    target_register_index = vm->memory[vm->cpu.program_counter + 2];
    uint32_t DATUM;
    
    switch (source_register_index)
    {
        case REG1:
            DATUM = vm->cpu.reg1;
            break;
            
        case REG2:
            DATUM = vm->cpu.reg2;
            break;
            
        case REG3:
            DATUM = vm->cpu.reg3;
            break;
            
        case REG4:
            DATUM = vm->cpu.reg4;
            break;
            
        default:
            return false;
    }
    
    switch (target_register_index)
    {
        case REG1:
            vm->cpu.reg1 *= DATUM;
            break;
            
        case REG2:
            vm->cpu.reg2 *= DATUM;
            break;
            
        case REG3:
            vm->cpu.reg3 *= DATUM;
            break;
            
        case REG4:
            vm->cpu.reg4 *= DATUM;
            break;
            
        default:
            return false;
    }
    
    /* Advance the program counter past this instruction. */
    vm->cpu.program_counter += 3;
    return true;
}

static bool EXECUTE_DIV(TOYVM* vm)
{
    uint8_t source_register_index;
    uint8_t target_register_index;
    
    if (!INSTRUCTION_FITS_IN_MEMORY(vm, 3))
    {
        return false;
    }
    
    source_register_index = vm->memory[vm->cpu.program_counter + 1];
    target_register_index = vm->memory[vm->cpu.program_counter + 2];
    uint32_t DATUM;
    
    switch (source_register_index)
    {
        case REG1:
            DATUM = vm->cpu.reg1;
            break;
            
        case REG2:
            DATUM = vm->cpu.reg2;
            break;
            
        case REG3:
            DATUM = vm->cpu.reg3;
            break;
            
        case REG4:
            DATUM = vm->cpu.reg4;
            break;
            
        default:
            return false;
    }
    
    switch (target_register_index)
    {
        case REG1:
            vm->cpu.reg1 /= DATUM;
            break;
            
        case REG2:
            vm->cpu.reg2 /= DATUM;
            break;
            
        case REG3:
            vm->cpu.reg3 /= DATUM;
            break;
            
        case REG4:
            vm->cpu.reg4 /= DATUM;
            break;
            
        default:
            return false;
    }
    
    /* Advance the program counter past this instruction. */
    vm->cpu.program_counter += 3;
    return true;
}

static bool EXECUTE_MOD(TOYVM* vm)
{
    uint8_t source_register_index;
    uint8_t target_register_index;
    
    if (!INSTRUCTION_FITS_IN_MEMORY(vm, 3))
    {
        return false;
    }
    
    source_register_index = vm->memory[vm->cpu.program_counter + 1];
    target_register_index = vm->memory[vm->cpu.program_counter + 2];
    uint32_t DATUM;
    
    switch (source_register_index)
    {
        case REG1:
            DATUM = vm->cpu.reg1;
            break;
            
        case REG2:
            DATUM = vm->cpu.reg2;
            break;
            
        case REG3:
            DATUM = vm->cpu.reg3;
            break;
            
        case REG4:
            DATUM = vm->cpu.reg4;
            break;
            
        default:
            return false;
    }
    
    switch (target_register_index)
    {
        case REG1:
            vm->cpu.reg1 %= DATUM;
            break;
            
        case REG2:
            vm->cpu.reg2 %= DATUM;
            break;
            
        case REG3:
            vm->cpu.reg3 %= DATUM;
            break;
            
        case REG4:
            vm->cpu.reg4 %= DATUM;
            break;
            
        default:
            return false;
    }
    
    /* Advance the program counter past this instruction. */
    vm->cpu.program_counter += 3;
    return true;
}

static bool EXECUTE_CONST(TOYVM* vm)
{
    if (!INSTRUCTION_FITS_IN_MEMORY(vm, 6))
    {
        return false;
    }
    
    size_t program_counter = PROGRAM_COUNTER(vm);
    int32_t datum = READ_WORD(vm, program_counter + 2);
    uint8_t register_index = READ_BYTE(vm, program_counter + 1);
    
    if (!IS_VALID_REGISTER_INDEX(register_index))
    {
        return false;
    }
    
    switch (register_index)
    {
        case REG1:
            vm->cpu.reg1 = datum;
            break;
            
        case REG2:
            vm->cpu.reg2 = datum;
            break;
            
        case REG3:
            vm->cpu.reg3 = datum;
            break;
            
        case REG4:
            vm->cpu.reg4 = datum;
            break;
    }
    
    vm->cpu.program_counter += 6;
    return true;
}
/*
static bool EXECUTE_INT(TOYVM* vm)
{
    if (!INSTRUCTION_FITS_IN_MEMORY(vm, 2))
    {
        return false;
    }
    
    uint8_t handler_id = READ_BYTE(vm, PROGRAM_COUNTER(vm) + 1);
    
    switch (handler_id) {
        case INTERRUPT_PRINT_INTEGER:
            break;
            
        case INTERRUPT_PRINT_STRING:
            
    }
    
    vm->cpu.program_counter += 2;
    return true;
}*/

static bool EXECUTE_PUSH(TOYVM* vm)
{
    if (!INSTRUCTION_FITS_IN_MEMORY(vm, 2))
    {
        return false;
    }
    
    if (STACK_IS_FULL(vm))
    {
        return false;
    }

    switch (READ_BYTE(vm, PROGRAM_COUNTER(vm) + 1))
    {
        case REG1:
            WRITE_WORD(vm, vm->cpu.stack_pointer - 4, vm->cpu.reg1);
            break;
            
        case REG2:
            WRITE_WORD(vm, vm->cpu.stack_pointer - 4, vm->cpu.reg2);
            break;
            
        case REG3:
            WRITE_WORD(vm, vm->cpu.stack_pointer - 4, vm->cpu.reg3);
            break;
            
        case REG4:
            WRITE_WORD(vm, vm->cpu.stack_pointer - 4, vm->cpu.reg4);
            break;
            
        default:
            vm->cpu.status |= STATUS_INVALID_REGISTER_INDEX;
            return false;
    }
    
    vm->cpu.stack_pointer -= 4;
    vm->cpu.program_counter += 2;
    return true;
}

static bool EXECUTE_POP(TOYVM* vm)
{
    if (!INSTRUCTION_FITS_IN_MEMORY(vm, 2))
    {
        return false;
    }
    
    if (STACK_IS_EMPTY(vm))
    {
        return false;
    }
    
    size_t program_counter = PROGRAM_COUNTER(vm);
    int32_t datum = READ_WORD(vm, program_counter + 2);
    
    switch (READ_BYTE(vm, program_counter + 1))
    {
        case REG1:
            vm->cpu.reg1 = datum;
            break;
            
        case REG2:
            vm->cpu.reg2 = datum;
            break;
            
        case REG3:
            vm->cpu.reg3 = datum;
            break;
            
        case REG4:
            vm->cpu.reg4 = datum;
            break;
            
        default:
            vm->cpu.status |= STATUS_INVALID_REGISTER_INDEX;
            return false;
    }
    
    vm->cpu.stack_pointer += 4;
    vm->cpu.program_counter += 2;
    return true;
}

static uint32_t POP_VM(TOYVM* vm)
{
    if (STACK_IS_EMPTY(vm))
    {
        return 0;
    }
    
    uint32_t word = READ_WORD(vm, vm->cpu.stack_pointer);
    vm->cpu.stack_pointer += 4;
    return word;
}

static void PRINT_STRING(TOYVM* vm, uint32_t address)
{
    printf("%s", (char*) vm->memory[address]);
}

static bool EXECUTE_INTERRUPT(TOYVM* vm)
{
    if (!INSTRUCTION_FITS_IN_MEMORY(vm, 2))
    {
        return false;
    }
    
    uint8_t interrupt_number = READ_BYTE(vm, PROGRAM_COUNTER(vm) + 1);
    
    switch (interrupt_number)
    {
        case INTERRUPT_PRINT_INTEGER:
            if (STACK_IS_EMPTY(vm))
            {
                return false;
            }
            
            printf("%d", POP_VM(vm));
            break;
            
        case INTERRUPT_PRINT_STRING:
            if (STACK_IS_EMPTY(vm))
            {
                return false;
            }
            
            PRINT_STRING(vm, POP_VM(vm));
            break;
            
        default:
            return false;
    }
    
    vm->cpu.program_counter += 2;
    return true;
}

static bool EXECUTE_MULTIPUSH(TOYVM* vm)
{
    if (!CAN_PERFORM_MULTIPUSH(vm))
    {
        vm->cpu.status |= STATUS_STACK_OVERFLOW;
        return false;
    }
    
    WRITE_WORD(vm, vm->cpu.stack_pointer -= 4, vm->cpu.reg1);
    WRITE_WORD(vm, vm->cpu.stack_pointer -= 4, vm->cpu.reg2);
    WRITE_WORD(vm, vm->cpu.stack_pointer -= 4, vm->cpu.reg3);
    WRITE_WORD(vm, vm->cpu.stack_pointer -= 4, vm->cpu.reg4);
    vm->cpu.program_counter++;
    return true;
}

static bool EXECUTE_MULTIPOP(TOYVM* vm)
{
    if (!CAN_PERFORM_MULTIPOP(vm))
    {
        vm->cpu.status |= STATUS_STACK_UNDERFLOW;
        return false;
    }
    
    vm->cpu.reg4 = READ_WORD(vm, vm->cpu.stack_pointer);
    vm->cpu.reg3 = READ_WORD(vm, vm->cpu.stack_pointer + 4);
    vm->cpu.reg2 = READ_WORD(vm, vm->cpu.stack_pointer + 8);
    vm->cpu.reg1 = READ_WORD(vm, vm->cpu.stack_pointer + 12);
    vm->cpu.stack_pointer += 16;
    vm->cpu.program_counter++;
    return true;
}

void RUN_VM(TOYVM* vm)
{
    uint8_t opcode = NOP;
    
    while (opcode != HALT)
    {
        opcode = vm->memory[vm->cpu.program_counter];
        
        switch (opcode)
        {
            case ADD:
                if (!EXECUTE_ADD(vm))
                {
                    return;
                }
                
                break;
                
            case MUL:
                if (!EXECUTE_MUL(vm))
                {
                    return;
                }
                
                break;
                
            case DIV:
                if (!EXECUTE_DIV(vm))
                {
                    return;
                }
                
                break;
                
            case MOD:
                if (!EXECUTE_MOD(vm))
                {
                    return;
                }
                
                break;
                
            case NEG:
                if (!EXECUTE_NEG(vm))
                {
                    return;
                }
                
                break;
                
            case CONST:
                if (!EXECUTE_CONST(vm))
                {
                    return;
                }
                
                break;
                
            case POP:
                if (!EXECUTE_POP(vm))
                {
                    return;
                }
                
                break;
                
            case PUSH:
                if (!EXECUTE_PUSH(vm))
                {
                    return;
                }
                
                break;
                
            case PUSH_ALL:
                if (!EXECUTE_MULTIPUSH(vm))
                {
                    return;
                }
                
                break;
                
            case POP_ALL:
                if (!EXECUTE_MULTIPOP(vm))
                {
                    return;
                }
                
                break;
                
            case INT:
                if (!EXECUTE_INTERRUPT(vm))
                {
                    return;
                }
                
                break;
                
            case HALT:
                return;
                
            default:
                return;
        }
    }
}

#endif /* TOYVM_H */
