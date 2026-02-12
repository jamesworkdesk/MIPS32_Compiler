#include "lexer.h"
#include "error.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <map>
#include <cstdint>

static std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string toLower(const std::string &s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

static const std::map<std::string, std::string> REGISTER_ALIASES = {
    {"$zero", "$0"},  {"$at", "$1"},
    {"$v0", "$2"},    {"$v1", "$3"},
    {"$a0", "$4"},    {"$a1", "$5"},    {"$a2", "$6"},    {"$a3", "$7"},
    {"$t0", "$8"},    {"$t1", "$9"},    {"$t2", "$10"},   {"$t3", "$11"},
    {"$t4", "$12"},   {"$t5", "$13"},   {"$t6", "$14"},   {"$t7", "$15"},
    {"$s0", "$16"},   {"$s1", "$17"},   {"$s2", "$18"},   {"$s3", "$19"},
    {"$s4", "$20"},   {"$s5", "$21"},   {"$s6", "$22"},   {"$s7", "$23"},
    {"$t8", "$24"},   {"$t9", "$25"},
    {"$k0", "$26"},   {"$k1", "$27"},
    {"$gp", "$28"},   {"$sp", "$29"},   {"$fp", "$30"},   {"$ra", "$31"},
};

// Parse an immediate value (hex with 0x prefix, or decimal)
static int32_t parsePseudoImmediate(const std::string &s) {
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        return static_cast<int32_t>(std::stoul(s.substr(2), nullptr, 16));
    }
    return std::stoi(s);
}

std::vector<ParsedLine> tokenize(const std::string &filename) {
    std::vector<ParsedLine> lines;
    std::ifstream file(filename);

    if (!file.is_open()) {
        reportError(0, "cannot open file '" + filename + "'");
        return lines;
    }

    std::string rawLine;
    int lineNum = 0;

    while (std::getline(file, rawLine)) {
        lineNum++;

        // Strip comments at '#'
        std::string line = rawLine;
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        line = trim(line);
        if (line.empty()) continue;

        ParsedLine parsed;
        parsed.lineNumber = lineNum;
        parsed.rawText = trim(rawLine);

        // Check for label (colon)
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            parsed.label = trim(line.substr(0, colonPos));
            line = trim(line.substr(colonPos + 1));
            if (line.empty()) {
                // Label-only line
                lines.push_back(parsed);
                continue;
            }
        }

        // Parse mnemonic (first token)
        size_t spacePos = line.find_first_of(" \t");
        if (spacePos == std::string::npos) {
            // Mnemonic only, no operands (e.g. "nop")
            parsed.mnemonic = toLower(line);
        } else {
            parsed.mnemonic = toLower(line.substr(0, spacePos));
            std::string rest = trim(line.substr(spacePos));

            // Split operands by commas, respecting parentheses
            std::vector<std::string> rawOperands;
            std::string current;
            int parenDepth = 0;
            for (char c : rest) {
                if (c == '(') parenDepth++;
                if (c == ')') parenDepth--;
                if (c == ',' && parenDepth == 0) {
                    rawOperands.push_back(trim(current));
                    current.clear();
                } else {
                    current += c;
                }
            }
            if (!current.empty()) {
                rawOperands.push_back(trim(current));
            }

            // Process each operand: split offset($reg) into two operands
            for (const auto &op : rawOperands) {
                size_t parenOpen = op.find('(');
                size_t parenClose = op.find(')');
                if (parenOpen != std::string::npos &&
                    parenClose != std::string::npos &&
                    parenClose > parenOpen) {
                    std::string offset = trim(op.substr(0, parenOpen));
                    std::string reg = trim(op.substr(parenOpen + 1,
                                                      parenClose - parenOpen - 1));
                    parsed.operands.push_back(offset);
                    parsed.operands.push_back(reg);
                } else {
                    parsed.operands.push_back(op);
                }
            }
        }

        lines.push_back(parsed);
    }

    return lines;
}

void resolveAliases(std::vector<ParsedLine> &lines) {
    for (auto &line : lines) {
        for (auto &op : line.operands) {
            std::string lower = toLower(op);
            auto it = REGISTER_ALIASES.find(lower);
            if (it != REGISTER_ALIASES.end()) {
                op = it->second;
            }
        }
    }
}

void expandPseudos(std::vector<ParsedLine> &lines) {
    std::vector<ParsedLine> expanded;

    for (auto &line : lines) {
        if (line.mnemonic == "nop") {
            // nop -> sll $0, $0, 0
            line.mnemonic = "sll";
            line.operands = {"$0", "$0", "0"};
            expanded.push_back(line);
        } else if (line.mnemonic == "move") {
            // move $d, $s -> add $d, $s, $0
            if (line.operands.size() < 2) {
                reportError(line.lineNumber, "'move' requires 2 operands");
                expanded.push_back(line);
                continue;
            }
            line.mnemonic = "add";
            line.operands = {line.operands[0], line.operands[1], "$0"};
            expanded.push_back(line);
        } else if (line.mnemonic == "li") {
            // li $d, imm
            if (line.operands.size() < 2) {
                reportError(line.lineNumber, "'li' requires 2 operands");
                expanded.push_back(line);
                continue;
            }
            uint32_t val = static_cast<uint32_t>(parsePseudoImmediate(line.operands[1]));
            if (val <= 0xFFFF) {
                // Fits in 16 bits: ori $d, $0, imm
                line.mnemonic = "ori";
                line.operands = {line.operands[0], "$0", line.operands[1]};
                expanded.push_back(line);
            } else {
                // Need lui + ori
                uint16_t upper = static_cast<uint16_t>((val >> 16) & 0xFFFF);
                uint16_t lower = static_cast<uint16_t>(val & 0xFFFF);

                std::stringstream upperHex, lowerHex;
                upperHex << "0x" << std::hex << upper;
                lowerHex << "0x" << std::hex << lower;

                ParsedLine luiLine = line;
                luiLine.mnemonic = "lui";
                luiLine.operands = {line.operands[0], upperHex.str()};
                expanded.push_back(luiLine);

                ParsedLine oriLine = line;
                oriLine.mnemonic = "ori";
                oriLine.label.clear(); // label already on lui
                oriLine.operands = {line.operands[0], line.operands[0], lowerHex.str()};
                expanded.push_back(oriLine);
            }
        } else {
            expanded.push_back(line);
        }
    }

    lines = expanded;
}
