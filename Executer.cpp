#include "Assembler.hpp"
#include "DataControl.cpp"
#include "Pipeline.hpp"
#include "utils.hpp"

#include <iostream>
using namespace std;

vector<int32_t> regs(32, 0);
vector<int32_t> memory(1024, 0);

void execute() {
    IF_ID if_id;    if_id.empty = true;
    ID_EX id_ex;    id_ex.empty = true;
    EX_MEM ex_mem;  ex_mem.empty = true;
    MEM_WB mem_wb;  mem_wb.empty = true;

    int32_t PC = 0;
    bool finished = false;
    int cycle = 0;

    // branch control flags
    bool branchInPipeline = false;
    bool branchWait = false;

    while (!finished) {
        cycle++;
        cout << "\nCycle " << cycle << " ----------------\n";

        // if (cycle == 50) { cerr << "cycle cap reached\n"; break; }
        bool flush = false;
        //  WB stage 
        if (!mem_wb.empty && mem_wb.regWrite && mem_wb.rd != 0) {
            int32_t wbval = mem_wb.mem2reg ? mem_wb.memData : mem_wb.aluResult;
            cout << "[WB] cycle=" << cycle << " write x" << mem_wb.rd << " = " << wbval << "\n";
            regs[mem_wb.rd] = wbval;
        }

        //  MEM stage 
        MEM_WB new_mem_wb; new_mem_wb.empty = true;
        if (!ex_mem.empty) {
            new_mem_wb.rd = ex_mem.rd;
            new_mem_wb.regWrite = ex_mem.regWrite;
            new_mem_wb.mem2reg = ex_mem.mem2reg;

            if (ex_mem.memRead) {
                int idx = ex_mem.aluResult / 4;
                if (idx < 0 || idx >= (int)memory.size()) {
                    cerr << "Memory read out of bounds at cycle " << cycle << ": addr=" << ex_mem.aluResult << "\n";
                    exit(1);
                }
                new_mem_wb.memData = memory[idx];
                cout << "[MEM load] cycle=" << cycle
                     << " addr=" << ex_mem.aluResult
                     << " -> memData=" << new_mem_wb.memData
                     << " (index=" << idx << ")\n";
            } else {
                if (ex_mem.memWrite) {
                    int idx = ex_mem.aluResult / 4;
                    if (idx < 0 || idx >= (int)memory.size()) {
                        cerr << "Memory write out of bounds at cycle " << cycle << ": addr=" << ex_mem.aluResult << "\n";
                        exit(1);
                    }
                    memory[idx] = ex_mem.regData2;
                    cout << "[MEM store] cycle=" << cycle
                         << " addr=" << ex_mem.aluResult
                         << " value=" << ex_mem.regData2
                         << " (index=" << idx << ")\n";
                }
                new_mem_wb.memData = 0;
            }

            new_mem_wb.aluResult = ex_mem.aluResult;
            new_mem_wb.empty = false;
        }

        //  EX stage 
        EX_MEM new_ex_mem; new_ex_mem.empty = true;
        if (!id_ex.empty) {
            // Prepare operands
            int32_t op1 = id_ex.regData1;
            int32_t op2 = id_ex.ALUSrc ? id_ex.imm : id_ex.regData2;

            // Forwarding: (if not a load)
            if (!ex_mem.empty && ex_mem.regWrite && ex_mem.rd != 0 && ex_mem.rd == id_ex.rs1 && !ex_mem.memRead)
                op1 = ex_mem.aluResult;
            else if (!new_mem_wb.empty && new_mem_wb.regWrite && new_mem_wb.rd != 0 && new_mem_wb.rd == id_ex.rs1)
                op1 = new_mem_wb.mem2reg ? new_mem_wb.memData : new_mem_wb.aluResult;
            else if (!mem_wb.empty && mem_wb.regWrite && mem_wb.rd != 0 && mem_wb.rd == id_ex.rs1)
                op1 = mem_wb.mem2reg ? mem_wb.memData : mem_wb.aluResult;

            if (!id_ex.ALUSrc) {
                if (!ex_mem.empty && ex_mem.regWrite && ex_mem.rd != 0 && ex_mem.rd == id_ex.rs2 && !ex_mem.memRead)
                    op2 = ex_mem.aluResult;
                else if (!new_mem_wb.empty && new_mem_wb.regWrite && new_mem_wb.rd != 0 && new_mem_wb.rd == id_ex.rs2)
                    op2 = new_mem_wb.mem2reg ? new_mem_wb.memData : new_mem_wb.aluResult;
                else if (!mem_wb.empty && mem_wb.regWrite && mem_wb.rd != 0 && mem_wb.rd == id_ex.rs2)
                    op2 = mem_wb.mem2reg ? mem_wb.memData : mem_wb.aluResult;
            }

            int32_t result = alu(op1, op2, id_ex.opcode, id_ex.func7, id_ex.func3);

            // Branch / Jump
            bool branchTaken = false;
            int32_t branchTarget = id_ex.PC + id_ex.imm;

            if (id_ex.opcode == "1100011") {
                if (id_ex.func3 == 0b000 && op1 == op2) branchTaken = true;
                else if (id_ex.func3 == 0b101 && op1 >= op2) branchTaken = true;
                else if (id_ex.func3 == 0b001 && op1 != op2) branchTaken = true;
                else if (id_ex.func3 == 0b100 && op1 < op2) branchTaken = true;
            } else if (id_ex.opcode == "1101111") {
                branchTaken = true;
                result = id_ex.PC + 4;
            }

            cout << "[EX] cycle=" << cycle
                 << " PC=" << id_ex.PC
                 << " rs1_val=" << op1 << " rs2_val=" << op2
                 << " branchTaken=" << branchTaken
                 << " branchTarget=" << branchTarget
                 << " aluResult=" << result << "\n";

            new_ex_mem.aluResult = result;
            new_ex_mem.regData2 = id_ex.regData2;
            new_ex_mem.rd = id_ex.rd;
            new_ex_mem.regWrite = id_ex.regWrite;
            new_ex_mem.memRead = id_ex.memRead;
            new_ex_mem.memWrite = id_ex.memWrite;
            new_ex_mem.mem2reg = id_ex.mem2reg;
            new_ex_mem.branchTaken = branchTaken;
            new_ex_mem.branchTarget = branchTarget;
            new_ex_mem.empty = false;

            if (branchInPipeline) {
                branchInPipeline = false;
                branchWait = false;
                if (branchTaken) {
                    PC = branchTarget;
                    flush = true;
                    cout << "[EX] branch taken -> PC = " << PC << " (flush younger stages)\n";
                } else {
                    cout << "[EX] branch not taken -> resume fetch\n";
                }
            }
        }

        //  Load-use hazard detection 
        bool stall = false;
        IF_ID new_if_id; new_if_id.empty = true;
        ID_EX new_id_ex; new_id_ex.empty = true;

        if (!id_ex.empty && id_ex.memRead && !if_id.empty) {
            string next = if_id.instr;
            int next_rs1 = binToUInt(extractBits(next, 19, 15));
            int next_rs2 = binToUInt(extractBits(next, 24, 20));
            if (id_ex.rd != 0 && (id_ex.rd == next_rs1 || id_ex.rd == next_rs2)) {
                stall = true;
                new_if_id = if_id;
                new_id_ex.empty = true;
                cout << "[Hazard] cycle=" << cycle << " load-use detected, stalling 1 cycle\n";
            }
        }

        //  ID stage 
        if (!stall && !flush && !if_id.empty) {
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

            if (opcode == "1100011") new_id_ex.imm = imm_b(if_id.instr);
            else if (opcode == "1101111") new_id_ex.imm = imm_j(if_id.instr);
            else if (opcode == "0100011") new_id_ex.imm = imm_s(if_id.instr);
            else if (opcode == "0010111" || opcode == "0110111") new_id_ex.imm = imm_u(if_id.instr);
            else new_id_ex.imm = imm_i(if_id.instr);

            new_id_ex.empty = false;

            cout << "[ID decode] cycle=" << cycle << " PC=" << new_id_ex.PC
                 << " opcode=" << new_id_ex.opcode
                 << " rd=" << new_id_ex.rd << " rs1=" << new_id_ex.rs1 << " rs2=" << new_id_ex.rs2
                 << " imm=" << new_id_ex.imm << "\n";

            if (opcode == "1100011" || opcode == "1101111") {
                branchInPipeline = true;
                branchWait = true;
                cout << "[ID] branch/jump detected -> freeze IF until EX resolves\n";
            }
        }

        //  IF stage 
        IF_ID fetched_if; fetched_if.empty = true;
        if (!stall && !branchWait && PC / 4 < (int)binCode.size()) {
            fetched_if.instr = binCode[PC / 4];
            fetched_if.PC = PC;
            fetched_if.empty = false;
            cout << "[IF fetch] cycle=" << cycle << " PC=" << PC << "\n";
            PC += 4;
        } else if (branchWait) {
            cout << "[IF] waiting for branch resolution\n";
        } else if (stall) {
            cout << "[IF] stalled due to load-use hazard\n";
        }

        for (int i = 0; i < 32; i++){
            if(regs[i] != 0) cout << "x" << i << " = " << regs[i] << endl;
        }

        mem_wb = new_mem_wb;
        ex_mem = new_ex_mem;

        if (!stall) {
            id_ex = new_id_ex;
            if_id = fetched_if;
        } else {
            id_ex.empty = true;
        }

        if (flush) {
            if_id.empty = true;
            id_ex.empty = true;
            cout << "[FLUSH] cycle=" << cycle << " IF/ID and ID/EX cleared\n";
        }

        //  Termination 
        finished = if_id.empty && id_ex.empty && ex_mem.empty && mem_wb.empty && PC / 4 >= (int)binCode.size();
    }

    cout << "\nFinal Register Values:\n";
    for (int i = 0; i < 32; i++) cout << "x" << i << " = " << regs[i] << endl;
}