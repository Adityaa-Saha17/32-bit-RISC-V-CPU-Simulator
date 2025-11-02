#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <bits/stdc++.h>
using namespace std;

vector<string> binCode;

struct funcs {
    uint8_t func3;
    uint8_t func7;
};

unordered_map<string, funcs> funcMap = {
    {"add", {0b000, 0b0000000}},
    {"sub", {0b000, 0b0100000}},
    {"xor", {0b100, 0b0000000}},
    {"or",  {0b110, 0b0000000}},
    {"and", {0b111, 0b0000000}},
    {"sll", {0b001, 0b0000000}},
    {"srl", {0b101, 0b0000000}},
    {"sra", {0b101, 0b0100000}},
    {"slt", {0b010, 0b0000000}},
    {"sltu",{0b011, 0b0000000}},
    {"mul", {0b000, 0b0000001}},
    {"div", {0b100, 0b0000001}},

    {"addi", {0b000, 0b0000000}},
    {"xori", {0b100, 0b0000000}},
    {"ori",  {0b110, 0b0000000}},
    {"andi", {0b111, 0b0000000}},
    {"slli", {0b001, 0b0000000}},
    {"srli", {0b101, 0b0000000}},
    {"srai", {0b101, 0b0100000}},
    {"slti", {0b010, 0b0000000}},
    {"sltiu",{0b011, 0b0000000}},

    {"lb",  {0b000, 0b0000000}},
    {"lh",  {0b001, 0b0000000}},
    {"lw",  {0b010, 0b0000000}},
    {"lbu", {0b100, 0b0000000}},
    {"lhu", {0b101, 0b0000000}},

    {"sb",  {0b000, 0b0000000}},
    {"sh",  {0b001, 0b0000000}},
    {"sw",  {0b010, 0b0000000}},

    {"jal", {0b000, 0b0000000}},
    {"jalr",{0b000, 0b0000000}},

    {"beq", {0b000, 0b0000000}},
    {"bne", {0b001, 0b0000000}},
    {"blt", {0b100, 0b0000000}},
    {"bge", {0b101, 0b0000000}},
    {"bltu",{0b110, 0b0000000}},
    {"bgeu",{0b111, 0b0000000}}
};

unordered_map<string, uint8_t> reg = {
    {"x0", 0}, {"x1", 1}, {"x2", 2}, {"x3", 3},
    {"x4", 4}, {"x5", 5}, {"x6", 6}, {"x7", 7},
    {"x8", 8}, {"x9", 9}, {"x10",10}, {"x11",11},
    {"x12",12}, {"x13",13}, {"x14",14}, {"x15",15},
    {"x16",16}, {"x17",17}, {"x18",18}, {"x19",19},
    {"x20",20}, {"x21",21}, {"x22",22}, {"x23",23},
    {"x24",24}, {"x25",25}, {"x26",26}, {"x27",27},
    {"x28",28}, {"x29",29}, {"x30",30}, {"x31",31}
};

unordered_map<string, uint8_t> opCode = {
    {"R", 0b0110011},
    {"I", 0b0010011},
    {"L", 0b0000011},
    {"S", 0b0100011},
    {"B", 0b1100011},
    {"J", 0b1101111}
};

unordered_map<string, string> instMap = {
    {"add", "R"}, {"sub", "R"}, {"xor", "R"}, {"or", "R"}, {"and", "R"}, {"sll", "R"}, {"srl", "R"}, {"sra", "R"}, {"slt", "R"}, {"sltu", "R"}, {"mul", "R"}, {"div", "R"},
    {"addi", "I"}, {"xori", "I"}, {"ori", "I"}, {"andi", "I"}, {"slli", "I"}, {"srli", "I"}, {"srai", "I"}, {"slti", "I"}, {"sltiu", "I"},
    {"lb", "L"}, {"lh", "L"}, {"lw", "L"}, {"lbu", "L"}, {"lhu", "L"},
    {"jalr", "I"},
    {"beq", "B"}, {"bne", "B"}, {"blt", "B"}, {"bge", "B"}, {"bltu", "B"}, {"bgeu", "B"},
    {"sb", "S"}, {"sh", "S"}, {"sw", "S"},
    {"jal", "J"}
};

vector<string> parsing(string unparsed){
    vector<string> parsed;
    string curr;

    for (char c : unparsed){
        if (c == '#') break;
        if (c == ' ' || c == ','){
            if (!curr.empty()) parsed.push_back(curr);
            curr = "";
        }
        else curr.push_back(c);
    }
    if (!curr.empty()) parsed.push_back(curr);
    return parsed;
}

string toBin(uint32_t num, int bits){
    string bin = "";
    for (int i = bits - 1; i >= 0; i--)
        bin += ((num >> i) & 1) ? "1" : "0";
    return bin;
}

int32_t signExtend(int32_t imm, int bits){
    int32_t mask = 1 << (bits - 1);
    return (imm ^ mask) - mask;
}

string encodeR(const vector<string>& inst){
    string f = inst[0], rd = inst[1], rs1 = inst[2], rs2 = inst[3];
    auto spec = funcMap[f];
    uint32_t ins = (spec.func7 << 25) | ((reg[rs2] & 31) << 20) | ((reg[rs1] & 31) << 15) | ((spec.func3 & 7) << 12) | ((reg[rd] & 31) << 7) | (opCode["R"]);
    return toBin(ins, 32);
}

string encodeI(const vector<string>& inst){
    string f = inst[0], rd = inst[1], rs1 = inst[2];
    int im = stoi(inst[3]);
    auto spec = funcMap[f];
    uint32_t imm = im & 0xfff;
    string op;

    if (f == "lw" || f == "lh" || f == "lb" || f == "lhu" || f == "lbu") op = "L";
    else if (f == "jalr") op = "JALR";
    else op = "I";

    uint32_t ins = (imm << 20) | ((reg[rs1] & 31) << 15) | ((spec.func3 & 7) << 12) | ((reg[rd] & 31) << 7) | (opCode[op] & 127);
    return toBin(ins, 32);
}

string encodeB(const vector<string>& inst){
    string f = inst[0], rs1 = inst[1], rs2 = inst[2];
    int imm = stoi(inst[3]);
    imm = signExtend(imm, 13);

    auto spec = funcMap[f];
    uint32_t bin12 = (imm >> 12) & 1, bin11 = (imm >> 11) & 1;
    uint32_t bits10_5 = (imm >> 5) & 0b111111, bits4_1 = (imm >> 1) & 0b1111;
    uint32_t ins = (bin12 << 31) | (bin11 << 7) | (bits10_5 << 25) | (bits4_1 << 8) | ((reg[rs2] & 31) << 20) | ((reg[rs1] & 31) << 15) | ((spec.func3 & 7) << 12) | (opCode["B"]);
    return toBin(ins, 32);
}

string encodeS(const vector<string>& inst){
    string f = inst[0], rs2 = inst[1];
    string imm_rs1 = inst[2];
    int pos1 = imm_rs1.find('(');
    int pos2 = imm_rs1.find(')');
    int imm = stoi(imm_rs1.substr(0, pos1));
    string rs1 = imm_rs1.substr(pos1 + 1, pos2 - pos1 - 1);
    imm = signExtend(imm, 12);

    auto spec = funcMap[f];
    uint32_t bin12 = (imm >> 5) & 127;
    uint32_t ins = (bin12 << 25) | ((reg[rs2] & 31) << 20) | ((reg[rs1] & 31) << 15) | ((spec.func3 & 7) << 12) | ((imm & 31) << 7) | (opCode["S"]);
    return toBin(ins, 32);
}

string encodeJ(const vector<string>& inst){
    string f = inst[0], rd = inst[1];
    int imm = stoi(inst[2]);
    imm = signExtend(imm, 21);

    uint32_t bin20 = (imm >> 20) & 0b1, bin10_1 = (imm >> 1) & 1023;
    uint32_t bin11 = (imm >> 11) & 0b1, bin19_12 = (imm >> 12) & 255;
    uint32_t ins = (bin20 << 31) | (bin19_12 << 12) | (bin11 << 20) | (bin10_1 << 21) | ((reg[rd] & 31) << 7) | (opCode["J"]);
    return toBin(ins, 32);
}

void assembler(string& line){
    vector<string> inst = parsing(line);
    if (inst.empty()) return;

    string type = instMap[inst[0]], machineCode;

    if (type == "R") machineCode = encodeR(inst);
    else if (type == "I") machineCode = encodeI(inst);
    else if (type == "S") machineCode = encodeS(inst);
    else if (type == "B") machineCode = encodeB(inst);
    else if (type == "J") machineCode = encodeJ(inst);
    else if (type == "L") machineCode = encodeI(inst);
    
    binCode.push_back(machineCode);
}

#endif