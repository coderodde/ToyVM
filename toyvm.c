#include "toyvm.h"

size_t GetInstructionLength(uint8_t opcode)
{
    switch (opcode)
    {
        case PUSH_ALL:
        case POP_ALL:
        case RET:
        case HALT:
        case NOP:
            return 1;
            
        case LSP:
        case PUSH:
        case POP:
        case NEG:
        case INT:
            return 2;
            
        case ADD:
        case MUL:
        case DIV:
        case MOD:
        case CMP:
            return 3;
            
        case CALL:
        case JA:
        case JE:
        case JB:
        case JMP:
            return 5;
            
        case LOAD:
        case STORE:
        case CONST:
            return 6;
            
        default:
            return 0;
    }
}

static bool StackIsEmpty(TOYVM* vm)
{
    return vm->cpu.stack_pointer >= vm->memory_size;
}

static bool StackIsFull(TOYVM* vm)
{
    return vm->cpu.stack_pointer <= vm->stack_limit;
}

static int32_t GetAvailableStackSize(TOYVM* vm)
{
    return vm->cpu.stack_pointer - vm->stack_limit;
}

static int32_t GetOccupiedStackSize(TOYVM* vm)
{
    return vm->memory_size - vm->cpu.stack_pointer;
}

static bool CanPerformMultipush(TOYVM* vm)
{
    return GetAvailableStackSize(vm) >= sizeof(int32_t) * N_REGISTERS;
}

static bool CanPerformMultipop(TOYVM* vm)
{
    return GetOccupiedStackSize(vm) >= sizeof(int32_t) * N_REGISTERS;
}

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
    memset(vm->cpu.registers, 0, sizeof(int32_t) * N_REGISTERS);
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

/*******************************************************************************
 * Checks that an instruction fits entirely in the memory.                      *
 *******************************************************************************/
static bool InstructionFitsInMemory(TOYVM* vm, uint8_t opcode)
{
    size_t instruction_length = GetInstructionLength(opcode);
    return vm->cpu.program_counter + instruction_length <= vm->memory_size;
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
        return false;
    }
    
    source_register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    target_register_index = ReadByte(vm, GetProgramCounter(vm) + 2);
    
    if (!IsValidRegisterIndex(source_register_index) ||
        !IsValidRegisterIndex(target_register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return false;
    }
    
    vm->cpu.registers[target_register_index]
    += vm->cpu.registers[source_register_index];
    
    /* Advance the program counter past this instruction. */
    vm->cpu.program_counter += GetInstructionLength(ADD);
    return true;
}

static bool ExecuteNeg(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, NEG))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return false;
    }
    
    uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    
    if (!IsValidRegisterIndex(register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return false;
    }
    
    vm->cpu.registers[register_index] = -vm->cpu.registers[register_index];
    vm->cpu.program_counter += GetInstructionLength(NEG);
    return true;
}

static bool ExecuteMul(TOYVM* vm)
{
    uint8_t source_register_index;
    uint8_t target_register_index;
    
    if (!InstructionFitsInMemory(vm, MUL))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return false;
    }
    
    source_register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    target_register_index = ReadByte(vm, GetProgramCounter(vm) + 2);
    
    if (!IsValidRegisterIndex(source_register_index) ||
        !IsValidRegisterIndex(target_register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return false;
    }
    
    vm->cpu.registers[target_register_index] *=
    vm->cpu.registers[source_register_index];
    /* Advance the program counter past this instruction. */
    vm->cpu.program_counter += GetInstructionLength(MUL);
    return true;
}

static bool ExecuteDiv(TOYVM* vm)
{
    uint8_t source_register_index;
    uint8_t target_register_index;
    
    if (!InstructionFitsInMemory(vm, DIV))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return false;
    }
    
    source_register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    target_register_index = ReadByte(vm, GetProgramCounter(vm) + 2);
    
    if (!IsValidRegisterIndex(source_register_index) ||
        !IsValidRegisterIndex(target_register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return false;
    }
    
    vm->cpu.registers[target_register_index] /=
    vm->cpu.registers[source_register_index];
    /* Advance the program counter past this instruction. */
    vm->cpu.program_counter += GetInstructionLength(DIV);
    return true;
}

static bool ExecuteMod(TOYVM* vm)
{
    uint8_t source_register_index;
    uint8_t target_register_index;
    
    if (!InstructionFitsInMemory(vm, MOD))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return false;
    }
    
    source_register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    target_register_index = ReadByte(vm, GetProgramCounter(vm) + 2);
    
    if (!IsValidRegisterIndex(source_register_index) ||
        !IsValidRegisterIndex(target_register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return false;
    }
    
    vm->cpu.registers[target_register_index] =
    vm->cpu.registers[source_register_index] %
    vm->cpu.registers[target_register_index];
    
    /* Advance the program counter past this instruction. */
    vm->cpu.program_counter += GetInstructionLength(MOD);
    return true;
}

static bool ExecuteCmp(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, CMP))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return false;
    }
    
    uint8_t register_index_1 = ReadByte(vm, GetProgramCounter(vm) + 1);
    uint8_t register_index_2 = ReadByte(vm, GetProgramCounter(vm) + 2);
    
    if (!IsValidRegisterIndex(register_index_1) ||
        !IsValidRegisterIndex(register_index_2))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return false;
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
    
    vm->cpu.program_counter += GetInstructionLength(CMP);
    return true;
}

static bool ExecuteJumpIfAbove(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, JA))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return false;
    }
    
    if (vm->cpu.status.COMPARISON_ABOVE)
    {
        vm->cpu.program_counter = ReadWord(vm, GetProgramCounter(vm) + 1);
    }
    else
    {
        vm->cpu.program_counter += GetInstructionLength(JA);
    }
    
    return true;
}

static bool ExecuteJumpIfEqual(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, JE))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return false;
    }
    
    if (vm->cpu.status.COMPARISON_EQUAL)
    {
        vm->cpu.program_counter = ReadWord(vm, GetProgramCounter(vm) + 1);
    }
    else
    {
        vm->cpu.program_counter += GetInstructionLength(JE);
    }
    
    return true;
}

static bool ExecuteJumpIfBelow(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, JB))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return false;
    }
    
    if (vm->cpu.status.COMPARISON_BELOW)
    {
        vm->cpu.program_counter = ReadWord(vm, GetProgramCounter(vm) + 1);
    }
    else
    {
        vm->cpu.program_counter += GetInstructionLength(JB);
    }
    
    return true;
}

static bool ExecuteJump(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, JMP))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return false;
    }
    
    vm->cpu.program_counter = ReadWord(vm, GetProgramCounter(vm) + 1);
    return true;
}

static bool ExecuteCallL(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, CALL))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return false;
    }
    
    if (GetAvailableStackSize(vm) < 4)
    {
        vm->cpu.status.STACK_OVERFLOW = 1;
        return false;
    }
    
    /* Save the return address on the stack. */
    uint32_t address = ReadWord(vm, GetProgramCounter(vm) + 1);
    PushVM(vm, (uint32_t)(GetProgramCounter(vm) + GetInstructionLength(CALL)));
    /* Actual jump to the subroutine. */
    vm->cpu.program_counter = address;
    return true;
}

static bool ExecuteRet(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, RET))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return false;
    }
    
    if (StackIsEmpty(vm))
    {
        vm->cpu.status.STACK_UNDERFLOW = 1;
        return false;
    }
    
    vm->cpu.program_counter = PopVM(vm);
    return true;
}

static bool ExecuteLoad(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, LOAD))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return false;
    }
    
    uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    
    if (!IsValidRegisterIndex(register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return false;
    }
    
    uint32_t address = ReadWord(vm, GetProgramCounter(vm) + 2);
    vm->cpu.registers[register_index] = ReadWord(vm, address);
    vm->cpu.program_counter += GetInstructionLength(LOAD);
    return true;
}

static bool ExecuteStore(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, STORE))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return false;
    }
    
    uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    
    if (!IsValidRegisterIndex(register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return false;
    }
    
    uint32_t address = ReadWord(vm, GetProgramCounter(vm) + 2);
    WriteWord(vm, address, vm->cpu.registers[register_index]);
    vm->cpu.program_counter += GetInstructionLength(STORE);
    return true;
}

static bool ExecuteConst(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, CONST))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return false;
    }
    
    uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    int32_t datum = ReadWord(vm, GetProgramCounter(vm) + 2);
    
    if (!IsValidRegisterIndex(register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return false;
    }
    
    vm->cpu.registers[register_index] = datum;
    vm->cpu.program_counter += GetInstructionLength(CONST);
    return true;
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
        return false;
    }
    
    uint8_t interrupt_number = ReadByte(vm, GetProgramCounter(vm) + 1);
    
    if (StackIsEmpty(vm))
    {
        vm->cpu.status.STACK_UNDERFLOW = 1;
        return false;
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
            return false;
    }
    
    vm->cpu.program_counter += GetInstructionLength(INT);
    return true;
}

static bool ExecutePush(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, PUSH))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return false;
    }
    
    if (StackIsFull(vm))
    {
        return false;
    }
    
    uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    
    if (!IsValidRegisterIndex(register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return false;
    }
    
    WriteWord(vm,
              vm->cpu.stack_pointer - 4,
              vm->cpu.registers[register_index]);
    
    vm->cpu.stack_pointer -= 4;
    vm->cpu.program_counter += GetInstructionLength(PUSH);
    return true;
}

static bool ExecutePushAll(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, PUSH_ALL))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return false;
    }
    
    if (!CanPerformMultipush(vm))
    {
        vm->cpu.status.STACK_OVERFLOW = 1;
        return false;
    }
    
    WriteWord(vm, vm->cpu.stack_pointer -= 4, vm->cpu.registers[REG1]);
    WriteWord(vm, vm->cpu.stack_pointer -= 4, vm->cpu.registers[REG2]);
    WriteWord(vm, vm->cpu.stack_pointer -= 4, vm->cpu.registers[REG3]);
    WriteWord(vm, vm->cpu.stack_pointer -= 4, vm->cpu.registers[REG4]);
    vm->cpu.program_counter += GetInstructionLength(PUSH_ALL);
    return true;
}

static bool ExecutePop(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, POP))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return false;
    }
    
    if (StackIsEmpty(vm))
    {
        return false;
    }
    
    uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    
    if (!IsValidRegisterIndex(register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return false;
    }
    
    int32_t datum = ReadWord(vm, GetProgramCounter(vm) + 2);
    vm->cpu.registers[register_index] = datum;
    vm->cpu.stack_pointer += 4;
    vm->cpu.program_counter += GetInstructionLength(POP);
    return true;
}

static bool ExecutePopAll(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, POP_ALL))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return false;
    }
    
    if (!CanPerformMultipop(vm))
    {
        vm->cpu.status.STACK_UNDERFLOW = 1;
        return false;
    }
    
    vm->cpu.registers[REG4] = ReadWord(vm, vm->cpu.stack_pointer);
    vm->cpu.registers[REG3] = ReadWord(vm, vm->cpu.stack_pointer + 4);
    vm->cpu.registers[REG2] = ReadWord(vm, vm->cpu.stack_pointer + 8);
    vm->cpu.registers[REG1] = ReadWord(vm, vm->cpu.stack_pointer + 12);
    vm->cpu.stack_pointer += 16;
    vm->cpu.program_counter += GetInstructionLength(POP_ALL);
    return true;
}

static bool ExecuteLSP(TOYVM* vm)
{
    if (!InstructionFitsInMemory(vm, LSP))
    {
        vm->cpu.status.BAD_ACCESS = 1;
        return false;
    }
    
    uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
    
    if (!IsValidRegisterIndex(register_index))
    {
        vm->cpu.status.INVALID_REGISTER_INDEX = 1;
        return false;
    }
    
    vm->cpu.registers[register_index] = vm->cpu.stack_pointer;
    vm->cpu.program_counter += GetInstructionLength(LSP);
    return true;
}

void PrintStatus(TOYVM* vm)
{
    printf("HALT_BAD_INSTRUCTION  : %d\n", vm->cpu.status.HALT_BAD_INSTRUCTION);
    printf("STACK_UNDERFLOW       : %d\n", vm->cpu.status.STACK_UNDERFLOW);
    printf("STACK_OVERFLOW        : %d\n", vm->cpu.status.STACK_OVERFLOW);
    printf("INVALID_REGISTER_INDEX: %d\n",
           vm->cpu.status.INVALID_REGISTER_INDEX);
    
    printf("BAD_ACCESS            : %d\n", vm->cpu.status.BAD_ACCESS);
    printf("COMPARISON_ABOVE      : %d\n", vm->cpu.status.COMPARISON_ABOVE);
    printf("COMPARISON_EQUAL      : %d\n", vm->cpu.status.COMPARISON_EQUAL);
    printf("COMPARISON_BELOW      : %d\n", vm->cpu.status.COMPARISON_BELOW);
}

void RunVM(TOYVM* vm)
{
    uint8_t opcode = NOP;
    
    while (opcode != HALT)
    {
        opcode = vm->memory[vm->cpu.program_counter];
        
        switch (opcode)
        {
            case ADD:
                if (!ExecuteAdd(vm)) return;
                
                break;
                
            case NEG:
                if (!ExecuteNeg(vm)) return;
                
                break;
                
            case MUL:
                if (!ExecuteMul(vm)) return;
                
                break;
                
            case DIV:
                if (!ExecuteDiv(vm)) return;
                
                break;
                
            case MOD:
                if (!ExecuteMod(vm)) return;
                
                break;
                
            case CMP:
                if (!ExecuteCmp(vm)) return;
                
                break;
                
            case JA:
                if (!ExecuteJumpIfAbove(vm)) return;
                
                break;
                
            case JE:
                if (!ExecuteJumpIfEqual(vm)) return;
                
                break;
                
            case JB:
                if (!ExecuteJumpIfBelow(vm)) return;
                
                break;
                
            case JMP:
                if (!ExecuteJump(vm)) return;
                
                break;
                
            case CALL:
                if (!ExecuteCallL(vm)) return;
                
                break;
                
            case RET:
                if (!ExecuteRet(vm)) return;
                
                break;
                
            case LOAD:
                if (!ExecuteLoad(vm)) return;
                
                break;
                
            case STORE:
                if (!ExecuteStore(vm)) return;
                
                break;
                
            case CONST:
                if (!ExecuteConst(vm)) return;
                
                break;
                
            case PUSH:
                if (!ExecutePush(vm)) return;
                
                break;
                
            case PUSH_ALL:
                if (!ExecutePushAll(vm)) return;
                
                break;
                
            case POP:
                if (!ExecutePop(vm)) return;
                
                break;
                
            case POP_ALL:
                if (!ExecutePopAll(vm)) return;
                
                break;
                
            case LSP:
                if (!ExecuteLSP(vm)) return;
                
                break;
                
            case HALT:
                return;
                
            case INT:
                if (!ExecuteInterrupt(vm)) return;
                
                break;
                
            case NOP:
                /* Do nothing. */
                break;
                
            default:
                vm->cpu.status.HALT_BAD_INSTRUCTION = 1;
                return;
        }
    }
}

