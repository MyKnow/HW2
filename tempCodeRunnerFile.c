Memory readMipsBinary(const char* filename) {
    FILE* file = fopen(filename, "rb"); // 이진 읽기 모드로 파일 열기

    Memory mem;
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

void processInstructions(const uint32_t* mem, size_t length) {
    for (size_t i = 0; i < length; i++) {
        uint32_t instruction = mem[i];

        // MIPS 명령어 구조체에 저장
        R_Instruction r_inst;
        r_inst.opcode = (instruction >> 26) & 0x3F;
        r_inst.rs = (instruction >> 21) & 0x1F;
        r_inst.rt = (instruction >> 16) & 0x1F;
        r_inst.rd = (instruction >> 11) & 0x1F;
        r_inst.shamt = (instruction >> 6) & 0x1F;
        r_inst.funct = instruction & 0x3F;

        I_Instruction i_inst;
        i_inst.opcode = (instruction >> 26) & 0x3F;
        i_inst.rs = (instruction >> 21) & 0x1F;
        i_inst.rt = (instruction >> 16) & 0x1F;
        i_inst.immediate = instruction & 0xFFFF;

        J_Instruction j_inst;
        j_inst.opcode = (instruction >> 26) & 0x3F;
        j_inst.address = instruction & 0x3FFFFFF;

        // 예시 출력
        printf("Binary: 0x%08X\n", instruction);
        printf("R: opcode = 0x%02X, rs = 0x%02X, rt = 0x%02X, rd = 0x%02X, shamt = 0x%02X, funct = 0x%02X\n",
               r_inst.opcode, r_inst.rs, r_inst.rt, r_inst.rd, r_inst.shamt, r_inst.funct);
        printf("I: opcode = 0x%02X, rs = 0x%02X, rt = 0x%02X, immediate = 0x%04X\n",
               i_inst.opcode, i_inst.rs, i_inst.rt, i_inst.immediate);
        printf("J: opcode = 0x%02X, address = 0x%06X\n", j_inst.opcode, j_inst.address);
        printf("\n");
    }
}