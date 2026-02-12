#include "error.h"
#include <iostream>

static int g_errorCount = 0;

void reportError(int line, const std::string &msg) {
    if (line > 0)
        std::cerr << "Error on line " << line << ": " << msg << std::endl;
    else
        std::cerr << "Error: " << msg << std::endl;
    g_errorCount++;
}

void reportWarning(int line, const std::string &msg) {
    if (line > 0)
        std::cerr << "Warning on line " << line << ": " << msg << std::endl;
    else
        std::cerr << "Warning: " << msg << std::endl;
}

bool hasErrors() {
    return g_errorCount > 0;
}

int errorCount() {
    return g_errorCount;
}

void resetErrors() {
    g_errorCount = 0;
}
