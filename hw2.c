#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#define CHUNK_SIZE 4 // 32비트 = 4바이트
#define MEM_SIZE 200

void pt(int n) {
    for (int i = 0; i < n; i++) printf("*");
    printf("\n");
}

// 명령어 메모리 구조체
typedef struct {
    u_int32_t data[MEM_SIZE];
    size_t numChunks;
} InstMem;

// 레지스터 이름 열거형
typedef enum {
    $zero = 0, $at, $v0, $v1, $a0, $a1, $a2, $a3, $t0, $t1, $t2, $t3, $t4, $t5, $t6, $t7, $s0, $s1, $s2, $s3, $s4, $s5, $s6, $s7, $t8, $t9, $k0, $k1, $gp, $sp, $fp, $ra
} RegName_enum;

// 레지스터 이름 문자형
const char* RegName_str[] = {
    "$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"
};

// 레지스터 구조체
typedef struct {
    u_int32_t val[32];
} Register;

// 반환용 출력값 구조체
typedef struct {
    u_int32_t readData1;
    u_int32_t readData2;
} RegData;

// 명령어 구조체
typedef struct {
    char format;                // R, I, O
    u_int8_t opcode : 6;        // Inst 31-26
    u_int8_t rs : 5;            // Inst 25-21
    u_int8_t rt : 5;            // Inst 20-16
    u_int8_t rd : 5;            // Inst 15-11
    u_int8_t shamt : 5;         // Inst 10-06
    u_int8_t funct : 6;         // Inst 05-00
    u_int32_t address : 26;     // Inst 25-00
    u_int16_t immediate : 16;   // Inst 15-00
} Instruction;

// Control 구조체
typedef struct {
    bool RegDst;
    bool ALUSrc; // ReadData2(false) 또는 Immediate(true) 둘 중 하나로 ALU에 반환
    bool MemtoReg;
    bool RegWrite;
    bool MemRead;
    bool MemWrite;
    bool Branch;
    bool Jump;
    bool JR;
    u_int8_t ALUOp;
} Control;

// ALU Control 열거형
typedef enum {
    AND, OR, ADD, SUB = 6, SLT, BCOND, BNECOND
} ALUEnum;

// ALU OP 열거형
typedef enum {
    LW_SW, BEQ, R_TYPE, BNE
} ALUOPEnum;

// 데이터 메모리 전역변수 2^32 크기까진 만들어지지 않아서 2^31로 크기를 낮춤
u_int32_t data[0xFFFFFFF];

// 이진 파일에서 데이터를 읽은 후 빅 엔디안 형태로 변환하여 명령어 메모리 구조체의 32비트 data에 저장하는 함수
InstMem readMipsBinary(const char* filename);

// 바이너리를 읽고 명령어로 변환하는 함수
Instruction processInstructions(u_int32_t binary, u_int32_t pc, Register reg);

// OPCODE를 읽고 컨트롤 구조체를 반환하는 함수
Control processControl(char format, u_int8_t opcode, u_int8_t funct);

// ALUOp와 funct를 읽고 ALU에 Control 값을 반환하는 함수
u_int8_t ALUControl(u_int8_t ALUOp, u_int8_t funct);

// 레지스터 값을 받고 데이터를 반환하는 함수
RegData processRegister(Register* reg, u_int8_t readReg1, u_int8_t readReg2, u_int8_t writeReg, u_int32_t writeData, bool regWrite);

// 2개의 32비트 데이터로 연산을 수행하고 반환하는 함수
u_int32_t processALU(u_int32_t readData1, u_int32_t readData2, u_int8_t aluControl, bool* Zero);

// 주소값과 데이터를 받고 데이터 메모리에 작업하는 함수
u_int32_t processData(u_int32_t address, u_int32_t writeData, bool memRead, bool memWrite);

// Jump address를 생성하는 함수
u_int32_t processJAddress(u_int32_t address, u_int32_t pc, u_int32_t readData1, u_int32_t offset, bool brTaken, bool jump, bool jr);

// rt와 rd 중 하나를 선택해서 writeRegister로 반환하는 함수
u_int8_t regMux(bool RegDst, u_int8_t rt, u_int8_t rd);

// 16비트의 immediate를 signExtend한 32비트 정수를 반환하는 함수
u_int32_t signExtend(u_int16_t immediate);

// readData2와 immdiate 중 하나를 선택해서 ALU로 반환하는 함수
u_int32_t aluMux(bool ALUSrc, u_int32_t readData, u_int32_t immediate);

// readData 또는 address(aluResult) 중 하나를 선택해서 register의 writeData로 반환하는 함수
u_int32_t memMux(bool MemtoReg, u_int32_t aluResult, u_int32_t readData);

// control의 branch와 alu의 zero 둘 다 true일 때 pcMux에 true 반환
bool pcAndgate(bool branch, bool zero);

// pc+4와 offset(pc+4 + imm<<2) 중 하나를 선택해서 jump Mux로 반환하는 함수
u_int32_t pcMux(bool PCSrc, u_int32_t addPC, u_int32_t offset);

// 각 사이클마다 변경 항목 출력
void printCycle(u_int32_t cle, u_int32_t pc, u_int32_t binary, Instruction inst, Register reg, u_int32_t bPC);

// 프로그램 결과 출력
void printResult();

int main() {
    // 모든 명령어를 Instruction Memory에 저장하기
    InstMem iMem = readMipsBinary("input.bin");

    // 레지스터를 선언하고 0으로 모두 초기화하고 ra, sp는 별도로 초기화 하기
    Register reg;
    memset(&reg, 0, sizeof(reg));
    reg.val[$ra] = 0xFFFFFFFF;
    reg.val[$sp] = 0x10000000;

    // 명령어 구조체 선언
    Instruction inst;

    // Control 구조체 선언
    Control ctr;

    // Registers에서 출력되는 readData 2개를 담는 구조체 선언
    RegData regResult;

    // 전역변수인 data를 0으로 초기화
    memset(&data, 0, sizeof(data));

    // pc count와 cycle count 선언 및 0으로 초기화
    u_int32_t pc = 0;
    u_int32_t cle = 0;

    while (pc != 0xFFFFFFFF) {
        // inst 초기화
        memset(&inst, 0x0, sizeof(inst));

        printf("\ncle [%d] (PC: 0x%X)\n", cle, pc);

        // 현재 pc가 가리키는 명령어 받아오기
        inst = processInstructions(iMem.data[pc/4], pc, reg);

        // opcode와 format을 읽고 Count가 반환할 값 초기화
        ctr = processControl(inst.format, inst.opcode, inst.funct);
        
        // 레지스터에서 값을 읽어서 반환함
        regResult = processRegister(&reg, inst.rs, inst.rt, inst.rd, 0, false);

        // 16bits immediate를 signExtend 하여 32비트로 확장
        u_int32_t extendImmediate = signExtend(inst.immediate);

        // readData2와 immediate 둘 중 하나를 ALU의 input 값으로 고름
        u_int32_t aluInput2 = aluMux(ctr.ALUSrc, regResult.readData2, extendImmediate);

        // aluControl 값 받아옴
        u_int8_t aluControl = ALUControl(ctr.ALUOp, inst.funct);

        // ALU 실행
        bool zero;
        u_int32_t aluResult = processALU(regResult.readData1, aluInput2, aluControl, &zero);

        // 메모리에 값을 읽거나 쓰고 읽은 경우 데이터 반환
        u_int32_t dataResult = processData(aluResult, regResult.readData2, ctr.MemRead, ctr.MemWrite);

        // dataResult나 aluResult 의 값을 조건에 따라 반환
        u_int32_t writeData = memMux(ctr.MemtoReg, aluResult, dataResult);

        // RegWrite가 true라면 레지스터에 값을 씀
        if (ctr.RegWrite == true) processRegister(&reg, inst.rs, inst.rt, regMux(ctr.RegDst, inst.rt, inst.rd), writeData, true);

        // 주어진 조건에 따라 pc 업데이트
        pc = processJAddress(inst.address, pc, regResult.readData1, extendImmediate, pcAndgate(ctr.Branch, zero), ctr.Jump, ctr.JR);

        // cycle count 1 증가
        cle++;

        // zero 초기화
        zero = false;
    }

    printf("%d\n", cle);

    return 0;
}

InstMem readMipsBinary(const char* filename) {
    FILE* file = fopen(filename, "rb"); // 이진 읽기 모드로 파일 열기

    InstMem mem;
    memset(&mem, 0, sizeof(mem)); // 초기화

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

Instruction processInstructions(u_int32_t binary, u_int32_t pc, Register reg) {
    Instruction inst;

    inst.opcode = (binary >> 26) & 0x3F;

    if (inst.opcode == 0) {
        inst.format = 'R';
        inst.opcode = (binary >> 26) & 0x3F;
        inst.rs = (binary >> 21) & 0x1F;
        inst.rt = (binary >> 16) & 0x1F;
        inst.rd = (binary >> 11) & 0x1F;
        inst.shamt = (binary >> 6) & 0x1F;
        inst.funct = binary & 0x3F;

        if (inst.funct == 0) inst.format = 'N'; // NOP
    } else if (((binary >> 26) & 0x3F) == 2 || ((binary >> 26) & 0x3F) == 3) {
        inst.format = 'J';
        inst.opcode = (binary >> 26) & 0x3F;
        inst.address = binary & 0x3FFFFFF;
    } else {
        inst.format = 'I';
        inst.opcode = (binary >> 26) & 0x3F;
        inst.rs = (binary >> 21) & 0x1F;
        inst.rt = (binary >> 16) & 0x1F;
        inst.immediate = binary & 0xFFFF;
    }


    if(inst.format == 'N') {
        printf("NOP\n");
        return inst;
    }
    printf("binary              || 0x%08x\n", binary);
    if (inst.format == 'R') {
        printf("processInstructions || type : R, opcode : 0x%X, rs : 0x%X (R[%d]=0x%X), rt : 0x%X (R[%d]=0x%X), rd : 0x%X (R[%d]=0x%X), shamt : 0x%X, funct : 0x%X\n", inst.opcode, inst.rs, inst.rs, reg.val[inst.rs], inst.rt, inst.rt, reg.val[inst.rt], inst.rd, inst.rd, reg.val[inst.rd], inst.shamt, inst.funct);
    }
    else if (inst.format == 'I') {
        printf("processInstructions || type : I, opcode : 0x%X, rs : 0x%X (R[%d]=0x%X), rt : 0x%X (R[%d]=0x%X), immediate : 0x%X\n", inst.opcode, inst.rs, inst.rs, reg.val[inst.rs], inst.rt, inst.rt, reg.val[inst.rt], inst.immediate);
    }
    else if (inst.format == 'J') {
        printf("processInstructions || type : J, opcode : 0x%X, rs : 0x%X (R[%d]=0x%X), Address : 0x%08x\n", inst.opcode, inst.rs, inst.rs, reg.val[inst.rs], inst.address);
    }

    return inst;
}

Control processControl(char format, u_int8_t opcode, u_int8_t funct) {
    Control ctr;

    // Control 유닛의 내부 값을 bool false(0)으로 초기화 하는 함수
    memset(&ctr, false, sizeof(ctr));

    // ALUOp는 따로 u_int8_t 0으로 초기화
    ctr.ALUOp = 0;

    char oper[10] = "";

    printf("processControl      || format : %c, opcode : %u, funct : 0x%x\n", format, opcode, funct);
    // R-Type 명령어
    if(format == 'R') {
        ctr.ALUSrc = false;
        // JR 명령어
        if (funct == 0x8) {
            ctr.JR = true;
        }
        // MOVE 명령어
        else if (funct == 0x25) {
            ctr.RegWrite = true;
            ctr.RegDst = true;
            ctr.ALUOp = R_TYPE;
        }
        // ADDU 명령어
        else if (funct == 0x21) {

        }
        else {
            ctr.RegDst = true;
            ctr.RegWrite = true;
            ctr.ALUOp = R_TYPE;
        }
    } 

    // I-Type 명령어
    else if (format == 'I') {
        ctr.ALUSrc = true;
        // ADDIU / LI 명령어
        if (opcode == 0x9) {
            ctr.RegDst = false;
            ctr.RegWrite = true;
            ctr.ALUOp = LW_SW;
        }
        // SW 명령어
        else if (opcode == 0x2b) {
            ctr.RegWrite = false;
            ctr.MemWrite = true;
            ctr.MemRead = false;
            ctr.ALUOp = LW_SW;
        }
        // BEQ 명령어
        else if (opcode == 0x4) {
            ctr.Branch = true;
            ctr.ALUSrc = false;
            ctr.ALUOp = BEQ;
        }
        // LW 명령어
        else if (opcode == 0x23) {
            ctr.MemtoReg = true;
            ctr.RegWrite = true;
            ctr.MemRead = true;
            ctr.MemWrite = false;
            ctr.ALUOp = LW_SW;
        }
        // SLTI 명령어
        else if (opcode == 0xa) {
            ctr.RegDst = true;
            ctr.RegWrite = true;
            ctr.ALUOp = 3;
        }
        // BNEZ 명령어
        else if (opcode == 0x5) {
            ctr.Branch = true;
            ctr.ALUSrc = false;
            ctr.ALUOp = BNE;
        }
    }
    // JAL 명령어
    else if (opcode == 0x3) {
        ctr.RegWrite = true;
    }
    // BNE 명령어
    else if (opcode == 0x5) {
        ctr.Branch = true;
    }

    return ctr;
}

u_int8_t ALUControl(u_int8_t ALUOp, u_int8_t funct) {
    u_int8_t control;
    funct -= 32;

    printf("ALUControl          || ALUOp : 0x%x, funct : 0x%x\n", ALUOp, funct);

    if (ALUOp == LW_SW) control = ADD;
    else if (ALUOp == BEQ) control = BCOND;
    else if (ALUOp == BNE) control = BNECOND;
    else {
        if (funct == 0x0 || funct == 0x25) control = ADD;
        else if (funct == 0x2) control = SUB;
        else if (funct == 0x4) control = AND;
        else if (funct == 0x5) control = OR;
        else if (funct == 0xa) control = SLT;
    }

    return control;
}

RegData processRegister(Register* reg, u_int8_t readReg1, u_int8_t readReg2, u_int8_t writeReg, u_int32_t writeData, bool regWrite) {
    RegData regData;

    printf("process Register    || readReg1 : %u, readReg2 : %u, writeReg : %u, writeData : 0x%x, regWrite : %d\n", readReg1, readReg2, writeReg, writeData, regWrite);

    if (regWrite == false) {
        regData.readData1 = reg->val[readReg1];
        regData.readData2 = reg->val[readReg2];
    } else {
        pt(5);
        printf("reg_Val = %08x\n", reg->val[writeReg]);
        reg->val[writeReg] = writeData;
        printf("reg_Val[%d] = %08x\n", writeReg, reg->val[writeReg]);
    }

    return regData;
}

u_int32_t processALU(u_int32_t readData1, u_int32_t readData2, u_int8_t aluControl, bool* Zero) {
    u_int32_t ALUResult;
    printf("processALU          || rD1 : 0x%x, rD2 : 0x%x, aCtr : %d, zero : %d, ", readData1, readData2, aluControl, *Zero);

    if (aluControl == ADD) ALUResult = readData1 + readData2;
    else if (aluControl == SUB)  ALUResult = readData1 - readData2;
    else if (aluControl == BCOND) {
        ALUResult = readData1 - readData2;
        if (ALUResult == 0) *Zero = true;
        else *Zero = false;
        ALUResult = readData2;
    }
    else if (aluControl == BNECOND) {
        ALUResult = readData1 - readData2;
        if (ALUResult != 0) *Zero = true;
        else *Zero = false;
        ALUResult = readData2;
    }
    else if (aluControl == AND) ALUResult = readData1 & readData2;
    else if (aluControl == OR) ALUResult = readData1 | readData2;
    else if (aluControl == SLT) ALUResult = readData1 < readData2;

    printf("ALUResult : 0x%x\n", ALUResult);

    return ALUResult;
}

u_int32_t processData(u_int32_t address, u_int32_t writeData, bool memRead, bool memWrite) {
    u_int32_t readData;

    printf("processData         || address : 0x%x, writeData : 0x%x, memRead : %d, memWrite : %d\n", address, writeData, memRead, memWrite);
    
    if (memRead == false && memWrite == true) {
        data[address] = writeData;
        printf("processData         || (memWrite) data[0x%08x] <- %u\n", address, writeData);
    }
    else if (memRead == true && memWrite == false) {
        readData = (u_int32_t)data[address];
        printf("processData         || (memRead) %u <- data[0x%08x]\n", readData, address);
    }

    return readData;
}

u_int32_t processJAddress(u_int32_t address, u_int32_t pc, u_int32_t readData1, u_int32_t offset, bool brTaken, bool jump, bool jr) {

    printf("processJAddress     || address : 0x%x, pc : 0x%x, readData1 : 0x%x, offset : 0x%x, brTaken : %d, jump : %d, jr : %d\n", address, pc, readData1, offset, brTaken, jump, jr);

    u_int32_t returnPC;
    
    u_int32_t addPC = pc + 4;
    
    u_int32_t jAddress = ((addPC & 0xF0000000) | (address << 2));

    u_int32_t pcALU = addPC + (offset << 2);

    returnPC = pcMux(brTaken, addPC, pcALU);
    if (jump == true) {
        returnPC = jAddress;
        printf("processJAddress     || (JUMP) PC < - 0x%08X\n", returnPC);
    } else if (jr == true) {
        returnPC = readData1;
        printf("processJAddress     || (JUMP) PC < - 0x%08X\n", returnPC);
    }
    else {
        printf("processJAddress     || PC < - 0x%08X = 0x%08X+4\n", returnPC, pc);
    }

    return returnPC;
}

u_int8_t regMux(bool RegDst, u_int8_t rt, u_int8_t rd) {
    if (RegDst == true) return rd;
    return rt;
}

u_int32_t signExtend(u_int16_t immediate) {
    u_int32_t extend = (u_int32_t)(int32_t)(int16_t)immediate;

    return extend;
}

u_int32_t aluMux(bool ALUSrc, u_int32_t readData, u_int32_t immediate) {
    printf("aluMux              || ALUSrc : %d, readData : 0x%x, immediate : 0x%x\n", ALUSrc, readData, immediate);
    if (ALUSrc == true) return immediate;
    return readData;
}

u_int32_t memMux(bool MemtoReg, u_int32_t aluResult, u_int32_t readData) {
    printf("memMux              || MemtoReg : %d, aluResult : 0x%x, readData : 0x%x\n", MemtoReg, aluResult, readData);
    if (MemtoReg == true) return readData;
    else return aluResult;
}

bool pcAndgate(bool branch, bool zero) {
    if (branch == true && zero == true) return true;
    return false;
}

u_int32_t pcMux(bool PCSrc, u_int32_t addPC, u_int32_t offset) {
    if (PCSrc == true) return offset;
    else return addPC;
}

void printCycle(u_int32_t cle, u_int32_t pc, u_int32_t binary, Instruction inst, Register reg, u_int32_t bPC) {
    printf("[Load] r[30] <- Mem[0x0003ffff] = 0x0\n");
}

void printResult() {
    printf("Return value (r2): 45\n");
    printf("Total Cycle: 146\n");
    printf("Executed 'R' instruction: 13\n");
    printf("Executed 'I' instruction: 101\n");
    printf("Executed 'J' instruction: 0\n");
    printf("Number of Branch Taken: 11\n");
    printf("Number of Memory Access Instructions: 66\n");
}