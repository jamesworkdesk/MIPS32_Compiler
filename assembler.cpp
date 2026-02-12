#include "assembler.h"
#include "lexer.h"
#include "encoder.h"
#include "error.h"
#include <fstream>
#include <iomanip>
#include <iostream>

static std::string deriveOutputFilename(const std::string &input) {
    size_t dot = input.rfind('.');
    if (dot != std::string::npos) {
        return input.substr(0, dot) + ".mif";
    }
    return input + ".mif";
}

static void writeMIF(const std::vector<EncodedInst> &encoded,
                      const std::string &outFile) {
    std::ofstream out(outFile);
    if (!out.is_open()) {
        std::cerr << "Error: cannot open output file '" << outFile << "'"
                  << std::endl;
        return;
    }

    out << "WIDTH=32;" << std::endl;
    out << "DEPTH=256;" << std::endl;
    out << std::endl;
    out << "ADDRESS_RADIX=HEX;" << std::endl;
    out << "DATA_RADIX=HEX;" << std::endl;
    out << std::endl;
    out << "CONTENT BEGIN" << std::endl;

    int numInstructions = static_cast<int>(encoded.size());

    for (int i = 0; i < numInstructions && i < 256; i++) {
        out << "   ";
        out << std::setw(3) << std::setfill('0') << std::hex << i;
        out << "  :   ";
        out << encoded[i].hex << ";";
        if (!encoded[i].rawText.empty()) {
            out << "  -- " << encoded[i].rawText;
        }
        out << std::endl;
    }

    if (numInstructions < 256) {
        out << "   [";
        out << std::setw(3) << std::setfill('0') << std::hex << numInstructions;
        out << "..";
        out << std::setw(3) << std::setfill('0') << std::hex << 255;
        out << "]  :   00000000;" << std::endl;
    }

    out << std::endl;
    out << "END;";
    out.close();
}

bool assemble(const std::string &inputFile) {
    resetErrors();

    // Step 1: Tokenize
    auto lines = tokenize(inputFile);
    if (hasErrors()) return false;

    // Step 2: Resolve register aliases ($zero -> $0, etc.)
    resolveAliases(lines);

    // Step 3: Expand pseudo-instructions (nop, move, li)
    expandPseudos(lines);

    // Step 4: Build label table
    auto labels = buildLabelTable(lines);
    if (hasErrors()) return false;

    // Step 5: Encode instructions
    auto encoded = encode(lines, labels);
    if (hasErrors()) return false;

    // Step 6: Write MIF output
    std::string outFile = deriveOutputFilename(inputFile);
    writeMIF(encoded, outFile);

    std::cout << "Assembly complete: " << encoded.size()
              << " instructions written to " << outFile << std::endl;
    return true;
}
