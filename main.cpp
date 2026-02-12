#include "assembler.h"
#include <iostream>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input.txt>" << std::endl;
        return 1;
    }

    if (!assemble(argv[1])) {
        std::cerr << "Assembly failed." << std::endl;
        return 1;
    }

    return 0;
}
