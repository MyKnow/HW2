#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define CHUNK_SIZE 4 // 32비트 = 4바이트
#define MEM_SIZE 200

u_int8_t pc = 0;

// 메모리 구조체
typedef struct {
    uint32_t data[MEM_SIZE];
    size_t numChunks;
} InstMem;

// R 형식 명령어 구조체
typedef struct {
    uint32_t opcode : 6;
    uint32_t rs : 5;
    uint32_t rt : 5;
    uint32_t rd : 5;
    uint32_t shamt : 5;
    uint32_t funct : 6;
} R_Instruction;

// I 형식 명령어 구조체
typedef struct {
    uint32_t opcode : 6;
    uint32_t rs : 5;
    uint32_t rt : 5;
    uint32_t immediate : 16;
} I_Instruction;

// J 형식 명령어 구조체
typedef struct {
    uint32_t opcode : 6;
    uint32_t address : 26;
} J_Instruction;

// 이진 파일에서 데이터를 읽은 후 빅 엔디안 형태로 변환하여 명령어 메모리 구조체의 32비트 data에 저장하는 함수
InstMem readMipsBinary(const char* filename);

// mem의 data에서 바이너리를 읽고 명령어의 형식을 구분하는 함수
void processInstructions(const uint32_t* mem, size_t length);

int main() {
    // 모든 명령어를 Instruction Memory에 저장하기
    InstMem mem = readMipsBinary("input.bin");



    processInstructions(mem.data, mem.numChunks);

    return 0;
}

InstMem readMipsBinary(const char* filename) {
    FILE* file = fopen(filename, "rb"); // 이진 읽기 모드로 파일 열기

    InstMem mem;
    mem.numChunks = 0; // 초기화

    if (file == NULL) {
        printf("파일을 열 수 없습니다.");
        return mem;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 바이너리 수 파악
    mem.numChunks = file_size / CHUNK_SIZE;

    if (mem.numChunks > MEM_SIZE) {
        printf("메모리 공간이 부족합니다.");
        fclose(file);
        return mem;
    }

    size_t result = fread(mem.data, CHUNK_SIZE, mem.numChunks, file);
    if (result != mem.numChunks) {
        printf("파일을 읽을 수 없습니다.");
        fclose(file);
        return mem;
    }

    fclose(file); // 파일 닫기

    // 빅 엔디안으로 변환
    for (size_t i = 0; i < mem.numChunks; i++) {
        mem.data[i] = ((mem.data[i] >> 24) & 0xFF) | ((mem.data[i] >> 8) & 0xFF00) | ((mem.data[i] << 8) & 0xFF0000) | ((mem.data[i] << 24) & 0xFF000000);
    }

    return mem;
}

void processInstructions(const uint32_t* instructions, size_t length) {
    for (size_t i = 0; i < length; i++) {
        uint32_t instruction = instructions[i];

        // 타입 구분 및 출력
        if (((instruction >> 26) & 0x3F) == 0) {
            printf("R 타입 명령어\n");

            R_Instruction r_inst;
            r_inst.opcode = (instruction >> 26) & 0x3F;
            r_inst.rs = (instruction >> 21) & 0x1F;
            r_inst.rt = (instruction >> 16) & 0x1F;
            r_inst.rd = (instruction >> 11) & 0x1F;
            r_inst.shamt = (instruction >> 6) & 0x1F;
            r_inst.funct = instruction & 0x3F;

            printf("Binary: 0x%08X\n", instruction);
            printf("opcode = 0x%02X, rs = 0x%02X, rt = 0x%02X, rd = 0x%02X, shamt = 0x%02X, funct = 0x%02X\n",
                   r_inst.opcode, r_inst.rs, r_inst.rt, r_inst.rd, r_inst.shamt, r_inst.funct);
        } else if (((instruction >> 26) & 0x3F) == 2 || ((instruction >> 26) & 0x3F) == 3) {
            printf("J 타입 명령어\n");

            J_Instruction j_inst;
            j_inst.opcode = (instruction >> 26) & 0x3F;
            j_inst.address = instruction & 0x3FFFFFF;

            printf("Binary: 0x%08X\n", instruction);
            printf("opcode = 0x%02X, address = 0x%06X\n", j_inst.opcode, j_inst.address);
        } else {
            printf("I 타입 명령어\n");

            I_Instruction i_inst;
            i_inst.opcode = (instruction >> 26) & 0x3F;
            i_inst.rs = (instruction >> 21) & 0x1F;
            i_inst.rt = (instruction >> 16) & 0x1F;
            i_inst.immediate = instruction & 0xFFFF;

            printf("Binary: 0x%08X\n", instruction);
            printf("opcode = 0x%02X, rs = 0x%02X, rt = 0x%02X, immediate = 0x%04X\n",
                   i_inst.opcode, i_inst.rs, i_inst.rt, i_inst.immediate);
        }
        printf("\n");
    }
}
