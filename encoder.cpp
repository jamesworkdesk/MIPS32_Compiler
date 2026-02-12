#include "encoder.h"
#include "error.h"
#include <bitset>
#include <sstream>
#include <iomanip>
#include <algorithm>

// Instruction table: mnemonic -> definition
static const std::map<std::string, InstructionDef> INSTRUCTIONS = {
    {"add",    {"000000", "100000", OperandPattern::R_DST_SRC_TMP}},
    {"addu",   {"000000", "100001", OperandPattern::R_DST_SRC_TMP}},
    {"addi",   {"001000", "",       OperandPattern::I_TMP_SRC_IMM}},
    {"addiu",  {"001001", "",       OperandPattern::I_TMP_SRC_IMM}},
    {"and",    {"000000", "100100", OperandPattern::R_DST_SRC_TMP}},
    {"andi",   {"001100", "",       OperandPattern::I_TMP_SRC_IMM}},
    {"beq",    {"000100", "",       OperandPattern::I_SRC_TMP_LABEL}},
    {"bne",    {"000101", "",       OperandPattern::I_SRC_TMP_LABEL}},
    {"j",      {"000010", "",       OperandPattern::J_LABEL}},
    {"jal",    {"000011", "",       OperandPattern::J_LABEL}},
    {"jr",     {"000000", "001000", OperandPattern::R_SRC_ONLY}},
    {"lbu",    {"100100", "",       OperandPattern::I_TMP_OFF_SRC}},
    {"lhu",    {"100101", "",       OperandPattern::I_TMP_OFF_SRC}},
    {"lui",    {"001111", "",       OperandPattern::I_TMP_IMM}},
    {"lw",     {"100011", "",       OperandPattern::I_TMP_OFF_SRC}},
    {"nor",    {"000000", "100111", OperandPattern::R_DST_SRC_TMP}},
    {"or",     {"000000", "100101", OperandPattern::R_DST_SRC_TMP}},
    {"ori",    {"001101", "",       OperandPattern::I_TMP_SRC_IMM}},
    {"sb",     {"101000", "",       OperandPattern::I_TMP_OFF_SRC}},
    {"sh",     {"101001", "",       OperandPattern::I_TMP_OFF_SRC}},
    {"sll",    {"000000", "000000", OperandPattern::R_DST_TMP_SHAMT}},
    {"slt",    {"000000", "101010", OperandPattern::R_DST_SRC_TMP}},
    {"slti",   {"001010", "",       OperandPattern::I_TMP_SRC_IMM}},
    {"sltiu",  {"001011", "",       OperandPattern::I_TMP_SRC_IMM}},
    {"sltu",   {"000000", "101011", OperandPattern::R_DST_SRC_TMP}},
    {"srl",    {"000000", "000010", OperandPattern::R_DST_TMP_SHAMT}},
    {"sub",    {"000000", "100010", OperandPattern::R_DST_SRC_TMP}},
    {"subu",   {"000000", "100011", OperandPattern::R_DST_SRC_TMP}},
    {"sw",     {"101011", "",       OperandPattern::I_TMP_OFF_SRC}},
};

// Register number -> 5-bit binary string
static const std::string REG_BINARY[32] = {
    "00000","00001","00010","00011","00100","00101","00110","00111",
    "01000","01001","01010","01011","01100","01101","01110","01111",
    "10000","10001","10010","10011","10100","10101","10110","10111",
    "11000","11001","11010","11011","11100","11101","11110","11111"
};

static std::string encodeRegister(const std::string &operand, int lineNumber) {
    if (operand.size() < 2 || operand[0] != '$') {
        reportError(lineNumber, "invalid register '" + operand + "'");
        return "00000";
    }
    int n = 0;
    try {
        n = std::stoi(operand.substr(1));
    } catch (...) {
        reportError(lineNumber, "invalid register '" + operand + "'");
        return "00000";
    }
    if (n < 0 || n > 31) {
        reportError(lineNumber, "register number out of range: " + operand);
        return "00000";
    }
    return REG_BINARY[n];
}

static int parseImmediate(const std::string &operand, int lineNumber) {
    try {
        if (operand.size() > 2 && operand[0] == '0' &&
            (operand[1] == 'x' || operand[1] == 'X')) {
            return static_cast<int>(std::stoul(operand.substr(2), nullptr, 16));
        }
        return std::stoi(operand);
    } catch (...) {
        reportError(lineNumber, "invalid immediate value '" + operand + "'");
        return 0;
    }
}

static std::string encodeImmediate(int value, int bits) {
    if (bits == 5)  return std::bitset<5>(value).to_string();
    if (bits == 16) return std::bitset<16>(value).to_string();
    if (bits == 26) return std::bitset<26>(value).to_string();
    return std::string(bits, '0');
}

static std::string bin2hex(const std::string &binary) {
    std::bitset<32> b(binary);
    unsigned long n = b.to_ulong();
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(8) << n;
    std::string result = ss.str();
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

static int expectedOperandCount(OperandPattern pattern) {
    switch (pattern) {
        case OperandPattern::R_DST_SRC_TMP:   return 3;
        case OperandPattern::R_DST_TMP_SHAMT:  return 3;
        case OperandPattern::R_SRC_ONLY:        return 1;
        case OperandPattern::I_TMP_SRC_IMM:     return 3;
        case OperandPattern::I_TMP_IMM:         return 2;
        case OperandPattern::I_SRC_TMP_LABEL:   return 3;
        case OperandPattern::I_TMP_OFF_SRC:     return 3; // $t, offset, $s (lexer splits offset($s))
        case OperandPattern::J_LABEL:           return 1;
    }
    return 0;
}

static std::string encodeInstruction(const InstructionDef &def,
                                      const ParsedLine &line,
                                      const std::map<std::string, int> &labels,
                                      int address) {
    int ln = line.lineNumber;

    switch (def.pattern) {
        case OperandPattern::R_DST_SRC_TMP: {
            // add $d, $s, $t -> opcode | rs | rt | rd | 00000 | funct
            std::string rd = encodeRegister(line.operands[0], ln);
            std::string rs = encodeRegister(line.operands[1], ln);
            std::string rt = encodeRegister(line.operands[2], ln);
            return def.opcode + rs + rt + rd + "00000" + def.funct;
        }
        case OperandPattern::R_DST_TMP_SHAMT: {
            // sll $d, $t, shamt -> opcode | 00000 | rt | rd | shamt | funct
            std::string rd = encodeRegister(line.operands[0], ln);
            std::string rt = encodeRegister(line.operands[1], ln);
            std::string shamt = encodeImmediate(
                parseImmediate(line.operands[2], ln), 5);
            return def.opcode + "00000" + rt + rd + shamt + def.funct;
        }
        case OperandPattern::R_SRC_ONLY: {
            // jr $s -> opcode | rs | 000000000000000 | funct
            std::string rs = encodeRegister(line.operands[0], ln);
            return def.opcode + rs + "000000000000000" + def.funct;
        }
        case OperandPattern::I_TMP_SRC_IMM: {
            // addi $t, $s, imm -> opcode | rs | rt | imm
            std::string rt = encodeRegister(line.operands[0], ln);
            std::string rs = encodeRegister(line.operands[1], ln);
            std::string imm = encodeImmediate(
                parseImmediate(line.operands[2], ln), 16);
            return def.opcode + rs + rt + imm;
        }
        case OperandPattern::I_TMP_IMM: {
            // lui $t, imm -> opcode | 00000 | rt | imm
            std::string rt = encodeRegister(line.operands[0], ln);
            std::string imm = encodeImmediate(
                parseImmediate(line.operands[1], ln), 16);
            return def.opcode + "00000" + rt + imm;
        }
        case OperandPattern::I_SRC_TMP_LABEL: {
            // beq $s, $t, label -> opcode | rs | rt | offset
            std::string rs = encodeRegister(line.operands[0], ln);
            std::string rt = encodeRegister(line.operands[1], ln);
            const std::string &labelName = line.operands[2];
            auto it = labels.find(labelName);
            if (it == labels.end()) {
                reportError(ln, "undefined label '" + labelName + "'");
                return std::string(32, '0');
            }
            int target = it->second;
            int offset;
            // Replicate original asymmetric formula:
            // Forward:  offset = (target - current) - 1
            // Backward: offset = -(current - target)
            if (target > address) {
                offset = (target - address) - 1;
            } else {
                offset = -(address - target);
            }
            std::string imm = encodeImmediate(offset, 16);
            return def.opcode + rs + rt + imm;
        }
        case OperandPattern::I_TMP_OFF_SRC: {
            // lw $t, offset($s) -> opcode | rs | rt | offset
            // Lexer splits "offset($s)" into two operands: [offset, $s]
            std::string rt = encodeRegister(line.operands[0], ln);
            std::string imm = encodeImmediate(
                parseImmediate(line.operands[1], ln), 16);
            std::string rs = encodeRegister(line.operands[2], ln);
            return def.opcode + rs + rt + imm;
        }
        case OperandPattern::J_LABEL: {
            // j label -> opcode | address
            const std::string &labelName = line.operands[0];
            auto it = labels.find(labelName);
            if (it == labels.end()) {
                reportError(ln, "undefined label '" + labelName + "'");
                return std::string(32, '0');
            }
            std::string addr = encodeImmediate(it->second, 26);
            return def.opcode + addr;
        }
    }
    return std::string(32, '0');
}

std::map<std::string, int> buildLabelTable(const std::vector<ParsedLine> &lines) {
    std::map<std::string, int> labels;
    int address = 0;

    for (const auto &line : lines) {
        if (!line.label.empty()) {
            if (labels.count(line.label)) {
                reportError(line.lineNumber,
                            "duplicate label '" + line.label + "'");
            }
            labels[line.label] = address;
        }
        if (!line.mnemonic.empty()) {
            address++;
        }
    }

    return labels;
}

std::vector<EncodedInst> encode(const std::vector<ParsedLine> &lines,
                                 const std::map<std::string, int> &labels) {
    std::vector<EncodedInst> encoded;
    int address = 0;

    for (const auto &line : lines) {
        if (line.mnemonic.empty()) continue; // skip label-only lines

        auto it = INSTRUCTIONS.find(line.mnemonic);
        if (it == INSTRUCTIONS.end()) {
            reportError(line.lineNumber,
                        "unknown instruction '" + line.mnemonic + "'");
            address++;
            continue;
        }

        const auto &def = it->second;

        // Check operand count
        int expected = expectedOperandCount(def.pattern);
        if (static_cast<int>(line.operands.size()) < expected) {
            reportError(line.lineNumber,
                        "'" + line.mnemonic + "' requires " +
                        std::to_string(expected) + " operands, got " +
                        std::to_string(line.operands.size()));
            address++;
            continue;
        }

        std::string binary = encodeInstruction(def, line, labels, address);

        EncodedInst inst;
        inst.hex = bin2hex(binary);
        inst.rawText = line.rawText;
        encoded.push_back(inst);

        address++;
    }

    return encoded;
}
