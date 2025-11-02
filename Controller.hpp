#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include <bits/stdc++.h>
using namespace std;

struct ControlSignals{
    bool regRead = false;
    bool regWrite = false;
    bool ALUSrc = false;
    bool memRead = false;
    bool memWrite = false;
    bool mem2reg = false;
    bool branch = false;
    bool jump = false;
    uint8_t ALUOp = 0;
};

enum ALUOperation{
    ALUAdd = 0,
    ALUSub = 1,
    ALUAnd = 2,
    ALUOr = 3,
    ALUXor = 4,
    ALUSll = 5,
    ALUNop = 6
};

ControlSignals getControlSignal(const string& op, uint8_t func3, uint8_t func7);
uint8_t getALUControl(uint8_t ALUOp, uint8_t func3, uint8_t func7);

#endif