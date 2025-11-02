#include "Assembler.hpp"
#include "DataControl.cpp"
#include "Pipeline.hpp"
#include "utils.hpp"

vector<int32_t> regs(32, 0);
vector<int32_t> memory(1024, 0);

void execute() {
    IF_ID if_id;     // old IF/ID
    ID_EX id_ex;     // old ID/EX
    EX_MEM ex_mem;   // old EX/MEM
    MEM_WB mem_wb;   // old MEM/WB

    int32_t PC = 0;
    bool finished = false;
    int cycle = 0;

    while (!finished) {
        cycle++;
        cout << "\nCycle " << cycle << " ----------------\n";

        // -------------------- WB stage (use old mem_wb) --------------------
        if (!mem_wb.empty && mem_wb.regWrite && mem_wb.rd != 0) {
            regs[mem_wb.rd] = mem_wb.mem2reg ? mem_wb.memData : mem_wb.aluResult;
        }

        // -------------------- MEM stage (produce new MEM/WB) ----------------
        MEM_WB new_mem_wb;
        new_mem_wb.empty = true;
        if (!ex_mem.empty) {
            new_mem_wb.rd = ex_mem.rd;
            new_mem_wb.regWrite = ex_mem.regWrite;
            new_mem_wb.mem2reg = ex_mem.mem2reg;

            if (ex_mem.memRead) {
                // load: read from data memory
                new_mem_wb.memData = memory[ex_mem.aluResult / 4];
            } else {
                new_mem_wb.memData = 0;
            }

            // pass ALU result along
            new_mem_wb.aluResult = ex_mem.aluResult;
            new_mem_wb.empty = false;
        }

        // -------------------- EX stage (use id_ex, ex_mem, mem_wb old) ----------------
        EX_MEM new_ex_mem;
        new_ex_mem.empty = true;
        if (!id_ex.empty) {
            // Determine ALU operands with forwarding
            int32_t op1 = id_ex.regData1;
            int32_t op2 = id_ex.ALUSrc ? id_ex.imm : id_ex.regData2;

            // Forwarding from EX/MEM (priority) if EX/MEM writes and is not a load
            if (!ex_mem.empty && ex_mem.regWrite && ex_mem.rd != 0 && ex_mem.rd == id_ex.rs1 && !ex_mem.memRead) {
                op1 = ex_mem.aluResult;
            } else if (!mem_wb.empty && mem_wb.regWrite && mem_wb.rd != 0 && mem_wb.rd == id_ex.rs1) {
                op1 = mem_wb.mem2reg ? mem_wb.memData : mem_wb.aluResult;
            }

            if (!id_ex.ALUSrc) { // only consider rs2 forwarding when second operand is register
                if (!ex_mem.empty && ex_mem.regWrite && ex_mem.rd != 0 && ex_mem.rd == id_ex.rs2 && !ex_mem.memRead) {
                    op2 = ex_mem.aluResult;
                } else if (!mem_wb.empty && mem_wb.regWrite && mem_wb.rd != 0 && mem_wb.rd == id_ex.rs2) {
                    op2 = mem_wb.mem2reg ? mem_wb.memData : mem_wb.aluResult;
                }
            }

            // Now call ALU with forwarded operands
            int32_t result = alu(op1, op2, id_ex.opcode, id_ex.func7, id_ex.func3);

            new_ex_mem.aluResult = result;
            new_ex_mem.regData2 = id_ex.regData2;
            new_ex_mem.rd = id_ex.rd;
            new_ex_mem.regWrite = id_ex.regWrite;
            new_ex_mem.memRead = id_ex.memRead;
            new_ex_mem.memWrite = id_ex.memWrite;
            new_ex_mem.mem2reg = id_ex.mem2reg;
            new_ex_mem.jump = id_ex.jump;
            new_ex_mem.empty = false;
        }

        // -------------------- Hazard detection (LOAD-USE) --------------------
        bool stall = false;
        // If there is a load in ID/EX (i.e., producing data in MEM) and IF/ID instruction reads its rd => stall
        if (!id_ex.empty && id_ex.memRead && !if_id.empty) {
            int next_rs1 = binToUInt(extractBits(if_id.instr, 19, 15));
            int next_rs2 = binToUInt(extractBits(if_id.instr, 24, 20));
            if (id_ex.rd != 0 && (id_ex.rd == next_rs1 || id_ex.rd == next_rs2)) {
                stall = true;
            }
        }

        // -------------------- ID stage (if not stalling) --------------------
        ID_EX new_id_ex;
        new_id_ex.empty = true;
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
            new_id_ex.imm = imm_i(inst);
            new_id_ex.empty = false;
        } else if (stall) {
            // Insert bubble in ID/EX (a NOP) by leaving new_id_ex.empty == true
        }

        // -------------------- IF stage (if not stalling) --------------------
        IF_ID new_if_id;
        new_if_id.empty = true;
        if (!stall && PC / 4 < (int)binCode.size()) {
            new_if_id.instr = binCode[PC / 4];
            new_if_id.PC = PC;
            new_if_id.empty = false;
            PC += 4;
        } else if (stall) {
            // hold IF/ID (we will assign if_id back later)
        }

        // -------------------- Debug (optional) --------------------
        cout << "PC = " << PC << ":\n";
        for (int i = 0; i < 32; i++) cout << "x" << i << " = " << regs[i] << endl;
        cout << "\n";

        // -------------------- Commit pipeline registers (one clock tick) --------------------
        mem_wb = new_mem_wb;
        ex_mem = new_ex_mem;
        // if stall: hold IF/ID (i.e., don't move it forward), and set id_ex to bubble
        if (!stall) {
            id_ex = new_id_ex;
            if_id = new_if_id;
        } else {
            // freeze IF/ID (keep old if_id) and insert bubble at ID/EX
            id_ex.empty = true;
            // if_id remains unchanged
        }

        // -------------------- Termination condition --------------------
        finished = if_id.empty && id_ex.empty && ex_mem.empty && mem_wb.empty && PC / 4 >= (int)binCode.size();
    }

    cout << "\nFinal Register Values:\n";
    for (int i = 0; i < 32; i++) cout << "x" << i << " = " << regs[i] << endl;
}