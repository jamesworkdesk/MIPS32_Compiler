#ifndef ERROR_H
#define ERROR_H

#include <string>

void reportError(int line, const std::string &msg);
void reportWarning(int line, const std::string &msg);
bool hasErrors();
int errorCount();
void resetErrors();

#endif
