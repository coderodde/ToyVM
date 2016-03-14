#include <stdio.h>
#include "toyvm.h"

static size_t getFileSize(FILE* file)
{
    long int original_cursor = ftell(file);
    fseek(file, 0L, SEEK_END);
    size_t size = ftell(file);
    fseek(file, original_cursor, SEEK_SET);
    return size;
}

int main(int argc, const char * argv[]) {
    if (argc != 2)
    {
        puts("Usage: toy FILE.brick\n");
        return 0;
    }
    
    FILE* file = fopen(argv[1], "r");
    
    if (!file)
    {
        printf("ERROR: cannot read file \"%s\".", argv[1]);
        return (EXIT_FAILURE);
    }
    
    size_t file_size = getFileSize(file);
    
    TOYVM vm;
    InitializeVM(&vm, 2 * file_size, file_size);
    
    fread(vm.memory, 1, file_size, file);
    fclose(file);

    RunVM(&vm);
    
    if (vm.cpu.status.BAD_ACCESS
        || vm.cpu.status.BAD_INSTRUCTION
        || vm.cpu.status.INVALID_REGISTER_INDEX
        || vm.cpu.status.STACK_OVERFLOW
        || vm.cpu.status.STACK_UNDERFLOW)
    {
        PrintStatus(&vm);
    }
}
