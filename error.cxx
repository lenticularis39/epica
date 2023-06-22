#include "error.h"

void ast_error(std::string message, yy::location loc) {
    std::cerr << loc << ":" << std::endl << message << '\n';
}
