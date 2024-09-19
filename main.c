#include <stdint.h>   // Includes fixed-width integer types like uint8_t and uint16_t
#include <stdio.h>    // Includes standard input/output functions like printf
#include <assert.h>   // Includes the assert macro for runtime checks
#include <stdlib.h>   // Includes standard library functions like exit

// Define a CPU structure to represent the state of the emulator
typedef struct {
    uint8_t  registers[16];       // An array of 16 8-bit general-purpose registers (V0 to VF)
    size_t   position_in_memory;  // The current position (address) in the memory array
    uint8_t  memory[4096];        // Memory array of 4096 bytes (addresses 0x000 to 0xFFF)
    uint16_t stack[16];           // A stack for storing return addresses (used by CALL and RET)
    size_t   stack_pointer;       // Points to the next free slot in the stack
} CPU;

// Function to read a 16-bit opcode from memory at the current position
uint16_t read_opcode(CPU *cpu) {
    size_t p = cpu->position_in_memory;        // Get the current memory position

    // Read two consecutive bytes and combine them into a 16-bit opcode
    uint16_t op_byte1 = cpu->memory[p];        // First byte (higher-order bits)
    uint16_t op_byte2 = cpu->memory[p + 1];    // Second byte (lower-order bits)

    // Combine the two bytes into one 16-bit opcode
    return (op_byte1 << 8) | op_byte2;         // Shift op_byte1 left by 8 bits and OR with op_byte2
}

// Function to perform a subroutine call to the specified address
void call(CPU *cpu, uint16_t addr) {
    size_t sp = cpu->stack_pointer;  // Get the current stack pointer

    // Check for stack overflow
    if (sp >= sizeof(cpu->stack) / sizeof(cpu->stack[0])) {
        printf("Stack overflow!\n");
        exit(EXIT_FAILURE);  // Terminate the program on stack overflow
    }

    // Push the current position onto the stack
    cpu->stack[sp] = cpu->position_in_memory;  // Save the return address
    cpu->stack_pointer += 1;                   // Increment the stack pointer

    // Jump to the subroutine address
    cpu->position_in_memory = addr;            // Set the program counter to the subroutine address
}

// Function to return from a subroutine
void ret(CPU *cpu) {
    // Check for stack underflow
    if (cpu->stack_pointer == 0) {
        printf("Stack underflow!\n");
        exit(EXIT_FAILURE);  // Terminate the program on stack underflow
    }

    cpu->stack_pointer -= 1;                      // Decrement the stack pointer
    uint16_t addr = cpu->stack[cpu->stack_pointer];  // Retrieve the return address
    cpu->position_in_memory = addr;               // Set the program counter to the return address
}

// Function to add the values in registers Vx and Vy, handling overflow
void add_xy(CPU *cpu, uint8_t x, uint8_t y) {
    uint8_t arg1 = cpu->registers[x];  // Get the value from register Vx
    uint8_t arg2 = cpu->registers[y];  // Get the value from register Vy

    // Perform addition and check for overflow
    uint16_t result = arg1 + arg2;     // Use a 16-bit integer to detect overflow

    cpu->registers[x] = (uint8_t)result;  // Store the lower 8 bits back in Vx

    // Set register VF to indicate overflow (carry flag)
    if (result > 0xFF) {
        cpu->registers[0xF] = 1;  // Set VF to 1 if there was an overflow
    } else {
        cpu->registers[0xF] = 0;  // Set VF to 0 if there was no overflow
    }
}


// Function to execute instructions in a loop
void run(CPU *cpu) {
    while (1) {  // Infinite loop until a return statement is encountered
        uint16_t opcode = read_opcode(cpu);   // Read the next opcode from memory

        cpu->position_in_memory += 2;         // Move to the next instruction (each opcode is 2 bytes)

        // Decode the opcode into its constituent parts using bitwise operations
        uint8_t c = (opcode & 0xF000) >> 12;  // Bits 12-15 (first nibble)
        uint8_t x = (opcode & 0x0F00) >> 8;   // Bits 8-11 (second nibble)
        uint8_t y = (opcode & 0x00F0) >> 4;   // Bits 4-7 (third nibble)
        uint8_t d = opcode & 0x000F;          // Bits 0-3 (fourth nibble)

        uint16_t nnn = opcode & 0x0FFF;       // Address (lower 12 bits)

        // Handle the opcode based on its decoded values
        if (c == 0 && x == 0 && y == 0 && d == 0) {
            // Opcode 0x0000: HALT instruction
            return;  // Exit the run loop (halt execution)
        } else if (c == 0 && x == 0 && y == 0xE && d == 0xE) {
            // Opcode 0x00EE: RET instruction
            ret(cpu);  // Return from subroutine
        } else if (c == 0x2) {
            // Opcode 0x2NNN: CALL instruction
            call(cpu, nnn);  // Call subroutine at address NNN
        } else if (c == 0x8 && d == 0x4) {
            // Opcode 0x8XY4: ADD Vx, Vy
            add_xy(cpu, x, y);  // Add value of Vy to Vx with overflow handling
        } else {
            // Handle unimplemented opcodes
            printf("Unhandled opcode: 0x%04X\n", opcode);
            exit(EXIT_FAILURE);  // Exit the program with a failure code
        }
    }
}

// The main function where the program execution begins
int main() {
    // Initialize the CPU structure with zeros
    CPU cpu = {0};

    cpu.position_in_memory = 0;  // Start execution at the beginning of memory

    // Initialize registers with given values
    cpu.registers[0] = 5;    // V0 = 5
    cpu.registers[1] = 16;   // V1 = 16 (adjusted to make the final sum 69)

    // Load instructions into memory
    uint8_t *mem = cpu.memory;  // Pointer to the memory array for convenience

    // Program instructions
    mem[0x000] = 0x21; mem[0x001] = 0x00;  // Address 0x0000: CALL 0x0100    // <1>
    mem[0x002] = 0x21; mem[0x003] = 0x00;  // Address 0x0002: CALL 0x0100    // <2>
    mem[0x004] = 0x00; mem[0x005] = 0x00;  // Address 0x0004: HALT           // <3>

    // Subroutine at address 0x0100
    mem[0x100] = 0x80; mem[0x101] = 0x14;  // Address 0x0100: ADD V0, V1     // <4>
    mem[0x102] = 0x80; mem[0x103] = 0x14;  // Address 0x0102: ADD V0, V1     // <5>
    mem[0x104] = 0x00; mem[0x105] = 0xEE;  // Address 0x0104: RET            // <6>

    run(&cpu);  // Start the CPU execution

    // After execution, check that the result is as expected
    assert(cpu.registers[0] == 69);  // V0 should now be 69 kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk

    // Print the result of the computation
    printf("5 + (16 * 2) + (16 * 2) = %d\n", cpu.registers[0]); // 69 kkkkkkkkkk

    return 0;  // Indicate successful program termination
}
