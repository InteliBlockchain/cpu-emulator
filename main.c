#include <stdint.h>   // Includes fixed-width integer types like uint8_t and uint16_t
#include <stdio.h>    // Includes standard input/output functions like printf
#include <assert.h>   // Includes the assert macro for runtime assertions

// Define a structure named 'CPU' to represent the CPU state
typedef struct {
    uint16_t current_operation; // 16-bit unsigned integer to store the current opcode
    uint8_t registers[2];       // An array of two 8-bit unsigned integers acting as CPU registers
} CPU;

// Function to read the current opcode from the CPU
uint16_t read_opcode(CPU *cpu) {
    return cpu->current_operation; // Return the opcode stored in the CPU
}

// Function to add the values of two registers and store the result in the first register
void add_xy(CPU *cpu, uint8_t x, uint8_t y) {
    cpu->registers[x] += cpu->registers[y]; // Add the value of register y to register x
}

// Function to execute the current opcode by decoding and performing the appropriate operation
void run(CPU *cpu) {
    uint16_t opcode = read_opcode(cpu); // Retrieve the opcode from the CPU

    // Decode the opcode into its constituent parts using bitwise operations
    uint8_t c = (opcode & 0xF000) >> 12; // Extract the highest 4 bits (bits 12-15)
    uint8_t x = (opcode & 0x0F00) >> 8;  // Extract bits 8-11
    uint8_t y = (opcode & 0x00F0) >> 4;  // Extract bits 4-7
    uint8_t d = opcode & 0x000F;         // Extract the lowest 4 bits (bits 0-3)

    // Check if the opcode matches the pattern for an ADD operation (0x8xy4)
    if (c == 0x8 && d == 0x4) {
        add_xy(cpu, x, y); // Perform the addition of registers x and y
    } else {
        // If the opcode is not recognized, print an error message
        printf("Unhandled opcode: 0x%04X\n", opcode);
    }
}

// The main function where the program execution begins
int main() {
    CPU cpu = {0}; // Initialize the CPU structure with zeros for all members

    // Set up the CPU state
    cpu.current_operation = 0x8014; // Set the opcode to perform ADD V0, V1 (opcode 0x8xy4)
    cpu.registers[0] = 5;  // Initialize register V0 with the value 5
    cpu.registers[1] = 10; // Initialize register V1 with the value 10

    run(&cpu); // Execute the opcode by running the CPU

    assert(cpu.registers[0] == 15); // Verify that the addition result is correct

    // Print the result of the addition
    printf("5 + 10 = %d\n", cpu.registers[0]);

    return 0; // Return 0 to indicate successful execution
}
