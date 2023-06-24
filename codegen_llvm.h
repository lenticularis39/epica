#ifndef EPICA_CODEGEN_LLVM_H
#define EPICA_CODEGEN_LLVM_H

#include <unordered_map>
#include <llvm/IR/IRBuilder.h>
#include "ast.h"

class CodegenLLVM {
private:
    Program *program;
    llvm::LLVMContext ctx;
    llvm::Module *mod;

    llvm::Function *current_func;
    llvm::BasicBlock *current_bb;
    llvm::Value *current_value;
    std::unordered_map<std::string, llvm::AllocaInst *> current_vars;

    void emit(Node *node);
    llvm::Type *get_type(Type t);
    llvm::FunctionType *get_function_type(Function *fun);
public:
    CodegenLLVM(Program *program);
    llvm::Module *compile();
};

#endif //EPICA_CODEGEN_LLVM_H
