#include <stdint.h>   // For fixed-width integer types like uint8_t and uint16_t
#include <stdio.h>    // For input/output functions like printf
#include <assert.h>   // For the assert macro used in testing

// Define a CPU structure to represent the state of the emulator
typedef struct {
    uint8_t registers[16];       // An array of 16 8-bit registers (V0 to VF)
    size_t position_in_memory;   // The current position in the memory array
    uint8_t memory[0x1000];      // Memory array of size 4096 bytes (0x1000 in hex)
} CPU;

// Function to add the values in registers Vx and Vy, handling overflow
void add_xy(CPU *cpu, uint8_t x, uint8_t y) {
    uint8_t arg1 = cpu->registers[x];  // Get the value from register Vx
    uint8_t arg2 = cpu->registers[y];  // Get the value from register Vy

    // Perform addition and check for overflow
    uint16_t result = arg1 + arg2;     // Add the two values (using 16-bit to detect overflow)

    cpu->registers[x] = (uint8_t)result;  // Store the lower 8 bits back in Vx

    // Set register VF to indicate overflow (carry flag)
    if (result > 0xFF) {
        cpu->registers[0xF] = 1;  // Set VF to 1 if there was an overflow
    } else {
        cpu->registers[0xF] = 0;  // Set VF to 0 if there was no overflow
    }
}

// Function to read a 16-bit opcode from memory at the current position
uint16_t read_opcode(CPU *cpu) {
    size_t p = cpu->position_in_memory;        // Get the current memory position

    // Read two consecutive bytes and combine them into a 16-bit opcode
    uint16_t op_byte1 = cpu->memory[p];        // First byte (higher-order bits)
    uint16_t op_byte2 = cpu->memory[p + 1];    // Second byte (lower-order bits)

    // Combine the two bytes into one 16-bit opcode
    return (op_byte1 << 8) | op_byte2;         // Shift op_byte1 left by 8 bits and OR with op_byte2
}

// Function to execute instructions in a loop
void run(CPU *cpu) {
    while (1) {  // Infinite loop until a return statement is encountered
        uint16_t opcode = read_opcode(cpu);   // Read the next opcode from memory

        cpu->position_in_memory += 2;         // Move to the next instruction (each opcode is 2 bytes)

        // Decode the opcode into its constituent parts using bitwise operations
        uint8_t c = (opcode & 0xF000) >> 12;  // Bits 12-15
        uint8_t x = (opcode & 0x0F00) >> 8;   // Bits 8-11
        uint8_t y = (opcode & 0x00F0) >> 4;   // Bits 4-7
        uint8_t d = opcode & 0x000F;          // Bits 0-3

        // Handle the opcode based on its decoded values
        if (c == 0 && x == 0 && y == 0 && d == 0) {   // Opcode 0x0000
            return;  // Exit the run loop (halt execution)
        } else if (c == 0x8 && d == 0x4) {            // Opcode 0x8xy4 (ADD Vx, Vy)
            add_xy(cpu, x, y);  // Perform addition of Vx and Vy
        } else {
            // Handle unimplemented opcodes
            printf("Unhandled opcode: 0x%04X\n", opcode);
            // You can implement additional opcode handling here
        }
    }
}


int main() {
    // Initialize the CPU structure with zeros
    CPU cpu = {0};

    cpu.position_in_memory = 0;  // Start execution at the beginning of memory

    // Initialize registers with given values
    cpu.registers[0] = 5;   // V0 = 5
    cpu.registers[1] = 10;  // V1 = 10
    cpu.registers[2] = 10;  // V2 = 10   // <4>
    cpu.registers[3] = 10;  // V3 = 10   // <4>

    // Load instructions into memory
    uint8_t *mem = cpu.memory;  // Pointer to the memory array for convenience

    mem[0] = 0x80; mem[1] = 0x14;  // Instruction at address 0x0000: ADD V0, V1   // <5>
    mem[2] = 0x80; mem[3] = 0x24;  // Instruction at address 0x0002: ADD V0, V2   // <6>
    mem[4] = 0x80; mem[5] = 0x34;  // Instruction at address 0x0004: ADD V0, V3   // <7>
    mem[6] = 0x00; mem[7] = 0x00;  // Instruction at address 0x0006: HALT         // <3>

    run(&cpu);  // Start the CPU execution

    // After execution, check that the result is as expected
    assert(cpu.registers[0] == 35);  // V0 should now be 35

    // Print the result of the computation
    printf("5 + 10 + 10 + 10 = %d\n", cpu.registers[0]);

    return 0;  // Indicate successful program termination
}
