#include "Assembler.hpp"
#include "DataControl.cpp"
#include "Pipeline.hpp"
#include "utils.hpp"

vector<int32_t> regs(32, 0);
vector<int32_t> memory(1024, 0);

void execute() {
    IF_ID if_id;
    ID_EX id_ex;
    EX_MEM ex_mem;
    MEM_WB mem_wb;

    int32_t PC = 0;
    bool finished = false;
    int cycle = 0;

    while (!finished) {
        cycle++;
        cout << "\nCycle " << cycle << " ----------------\n";

        // -------------------- WB stage --------------------
        if (!mem_wb.empty && mem_wb.regWrite && mem_wb.rd != 0) {
            regs[mem_wb.rd] = mem_wb.mem2reg ? mem_wb.memData : mem_wb.aluResult;
        }

        // -------------------- MEM stage --------------------
        MEM_WB new_mem_wb;
        new_mem_wb.empty = true;
        if (!ex_mem.empty) {
            new_mem_wb.rd = ex_mem.rd;
            new_mem_wb.regWrite = ex_mem.regWrite;
            new_mem_wb.mem2reg = ex_mem.mem2reg;

            if (ex_mem.memRead) {
                new_mem_wb.memData = memory[ex_mem.aluResult / 4];
            } else {
                if (ex_mem.memWrite) {
                    memory[ex_mem.aluResult / 4] = ex_mem.regData2;
                }
                new_mem_wb.memData = 0;
            }

            new_mem_wb.aluResult = ex_mem.aluResult;
            new_mem_wb.empty = false;

            // --- Handle branch/jump flush only when taken ---
            if (ex_mem.branchTaken) {
                PC = ex_mem.branchTarget;
                if_id.empty = true; // Flush next fetched instruction
                id_ex.empty = true; // Flush decode stage
            }
        }

        // -------------------- EX stage --------------------
        EX_MEM new_ex_mem;
        new_ex_mem.empty = true;
        if (!id_ex.empty) {
            int32_t op1 = id_ex.regData1;
            int32_t op2 = id_ex.ALUSrc ? id_ex.imm : id_ex.regData2;

            // --- Forwarding for rs1 ---
            if (!ex_mem.empty && ex_mem.regWrite && ex_mem.rd != 0 && ex_mem.rd == id_ex.rs1 && !ex_mem.memRead)
                op1 = ex_mem.aluResult;
            else if (!mem_wb.empty && mem_wb.regWrite && mem_wb.rd != 0 && mem_wb.rd == id_ex.rs1)
                op1 = mem_wb.mem2reg ? mem_wb.memData : mem_wb.aluResult;

            // --- Forwarding for rs2 ---
            if (!id_ex.ALUSrc) {
                if (!ex_mem.empty && ex_mem.regWrite && ex_mem.rd != 0 && ex_mem.rd == id_ex.rs2 && !ex_mem.memRead)
                    op2 = ex_mem.aluResult;
                else if (!mem_wb.empty && mem_wb.regWrite && mem_wb.rd != 0 && mem_wb.rd == id_ex.rs2)
                    op2 = mem_wb.mem2reg ? mem_wb.memData : mem_wb.aluResult;
            }

            int32_t result = alu(op1, op2, id_ex.opcode, id_ex.func7, id_ex.func3);

            // --- Branch / Jump handling ---
            bool branchTaken = false;
            int32_t branchTarget = id_ex.PC + id_ex.imm;

            if (id_ex.opcode == "1100011") { // BEQ
                if (id_ex.func3 == 0b000 && op1 == op2)
                    branchTaken = true;
            } 
            else if (id_ex.opcode == "1101111") { // JAL
                branchTaken = true;
                result = id_ex.PC + 4; // link value
            }

            new_ex_mem.aluResult = result;
            new_ex_mem.regData2 = id_ex.regData2;
            new_ex_mem.rd = id_ex.rd;
            new_ex_mem.regWrite = id_ex.regWrite;
            new_ex_mem.memRead = id_ex.memRead;
            new_ex_mem.memWrite = id_ex.memWrite;
            new_ex_mem.mem2reg = id_ex.mem2reg;
            new_ex_mem.jump = id_ex.jump;
            new_ex_mem.branchTaken = branchTaken;
            new_ex_mem.branchTarget = branchTarget;
            new_ex_mem.empty = false;
        }

        // -------------------- Hazard Detection --------------------
        bool stall = false;
        IF_ID new_if_id = if_id;
        ID_EX new_id_ex;
        new_id_ex.empty = true;

        // --- LOAD-USE Hazard Stall ---
        if (!id_ex.empty && id_ex.memRead && !if_id.empty) {
            string next = if_id.instr;
            int next_rs1 = binToUInt(extractBits(next, 19, 15));
            int next_rs2 = binToUInt(extractBits(next, 24, 20));
            if (id_ex.rd == next_rs1 || id_ex.rd == next_rs2) {
                stall = true;
                new_if_id = if_id;       // Hold IF/ID
                new_id_ex.empty = true;  // Bubble
            }
        }

        // -------------------- ID stage --------------------
        if (!stall && !if_id.empty) {
            string inst = if_id.instr;
            string opcode = extractBits(inst, 6, 0);
            uint8_t func3 = (uint8_t)binToUInt(extractBits(inst, 14, 12));
            uint8_t func7 = (uint8_t)binToUInt(extractBits(inst, 31, 25));
            int rd = (int)binToUInt(extractBits(inst, 11, 7));
            int rs1 = (int)binToUInt(extractBits(inst, 19, 15));
            int rs2 = (int)binToUInt(extractBits(inst, 24, 20));

            ControlSignals sig = getControlSignal(opcode, func3, func7);

            new_id_ex.PC = if_id.PC;
            new_id_ex.opcode = opcode;
            new_id_ex.func3 = func3;
            new_id_ex.func7 = func7;
            new_id_ex.rs1 = rs1;
            new_id_ex.rs2 = rs2;
            new_id_ex.rd = rd;
            new_id_ex.regData1 = regs[rs1];
            new_id_ex.regData2 = regs[rs2];
            new_id_ex.regWrite = sig.regWrite;
            new_id_ex.memRead = sig.memRead;
            new_id_ex.memWrite = sig.memWrite;
            new_id_ex.mem2reg = sig.mem2reg;
            new_id_ex.ALUSrc = sig.ALUSrc;
            new_id_ex.branch = sig.branch;
            new_id_ex.jump = sig.jump;
            new_id_ex.imm = imm_i(inst); // ensure imm_i covers I/B/J types correctly
            new_id_ex.empty = false;
        }

        // -------------------- IF stage --------------------
        if (!stall && PC / 4 < (int)binCode.size()) {
            new_if_id.instr = binCode[PC / 4];
            new_if_id.PC = PC;
            new_if_id.empty = false;
            PC += 4;
        }

        // -------------------- Debug (optional) --------------------
        cout << "PC = " << PC << ":\n";
        for (int i = 0; i < 32; i++) cout << "x" << i << " = " << regs[i] << endl;
        cout << "\n";

        // -------------------- Pipeline Register Update --------------------
        mem_wb = new_mem_wb;
        ex_mem = new_ex_mem;

        if (!stall) {
            id_ex = new_id_ex;
            if_id = new_if_id;
        } else {
            id_ex.empty = true; // insert bubble
        }

        // -------------------- Termination --------------------
        finished = if_id.empty && id_ex.empty && ex_mem.empty && mem_wb.empty && PC / 4 >= (int)binCode.size();
    }

    cout << "\nFinal Register Values:\n";
    for (int i = 0; i < 32; i++) cout << "x" << i << " = " << regs[i] << endl;
}
