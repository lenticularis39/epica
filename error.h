#ifndef EPICA_ERROR_H
#define EPICA_ERROR_H

#include "location.hh"

void ast_error(std::string message, yy::location loc);

#endif //EPICA_ERROR_H
