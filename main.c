#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#define CHUNK_SIZE 4 // 32비트 = 4바이트
#define MEM_SIZE 200

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
    u_int32_t readData1; // ReadRegister 1의 값
    u_int32_t readData2; // ReadRegister 2의 값
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
    bool RegDst; // regMux의 input으로, inst.rd(true) 또는 inst.rt(false)를 반환
    bool ALUSrc; // ReadData2(false) 또는 Immediate(true) 둘 중 하나로 ALU에 반환
    bool MemtoReg; // memMux의 input으로, readData(true) 또는 aluResult(false)로 반환
    bool RegWrite; // processRegister의 input으로, 레지스터를 모드를 결정함
    bool MemRead; // processData의 input으로, 메모리에서 값을 읽을 것인지(true) 안 읽을 것인지(false) 결정함
    bool MemWrite; // processData의 input으로, 메모리로 값을 쓸 것인지(true) 안 쓸 것인지(false) 결정함
    bool Branch; // pcAndgate의 input으로, (pc+4)+(imm<<2) 값을 pc로 할 것인지(true), (pc+4)로 할 것인지(false) 결정함
    bool Jump; // processJAddress의 input으로, pc를 Jump Address 값으로 할 것인지(true), pcAndgate의 값으로 할 것인지(false) 결정함
    bool JR; // processJAddress의 input으로, PC를 reg[$ra]값으로 할 것인지(true), 다른 값으로 할 것인지(false) 결정함
    u_int8_t ALUOp; // ALUOp의 input으로, opcode를 읽고 ALUControl의 값을 정함
} Control;

// ALU Control 열거형
typedef enum {
    AND, OR, ADD, SUB = 6, SLT, 
    BCOND,      // BEQ를 위한 컨트롤 값
    BNECOND,    // BNE를 위한 컨트롤 값 
    JAL         // JAL를 위한 컨트롤 값
} ALUEnum;

// ALU OP 열거형
typedef enum {
    LW_SW, BEQ, R_TYPE, BNE // BNE를 위한 ALUOp값
} ALUOPEnum;

// 데이터 메모리 전역변수 2^32 크기까진 만들어지지 않아서 2^28로 크기를 낮춤
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
u_int32_t processALU(u_int32_t readData1, u_int32_t readData2, u_int8_t aluControl, bool* Zero, u_int32_t pc);

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

// 프로그램 결과 출력
void printResult(u_int32_t reg_v0, u_int32_t cle, u_int8_t R_count, u_int8_t I_count, u_int8_t J_count, u_int8_t B_count, u_int8_t A_count);

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

    // R, I, J 명령어 수와 brTaken, mAccess 수를 카운트
    u_int8_t R_count = 0;
    u_int8_t I_count = 0;
    u_int8_t J_count = 0;
    u_int8_t B_count = 0;
    u_int8_t A_count = 0;
    u_int8_t N_count = 0;

    while (pc != 0xFFFFFFFF) {
        // cycle count 1 증가
        cle++;

        // inst 초기화
        memset(&inst, 0x0, sizeof(inst));

        printf("\ncle [%d] (PC: 0x%X)\n", cle, pc);

        // 현재 pc가 가리키는 명령어 받아오기
        inst = processInstructions(iMem.data[pc/4], pc, reg);

        // 각 명령어의 수 세기
        if (inst.format == 'R') R_count++;
        else if (inst.format == 'J') J_count++;
        else if (inst.format == 'I') I_count++;
        else {
            pc += 4;
            continue;
        }

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
        u_int32_t aluResult = processALU(regResult.readData1, aluInput2, aluControl, &zero, pc);

        // 메모리에 값을 읽거나 쓰고 읽은 경우 데이터 반환
        u_int32_t dataResult = processData(aluResult, regResult.readData2, ctr.MemRead, ctr.MemWrite);

        // dataResult나 aluResult 의 값을 조건에 따라 반환
        u_int32_t writeData = memMux(ctr.MemtoReg, aluResult, dataResult);

        // RegWrite가 true라면 레지스터에 값을 씀
        if (ctr.RegWrite == true) processRegister(&reg, inst.rs, inst.rt, regMux(ctr.RegDst, inst.rt, inst.rd), writeData, true);

        // 주어진 조건에 따라 pc 업데이트
        pc = processJAddress(inst.address, pc, regResult.readData1, extendImmediate, pcAndgate(ctr.Branch, zero), ctr.Jump, ctr.JR);

        // 각 명령어의 수 세기
        if (ctr.Branch == true) B_count++;
        if (ctr.MemRead == true || ctr.MemWrite == true) A_count++;

        // zero 초기화
        zero = false;
    }

    // 프로그램 결과 출력
    printResult(reg.val[$v0], cle, R_count, I_count, J_count, B_count, A_count);

    return 0;
}

InstMem readMipsBinary(const char* filename) {
    FILE* file = fopen(filename, "rb"); // 이진 읽기 모드로 파일 열기

    InstMem mem;
    memset(&mem, 0, sizeof(mem)); // 초기화

    if (file == NULL) {
        printf("파일을 열 수 없습니다.\n");
        exit(-1);
    }

    // 파일 포인터를 활용하여 파일의 크기를 계산하는 코드
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 바이너리 수 파악
    mem.numChunks = file_size / CHUNK_SIZE;

    // 메모리 사이즈보다 바이너리의 수가 많다면 쓰지 않고 앱 종료
    if (mem.numChunks > MEM_SIZE) {
        printf("메모리 공간이 부족합니다.\n");
        fclose(file);
        exit(0);
    }
    // mem.data에 32비트 크기로 데이터 청크의 수만큼 CHUNK_SIZE 크기의 데이터를 한 번에 반환
    size_t result = fread(mem.data, CHUNK_SIZE, mem.numChunks, file);
    if (result != mem.numChunks) {
        printf("파일을 읽을 수 없습니다.\n");
        fclose(file);
        exit(-1);
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

    // opcode 먼저 계산
    inst.opcode = (binary >> 26) & 0x3F;

    // R 명령어 분리
    if (inst.opcode == 0) {
        inst.format = 'R';
        inst.opcode = (binary >> 26) & 0x3F;
        inst.rs = (binary >> 21) & 0x1F;
        inst.rt = (binary >> 16) & 0x1F;
        inst.rd = (binary >> 11) & 0x1F;
        inst.shamt = (binary >> 6) & 0x1F;
        inst.funct = binary & 0x3F;

        if (inst.funct == 0) inst.format = 'N'; // NOP
    } 
    // J 명령어 분리
    else if (((binary >> 26) & 0x3F) == 2 || ((binary >> 26) & 0x3F) == 3) {
        inst.format = 'J';
        inst.opcode = (binary >> 26) & 0x3F;

        // Jump
        if (inst.opcode == 2) {

        }
        // Juml And Link
        else if (inst.opcode == 3) {
            inst.rs = $ra;
            inst.rd = $ra;
        }
        inst.address = binary & 0x3FFFFFF;
    } 
    // I 명령어 분리
    else {
        inst.format = 'I';
        inst.opcode = (binary >> 26) & 0x3F;
        inst.rs = (binary >> 21) & 0x1F;
        inst.rt = (binary >> 16) & 0x1F;
        inst.immediate = binary & 0xFFFF;
    }

    // N이라면 출력하지 않고 넘어감
    if(inst.format == 'N') {
        printf("NOP\n");
        return inst;
    }
    printf("binary              || 0x%08x\n", binary);
    if (inst.format == 'R') {
        printf("processInstructions || type : R, opcode : 0x%X, rs : 0x%X (R[%s]=0x%X), rt : 0x%X (R[%s]=0x%X), rd : 0x%X (R[%s]=0x%X), shamt : 0x%X, funct : 0x%X\n", inst.opcode, inst.rs, RegName_str[inst.rs], reg.val[inst.rs], inst.rt, RegName_str[inst.rt], reg.val[inst.rt], inst.rd, RegName_str[inst.rd], reg.val[inst.rd], inst.shamt, inst.funct);
    }
    else if (inst.format == 'I') {
        printf("processInstructions || type : I, opcode : 0x%X, rs : 0x%X (R[%s]=0x%X), rt : 0x%X (R[%s]=0x%X), immediate : 0x%X\n", inst.opcode, inst.rs, RegName_str[inst.rs], reg.val[inst.rs], inst.rt, RegName_str[inst.rt], reg.val[inst.rt], inst.immediate);
    }
    else if (inst.format == 'J') {
        printf("processInstructions || type : J, opcode : 0x%X, rs : 0x%X (R[%s]=0x%X), Address : 0x%08x\n", inst.opcode, inst.rs, RegName_str[inst.rs], reg.val[inst.rs], inst.address);
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
            ctr.RegDst = false;
            ctr.RegWrite = true;
            ctr.ALUOp = LW_SW;
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
    // J-Type 명령어
    else if (format == 'J') {
        // JAL 명령어
        if (opcode == 0x3) {
            ctr.RegWrite = true;
            ctr.Jump = true;
            ctr.RegDst = true;
            ctr.ALUOp = JAL;
        }
    }
    // BNE 명령어
    else if (opcode == 0x5) {
        ctr.Branch = true;
    }

    return ctr;
}

u_int8_t ALUControl(u_int8_t ALUOp, u_int8_t funct) {
    u_int8_t control;

    // funct의 최상위 비트를 제거하여 연산을 쉽게 함
    funct -= 32;

    printf("ALUControl          || ALUOp : 0x%x, funct : 0x%x\n", ALUOp, funct);

    // opcode와 funct에 따라 control값을 다르게 줌
    if (ALUOp == LW_SW) control = ADD;
    else if (ALUOp == BEQ) control = BCOND;
    else if (ALUOp == BNE) control = BNECOND;
    else if (ALUOp == JAL) control = JAL;
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

    printf("processRegister     || readReg1 : %s, readReg2 : %s, writeReg : %s, writeData : 0x%x, regWrite : %d\n", RegName_str[readReg1], RegName_str[readReg2], RegName_str[writeReg], writeData, regWrite);

    // regWrite가 false일 경우 값을 읽기만 함
    if (regWrite == false) {
        regData.readData1 = reg->val[readReg1];
        regData.readData2 = reg->val[readReg2];
        printf("processRegister(r)  || (reg_read) rD1 : reg[%s] = 0x%08x, rD2 : reg[%s] = 0x%08x\n", RegName_str[readReg1], reg->val[readReg1], RegName_str[readReg2], reg->val[readReg2]);
    } 
    // regWrite가 true일 경우 레지스터에 값을 씀
    else {
        reg->val[writeReg] = writeData;
        printf("processRegister(r)  || (reg_write) reg[%s] <- 0x%08x\n", RegName_str[writeReg], reg->val[writeReg]);
    }

    return regData;
}

u_int32_t processALU(u_int32_t readData1, u_int32_t readData2, u_int8_t aluControl, bool* Zero, u_int32_t pc) {
    u_int32_t ALUResult;
    printf("processALU          || rD1 : 0x%x, rD2 : 0x%x, aCtr : %d, zero : %d, ", readData1, readData2, aluControl, *Zero);

    // ALUControl 값에 따라 ALU가 연산을 다르게 함
    if (aluControl == ADD) ALUResult = readData1 + readData2;
    else if (aluControl == SUB)  ALUResult = readData1 - readData2;
    // BrTaken에 쓰일 Zero 자료형으로 인해  BCOND라는 컨트롤값을 하나 생성함
    else if (aluControl == BCOND) {
        ALUResult = readData1 - readData2;
        if (ALUResult == 0) *Zero = true;
        else *Zero = false;
        ALUResult = readData2;
    }
    // BNE 명령어를 위해 BrTaken에 쓰일 Zero 자료형으로 인해  BNECOND라는 컨트롤값을 하나 생성함
    else if (aluControl == BNECOND) {
        ALUResult = readData1 - readData2;
        if (ALUResult != 0) *Zero = true;
        else *Zero = false;
        ALUResult = readData2;
    }
    // JAL에 쓰일 명령어를 위해 컨트롤값을 하나 생성함
    else if (aluControl == JAL) {
        ALUResult = pc + 8;
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
    
    // memRead가 false고 memWrite가 true일 경우 데이터 메모리는 쓰기 모드
    if (memRead == false && memWrite == true) {
        data[address] = writeData;
        printf("processData(r)      || (Store) data[0x%08x] <- %u\n", address, writeData);
    }
    // memRead가 true고 memWrite가 false일 경우 데이터 메모리는 읽기 모드
    else if (memRead == true && memWrite == false) {
        readData = (u_int32_t)data[address];
        printf("processData(r)      || (Load) %u <- data[0x%08x]\n", readData, address);
    }

    return readData;
}

u_int32_t processJAddress(u_int32_t address, u_int32_t pc, u_int32_t readData1, u_int32_t offset, bool brTaken, bool jump, bool jr) {

    printf("processJAddress     || address : 0x%x, pc : 0x%x, readData1 : 0x%x, offset : 0x%x, brTaken : %d, jump : %d, jr : %d\n", address, pc, readData1, offset, brTaken, jump, jr);

    // 반환용 변수
    u_int32_t returnPC;
    
    // ADD ALU를 거친 pc값
    u_int32_t addPC = pc + 4;
    
    // J 명령어를 위한 jAddress 변수
    u_int32_t jAddress = ((addPC & 0xF0000000) | (address << 2));

    // branch를 위한 pcALU 변수
    u_int32_t pcALU = addPC + (offset << 2);

    // brTaken이 true라면 ALU 결과를, 아니라면 pc+4를 반환
    returnPC = pcMux(brTaken, addPC, pcALU);

    // jump가 true라면 returnPC를 아예 jAddress로 초기화
    if (jump == true) {
        returnPC = jAddress;
        printf("processJAddress(r)  || (JUMP) PC < - 0x%08X\n", returnPC);
    } 
    // jr이 true라면 readData1($ra)로 초기화
    else if (jr == true) {
        returnPC = readData1;
        printf("processJAddress(r)  || (JR) PC < - reg[$ra] = 0x%08X\n", returnPC);
    }
    // 아무것도 해당 안된다면 brTaken에 따라 바뀜
    else {
        printf("processJAddress(r)  || PC < - 0x%08X = 0x%08X+4\n", returnPC, pc);
    }

    return returnPC;
}

u_int8_t regMux(bool RegDst, u_int8_t rt, u_int8_t rd) {
    // RegDst가 true라면 WriteRegister에 rd값을 사용, 아니라면 rt값을 사용
    if (RegDst == true) return rd;
    return rt;
}

u_int32_t signExtend(u_int16_t immediate) {
    // 16bit immediate값을 32bit로 signExtend
    u_int32_t extend = (u_int32_t)(int32_t)(int16_t)immediate;

    return extend;
}

u_int32_t aluMux(bool ALUSrc, u_int32_t readData, u_int32_t immediate) {
    printf("aluMux              || ALUSrc : %d, readData : 0x%x, immediate : 0x%x\n", ALUSrc, readData, immediate);
    
    // ALUSrc가 true라면 ALU Input2에 signExtend 32bit 사용, false라면 readData2 사용
    if (ALUSrc == true) return immediate;
    return readData;
}

u_int32_t memMux(bool MemtoReg, u_int32_t aluResult, u_int32_t readData) {
    u_int32_t result;

    printf("memMux              || MemtoReg : %d, aluResult : 0x%x, readData : 0x%x, result : ", MemtoReg, aluResult, readData);

    // MemtoReg가 true라면 Data Memory[address]값을 사용, false라면 ALUResult값 사용
    if (MemtoReg == true) result = readData;
    else result = aluResult;

    printf("0x%x\n", result);
    return result;
}

bool pcAndgate(bool branch, bool zero) {
    // branch와 zero가 둘 다 true라면 pcAndgate도 true
    if (branch == true && zero == true) return true;
    return false;
}

u_int32_t pcMux(bool PCSrc, u_int32_t addPC, u_int32_t offset) {
    // pcAndgate값이 true라면 ALUResult를, false라면 pc+4값 사용
    if (PCSrc == true) return offset;
    else return addPC;
}

void printResult(u_int32_t reg_v0, u_int32_t cle, u_int8_t R_count, u_int8_t I_count, u_int8_t J_count, u_int8_t B_count, u_int8_t A_count) {
    printf("\n===========================================\n");
    printf("| Return value (%s): 0x%x\n", RegName_str[$v0], reg_v0);
    printf("| Total Cycle: %u\n", cle);
    printf("| Executed 'R' instruction: %u\n", R_count);
    printf("| Executed 'I' instruction: %u\n", I_count);
    printf("| Executed 'J' instruction: %u\n", J_count);
    printf("| Number of Branch Taken: %u\n", B_count);
    printf("| Number of Memory Access Instructions: %u\n", A_count);
    printf("===========================================\n");
}