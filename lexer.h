#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

struct ParsedLine {
    int lineNumber;                    // original source line (for errors)
    std::string rawText;               // original line text (for MIF comments)
    std::string label;                 // label defined on this line (empty if none)
    std::string mnemonic;              // instruction mnemonic (lowercase)
    std::vector<std::string> operands; // registers, immediates, labels
};

std::vector<ParsedLine> tokenize(const std::string &filename);
void resolveAliases(std::vector<ParsedLine> &lines);
void expandPseudos(std::vector<ParsedLine> &lines);

#endif
