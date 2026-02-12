#ifndef ENCODER_H
#define ENCODER_H

#include "lexer.h"
#include <string>
#include <vector>
#include <map>

enum class OperandPattern {
    R_DST_SRC_TMP,    // add $d, $s, $t
    R_DST_TMP_SHAMT,  // sll $d, $t, shamt
    R_SRC_ONLY,        // jr $s
    I_TMP_SRC_IMM,     // addi $t, $s, imm
    I_TMP_IMM,         // lui $t, imm
    I_SRC_TMP_LABEL,   // beq $s, $t, label
    I_TMP_OFF_SRC,     // lw $t, offset($s)
    J_LABEL,           // j label / jal label
};

struct InstructionDef {
    std::string opcode;    // 6-bit binary string
    std::string funct;     // 6-bit for R-type, empty otherwise
    OperandPattern pattern;
};

struct EncodedInst {
    std::string hex;      // 8-char uppercase hex
    std::string rawText;  // original source line for MIF comment
};

std::map<std::string, int> buildLabelTable(const std::vector<ParsedLine> &lines);
std::vector<EncodedInst> encode(const std::vector<ParsedLine> &lines,
                                 const std::map<std::string, int> &labels);

#endif
