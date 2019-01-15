#include <string.h>
#include "function.hpp"

Function::Function(char *n, int *args) {
    name = n;
    argTypes = args;
}

char* Function::getName() {
    return name;
}

int* Function::getArgTypes() {
    return argTypes;
}

bool Function::operator==(const Function &rhs) {
    return (strcmp(this->name, rhs.name) == 0) && (*(this->argTypes) == *(rhs.argTypes));
}