#include "Controller.hpp"

ControlSignals getControlSignal(const string& op, uint8_t func3, uint8_t func7){
    ControlSignals signals = {};

    if (op == "0110011"){ // R
        signals.regRead = true;
        signals.regWrite = true;
        signals.ALUSrc = false;
        signals.memRead = false;
        signals.memWrite = false;
        signals.mem2reg = false;
        signals.branch = false;
        signals.jump = false;
        signals.ALUOp = 2;
    }
    else if (op == "0010011"){ // I
        signals.regRead = true;
        signals.regWrite = true;
        signals.ALUSrc = true;
        signals.memRead = false;
        signals.memWrite = false;
        signals.mem2reg = false;
        signals.branch = false;
        signals.jump = false;
        signals.ALUOp = 2;
    }
    else if (op == "1101111"){ // J
        signals.regRead = false;
        signals.regWrite = true;
        signals.ALUSrc = false;
        signals.memRead = false;
        signals.memWrite = false;
        signals.mem2reg = false;
        signals.branch = false;
        signals.jump = true;
        signals.ALUOp = 3;
    }
    else if (op == "0000011"){ // L
        signals.regRead = true;
        signals.regWrite = true;
        signals.ALUSrc = true;
        signals.memRead = true;
        signals.memWrite = false;
        signals.mem2reg = true;
        signals.branch = false;
        signals.jump = false;
        signals.ALUOp = 0;
    }
    else if (op == "1100011"){ // B
        signals.regRead = true;
        signals.regWrite = false;
        signals.ALUSrc = false;
        signals.memRead = false;
        signals.memWrite = false;
        signals.mem2reg = false;
        signals.branch = true;
        signals.jump = false;
        signals.ALUOp = 1;
    }
    else if (op == "0100011"){ // S
        signals.regRead = true;
        signals.regWrite = false;
        signals.ALUSrc = true;
        signals.memRead = false;
        signals.memWrite = true;
        signals.mem2reg = false;
        signals.branch = false;
        signals.jump = false;
        signals.ALUOp = 0;
    }

    return signals;
}

uint8_t getALUControl(uint8_t ALUOp, uint8_t func3, uint8_t func7){
    if (ALUOp == 0) return ALUAdd;
    if (ALUOp == 1) return ALUSub;
    if (ALUOp == 3) return ALUAdd;

    switch (func3){
        case 0:
            if (func7 == 0b0100000) return ALUSub;
            return ALUAdd;
        case 1:
            return ALUSll;
        case 4:
            return ALUXor;
        case 6:
            return ALUOr;
        case 7:
            return ALUAnd;
        default:
            return ALUNop;
    }
    return ALUNop;
}
