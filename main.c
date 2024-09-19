#include <stdint.h>   // For fixed-width integer types like uint8_t and uint16_t
#include <stdio.h>    // For input/output functions like printf
#include <stdlib.h>   // For functions like exit
#include <assert.h>   // For the assert macro used in testing

// Define a CPU structure to represent the state of the emulator
typedef struct {
    uint8_t registers[16];          // An array of 16 8-bit general-purpose registers (V0 to VF)
    size_t position_in_memory;      // Program counter ("PC")
    uint8_t memory[4096];           // Memory array of 4096 bytes (addresses 0x000 to 0xFFF)
    uint16_t stack[16];             // A stack for storing return addresses (used by CALL and RET)
    size_t stack_pointer;           // Points to the next free slot in the stack
} CPU;

// Function prototypes (think of this as interfaces)
void run(CPU *cpu);
void ld(CPU *cpu, uint8_t vx, uint8_t kk);
void add(CPU *cpu, uint8_t vx, uint8_t kk);
void se(CPU *cpu, uint8_t vx, uint8_t kk);
void sne(CPU *cpu, uint8_t vx, uint8_t kk);
void jmp(CPU *cpu, uint16_t addr);
void call(CPU *cpu, uint16_t addr);
void ret(CPU *cpu);
void add_xy(CPU *cpu, uint8_t x, uint8_t y);
void and_xy(CPU *cpu, uint8_t x, uint8_t y);
void or_xy(CPU *cpu, uint8_t x, uint8_t y);
void xor_xy(CPU *cpu, uint8_t x, uint8_t y);

// Function to execute instructions in a loop
void run(CPU *cpu) {
    while (1) {  // Infinite loop until a return statement is encountered
        // Fetch the opcode (16 bits) by combining two consecutive bytes from memory
        uint8_t op_byte1 = cpu->memory[cpu->position_in_memory];
        uint8_t op_byte2 = cpu->memory[cpu->position_in_memory + 1];
        uint16_t opcode = (op_byte1 << 8) | op_byte2;

        // Decode the opcode into its constituent parts using bitwise operations
        uint8_t x = (opcode & 0x0F00) >> 8;    // Bits 8-11: Register X
        uint8_t y = (opcode & 0x00F0) >> 4;    // Bits 4-7: Register Y
        uint8_t kk = opcode & 0x00FF;          // Bits 0-7: Immediate 8-bit value
        uint8_t op_minor = opcode & 0x000F;    // Bits 0-3: Minor opcode
        uint16_t addr = opcode & 0x0FFF;       // Bits 0-11: Address

        cpu->position_in_memory += 2;          // Move to the next instruction (each opcode is 2 bytes)

        // Decode and execute the opcode
        if (opcode == 0x0000) {
            // Opcode 0x0000: HALT instruction
            return;  // Exit the run loop (halt execution)
        } else if (opcode == 0x00E0) {
            // Opcode 0x00E0: CLEAR SCREEN (Not implemented)
        } else if (opcode == 0x00EE) {
            // Opcode 0x00EE: RET instruction
            ret(cpu);  // Return from subroutine
        } else if ((opcode & 0xF000) == 0x1000) {
            // Opcode 0x1NNN: JMP instruction
            jmp(cpu, addr);
        } else if ((opcode & 0xF000) == 0x2000) {
            // Opcode 0x2NNN: CALL instruction
            call(cpu, addr);
        } else if ((opcode & 0xF000) == 0x3000) {
            // Opcode 0x3XKK: SE Vx, KK
            se(cpu, x, kk);
        } else if ((opcode & 0xF000) == 0x4000) {
            // Opcode 0x4XKK: SNE Vx, KK
            sne(cpu, x, kk);
        } else if ((opcode & 0xF000) == 0x5000) {
            // Opcode 0x5XY0: SE Vx, Vy
            se(cpu, x, cpu->registers[y]);
        } else if ((opcode & 0xF000) == 0x6000) {
            // Opcode 0x6XKK: LD Vx, KK
            ld(cpu, x, kk);
        } else if ((opcode & 0xF000) == 0x7000) {
            // Opcode 0x7XKK: ADD Vx, KK
            add(cpu, x, kk);
        } else if ((opcode & 0xF000) == 0x8000) {
            // Opcode 0x8XYN: Arithmetic and logical operations
            switch (op_minor) {
                case 0x0:
                    // Opcode 0x8XY0: LD Vx, Vy
                    ld(cpu, x, cpu->registers[y]);
                    break;
                case 0x1:
                    // Opcode 0x8XY1: OR Vx, Vy
                    or_xy(cpu, x, y);
                    break;
                case 0x2:
                    // Opcode 0x8XY2: AND Vx, Vy
                    and_xy(cpu, x, y);
                    break;
                case 0x3:
                    // Opcode 0x8XY3: XOR Vx, Vy
                    xor_xy(cpu, x, y);
                    break;
                case 0x4:
                    // Opcode 0x8XY4: ADD Vx, Vy
                    add_xy(cpu, x, y);
                    break;
                default:
                    printf("Unhandled opcode: 0x%04X\n", opcode);
                    exit(EXIT_FAILURE);
            }
        } else {
            // Unhandled opcode
            printf("Unhandled opcode: 0x%04X\n", opcode);
            exit(EXIT_FAILURE);
        }
    }
}

// Function to load a value into register Vx
void ld(CPU *cpu, uint8_t vx, uint8_t kk) {
    cpu->registers[vx] = kk;
}

// Function to add a value to register Vx
void add(CPU *cpu, uint8_t vx, uint8_t kk) {
    cpu->registers[vx] += kk;
}

// Function to skip next instruction if Vx equals kk
void se(CPU *cpu, uint8_t vx, uint8_t kk) {
    if (cpu->registers[vx] == kk) {
        cpu->position_in_memory += 2;  // Skip next instruction
    }
}

// Function to skip next instruction if Vx not equals kk
void sne(CPU *cpu, uint8_t vx, uint8_t kk) {
    if (cpu->registers[vx] != kk) {
        cpu->position_in_memory += 2;  // Skip next instruction
    }
}

// Function to jump to address
void jmp(CPU *cpu, uint16_t addr) {
    cpu->position_in_memory = addr;
}

// Function to call subroutine at address
void call(CPU *cpu, uint16_t addr) {
    if (cpu->stack_pointer >= sizeof(cpu->stack) / sizeof(cpu->stack[0])) {
        printf("Stack overflow!\n");
        exit(EXIT_FAILURE);
    }
    cpu->stack[cpu->stack_pointer++] = cpu->position_in_memory;
    cpu->position_in_memory = addr;
}

// Function to return from subroutine
void ret(CPU *cpu) {
    if (cpu->stack_pointer == 0) {
        printf("Stack underflow!\n");
        exit(EXIT_FAILURE);
    }
    cpu->position_in_memory = cpu->stack[--cpu->stack_pointer];
}

// Function to add Vy to Vx with carry flag
void add_xy(CPU *cpu, uint8_t x, uint8_t y) {
    uint16_t sum = cpu->registers[x] + cpu->registers[y];
    cpu->registers[x] = sum & 0xFF;
    cpu->registers[0xF] = (sum > 0xFF) ? 1 : 0; // Set carry flag VF
}

// Function to perform bitwise AND on Vx and Vy
void and_xy(CPU *cpu, uint8_t x, uint8_t y) {
    cpu->registers[x] &= cpu->registers[y];
}

// Function to perform bitwise OR on Vx and Vy
void or_xy(CPU *cpu, uint8_t x, uint8_t y) {
    cpu->registers[x] |= cpu->registers[y];
}

// Function to perform bitwise XOR on Vx and Vy
void xor_xy(CPU *cpu, uint8_t x, uint8_t y) {
    cpu->registers[x] ^= cpu->registers[y];
}

// The main function where the program execution begins
int main() {
    // Initialize the CPU structure with zeros
    CPU cpu = {0};

    cpu.position_in_memory = 0;  // Start execution at address 0

    // Initialize registers with given values
    cpu.registers[0] = 5;    // V0 = 5
    cpu.registers[1] = 10;   // V1 = 10

    // Load instructions into memory
    uint8_t *mem = cpu.memory;  // Pointer to the memory array for convenience

    // Main program
    mem[0x0000] = 0x21; mem[0x0001] = 0x00;  // Address 0x0000: CALL 0x0100
    mem[0x0002] = 0x21; mem[0x0003] = 0x00;  // Address 0x0002: CALL 0x0100

    // Subroutine at address 0x0100
    mem[0x0100] = 0x80; mem[0x0101] = 0x14;  // Address 0x0100: ADD V0, V1
    mem[0x0102] = 0x80; mem[0x0103] = 0x14;  // Address 0x0102: ADD V0, V1
    mem[0x0104] = 0x00; mem[0x0105] = 0xEE;  // Address 0x0104: ADD V0, V1

    run(&cpu);  // Start the CPU execution

    // After execution, check that the result is as expected
    assert(cpu.registers[0] == 45);  // V0 should now be 420

    // Print the result of the computation
    printf("0 + (70 * 3) + (70 * 3) = %d\n", cpu.registers[0]);

    return 0;  // Indicate successful program termination
}
