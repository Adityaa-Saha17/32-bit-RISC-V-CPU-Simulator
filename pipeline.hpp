#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include <bits/stdc++.h>
using namespace std;

struct IF_ID {
    string instr;
    int32_t PC;
    bool empty = true;
};

struct ID_EX {
    int32_t PC;
    int32_t regData1, regData2;
    int32_t imm;
    int rd, rs1, rs2;
    string opcode;
    uint8_t func3, func7;
    bool regWrite, memRead, memWrite, mem2reg, ALUSrc, branch, jump;
    bool empty = true;
};

struct EX_MEM {
    int32_t aluResult, regData2;
    int rd;
    bool regWrite, memRead, memWrite, mem2reg, jump, branchTaken = false;
    int32_t nextPC, branchTarget = 0;
    bool empty = true;
};

struct MEM_WB {
    int32_t memData, aluResult;
    int rd;
    bool regWrite, mem2reg;
    bool empty = true;
};

#endif