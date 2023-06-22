#ifndef EPICA_SEMANTIC_ANALYSER_H
#define EPICA_SEMANTIC_ANALYSER_H

#include <unordered_map>
#include "ast.h"

class SemanticAnalyser {
private:
    Program *program;
    std::unordered_map<std::string, Function *> function_map;
    Function *current_func;
    std::unordered_map<std::string, Parameter> current_params;
    std::unordered_map<std::string, Variable *> current_vars;

    bool resolve_types(Node *node);
    bool resolve_call(const std::string &func_name, std::vector<Expression *> args, Function *&func, yy::location loc);
public:
    SemanticAnalyser(Program *program);
    bool scan_functions();
    bool resolve_types();
    bool analyse();
};

#endif //EPICA_SEMANTIC_ANALYSER_H
