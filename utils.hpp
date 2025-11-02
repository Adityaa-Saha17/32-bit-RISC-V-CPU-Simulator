#ifndef UTILS_HPP
#define UTILS_HPP

#include<bits/stdc++.h>
using namespace std;

string extractBits(const string& bin, int hi, int lo){
    if (bin.size() < 32){
        cerr << "\nInvalid len: " << bin.size() << "!!\nAborting..." << endl;
        exit(1);
    }
    return bin.substr(31 - hi, hi - lo + 1);
}

int32_t binToInt(const string& bin){
    if (bin.empty()){
        cerr << "\nInvalid binCode!!\nAborting..." << endl;
        exit(1);
    }

    int32_t val = 0;
    for (char c : bin)
        val = (val << 1) | (c - '0');

    if (bin[0] == '1' && bin.size() < 32)
        val -= (1 << bin.size());
    return val;
}

uint32_t binToUInt(const string& bin){
    uint32_t val = 0;
    for (char c : bin)
        val = (val << 1) | (c - '0');
    return val;
}

int32_t imm_i(const string& inst){
    return binToInt(inst.substr(0, 12));
}

int32_t imm_s(const string& inst){
    return binToInt(inst.substr(0, 7) + inst.substr(20, 5));
}

int32_t imm_u(const string& inst){
    return binToInt(inst.substr(0, 20)) << 12;
}

int32_t imm_b(const string& inst) {
    string imm = "";
    imm += inst[0];
    imm += inst[24];
    imm += inst.substr(1, 6);
    imm += inst.substr(20, 4);
    imm += '0';
    return binToInt(imm);
}

int32_t imm_j(const string& inst){
    string imm = "";
    imm += inst[0];
    imm += inst.substr(12, 8);
    imm += inst[11];
    imm += inst.substr(1, 10);
    imm += '0';
    return binToInt(imm);
}

int32_t alu(int32_t in1, int32_t in2, const string& op, uint8_t func7, uint8_t func3){
    if (op == "0110011"){
        if (func3 == 0b000){
            if (func7 == 0b0000000) return in1 + in2;
            else if (func7 == 0b0100000) return in1 - in2;
            else if (func7 == 0b0000001) return in1 * in2;
        }
        else if (func3 == 0b111 && func7 == 0b0000000) return in1 & in2;
        else if (func3 == 0b110 && func7 == 0b0000000) return in1 | in2;
        else if (func3 == 0b100 && func7 == 0b0000000) return in1 ^ in2;
        else if (func3 == 0b100 && func7 == 0b0000001){
            if (in2 == 0){
                cerr << "\nDivison by Zero!!\nAborting..." << endl;
                exit(1);
            }
            return in1 / in2;
        }
        else if (func3 == 0b001 && func7 == 0b0000000) return in1 << (in2 & 31);
    }
    else if (op == "0010011"){
        if (func3 == 0b000) return in1 + in2;
        else if (func3 == 0b111) return in1 & in2;
        else if (func3 == 0b110) return in1 | in2;
        else if (func3 == 0b100) return in1 ^ in2;
        else if (func3 == 0b001 && func7 == 0b0000000) return in1 << (in2 & 31);
    }
    return 0;
}

#endif