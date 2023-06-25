#include <algorithm>
#include <cassert>
#include <llvm/CodeGen/UnreachableBlockElim.h>
#include <llvm/Passes/PassBuilder.h>
#include "codegen_llvm.h"
#include "ast.h"

CodegenLLVM::CodegenLLVM(Program *program) : program(program) {}

llvm::Type *CodegenLLVM::get_type(Type t) {
    switch (t) {
        case Type::Void:
            return llvm::Type::getVoidTy(ctx);
        case Type::Bool:
            return llvm::Type::getInt1Ty(ctx);
        case Type::Int:
            return llvm::Type::getInt64Ty(ctx);
        default:
            assert(false);
    }
}

llvm::FunctionType *CodegenLLVM::get_function_type(Function *fun) {
    std::vector<llvm::Type *> param_types;
    std::transform(fun->params.begin(), fun->params.end(), std::back_inserter(param_types),
                   [this](Parameter p) { return get_type(p.type); });
    return llvm::FunctionType::get(get_type(fun->type), param_types, 0);
}

llvm::Module *CodegenLLVM::compile() {
    mod = new llvm::Module("program", ctx);

    /* Create all function prototypes */
    for (Node *child : program->children) {
        assert(child->kind == NodeKind::Function);
        Function *fun = static_cast<Function *>(child);
        llvm::Function::Create(get_function_type(fun),
                               fun->name[0] == 'x' || fun->name == "main"
                                    ? llvm::Function::ExternalLinkage
                                    : llvm::Function::InternalLinkage,
                               fun->name,
                               mod);
    }

    /* Create prototypes for builtins */
    llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getInt64Ty(ctx), {}, 0),
                           llvm::Function::ExternalLinkage,
                           "read",
                           mod);
    llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(ctx),
                                                   {llvm::Type::getInt64Ty(ctx)},
                                                   0),
                           llvm::Function::ExternalLinkage,
                           "write",
                           mod);

    /* Emit code for all functions */
    for (Node *child : program->children) {
        Function *fun = static_cast<Function *>(child);
        current_func = mod->getFunction(fun->name);
        current_bb = llvm::BasicBlock::Create(ctx, "entry", current_func);
        current_vars.clear();

        /* Create local variables for arguments.
           Note: this is necessary, since you can assign new values to them */
        int i = 0;
        for (Parameter param : fun->params) {
            llvm::AllocaInst *arg = new llvm::AllocaInst(get_type(param.type),
                                                         0,
                                                         param.name,
                                                         current_bb);
            new llvm::StoreInst(current_func->getArg(i), arg, current_bb);
            current_vars.insert({param.name, arg});
            i++;
        }

        /* Emit code for body */
        emit(static_cast<Node *>(fun->body));

        /* Add default return value */
        if (fun->type == Type::Void)
            llvm::ReturnInst::Create(ctx, current_bb);
        else
            llvm::ReturnInst::Create(ctx,
                                     llvm::ConstantInt::get(get_type(fun->type), 0),
                                     current_bb);

        /* Cleanup */
        llvm::FunctionPassManager fpm;
        llvm::FunctionAnalysisManager fam;
        llvm::PassBuilder pb;
        pb.registerFunctionAnalyses(fam);
        fpm.addPass(llvm::UnreachableBlockElimPass());
        fpm.run(*current_func, fam);
    }

    return mod;
}

void CodegenLLVM::emit(Node *node) {
    switch (node->kind) {
        case NodeKind::Expression: {
            Expression *expression = static_cast<Expression *>(node);
            switch (expression->kind) {
                case ExpressionKind::CallExpr: {
                    auto call = static_cast<CallExpr *>(expression);
                    std::vector<llvm::Value *> args;
                    for (Expression *arg : call->args) {
                        emit(static_cast<Node *>(arg));
                        args.emplace_back(current_value);
                    }
                    if (call->func_name == "read") {
                        current_value = llvm::CallInst::Create(llvm::FunctionType::get(llvm::Type::getInt64Ty(ctx), {}, 0),
                                                          mod->getFunction(call->func_name),
                                                               args,
                                                               "",
                                                               current_bb);
                    } else {
                        current_value = llvm::CallInst::Create(get_function_type(call->func),
                                                               mod->getFunction(call->func_name),
                                                               args,
                                                               "",
                                                               current_bb);
                    }
                    break;
                }
                case ExpressionKind::BinOp: {
                    BinOp *binop = static_cast<BinOp *>(expression);
                    emit(static_cast<Node *>(binop->left));
                    auto left_value = current_value;
                    emit(static_cast<Node *>(binop->right));
                    auto right_value = current_value;

                    llvm::BinaryOperator::BinaryOps int_kind;
                    llvm::CmpInst::Predicate bool_kind;

                    switch (binop->kind) {
                        case BinOpKind::Add:
                            int_kind = llvm::BinaryOperator::Add;
                            goto binint;
                        case BinOpKind::Mult:
                            int_kind = llvm::BinaryOperator::Mul;
                            goto binint;
                        case BinOpKind::Sub:
                            int_kind = llvm::BinaryOperator::Sub;
                            goto binint;
                        case BinOpKind::Or:
                        case BinOpKind::LogOr:
                            int_kind = llvm::BinaryOperator::Or;
                            goto binint;
                        case BinOpKind::And:
                        case BinOpKind::LogAnd:
                            int_kind = llvm::BinaryOperator::And;
                            goto binint;
                        case BinOpKind::Xor:
                        case BinOpKind::LogXor:
                            int_kind = llvm::BinaryOperator::Xor;
                            goto binint;
                        binint:
                            current_value = llvm::BinaryOperator::Create(int_kind,
                                                                     left_value,
                                                                     right_value,
                                                                     "",
                                                                     current_bb);
                            break;
                        case BinOpKind::Lt:
                            bool_kind = llvm::CmpInst::Predicate::ICMP_SLT;
                            goto binbool;
                        case BinOpKind::Gt:
                            bool_kind = llvm::CmpInst::Predicate::ICMP_SGT;
                            goto binbool;
                        case BinOpKind::Eq:
                            bool_kind = llvm::CmpInst::Predicate::ICMP_EQ;
                            goto binbool;
                        case BinOpKind::Leq:
                            bool_kind = llvm::CmpInst::Predicate::ICMP_SLE;
                            goto binbool;
                        case BinOpKind::Geq:
                            bool_kind = llvm::CmpInst::Predicate::ICMP_SGE;
                            goto binbool;
                        binbool:
                            current_value = llvm::CmpInst::Create(llvm::Instruction::OtherOps::ICmp,
                                                                  bool_kind,
                                                                  left_value,
                                                                  right_value,
                                                                  "",
                                                                  current_bb);
                            break;
                    }
                    break;
                }
                case ExpressionKind::UnOp: {
                    UnOp *unop = static_cast<UnOp *>(expression);
                    emit(static_cast<Node *>(unop->arg));
                    /* Note: -x    ... 0 - x
                             not x ... -1 (11...11) - x
                             !x    ... -1 - x  */
                    current_value = llvm::BinaryOperator::Create(llvm::BinaryOperator::Sub,
                                                                 llvm::ConstantInt::get(get_type(unop->type),
                                                                                        unop->kind == UnOpKind::Neg
                                                                                            ? 0 : -1),
                                                                 current_value,
                                                                 "",
                                                                 current_bb);
                    break;
                }
                case ExpressionKind::Integer: {
                    Integer *integer = static_cast<Integer *>(expression);
                    current_value = llvm::ConstantInt::get(get_type(Type::Int), integer->value);
                    break;
                }
                case ExpressionKind::Boolean: {
                    Boolean *boolean = static_cast<Boolean *>(expression);
                    current_value = llvm::ConstantInt::get(get_type(Type::Bool), boolean->value);
                    break;
                }
                case ExpressionKind::Identifier: {
                    Identifier *identifier = static_cast<Identifier *>(expression);
                    llvm::AllocaInst *var = current_vars[identifier->name];
                    current_value = new llvm::LoadInst(get_type(identifier->type),
                                                       var,
                                                       identifier->name,
                                                       current_bb);
                    break;
                }
            }
            break;
        }
        case NodeKind::Statement: {
            Statement *statement = static_cast<Statement *>(node);
            switch (statement->kind) {
                case StatementKind::Call: {
                    Call *call = static_cast<Call *>(statement);
                    std::vector<llvm::Value *> args;
                    for (Expression *arg : call->args) {
                        emit(static_cast<Node *>(arg));
                        args.emplace_back(current_value);
                    }
                    if (call->func_name == "return") {
                        if (args.empty())
                            llvm::ReturnInst::Create(ctx, current_bb);
                        else
                            llvm::ReturnInst::Create(ctx, args[0], current_bb);
                        current_bb = llvm::BasicBlock::Create(ctx, "unreach", current_func);
                    } else if (call->func_name == "write") {
                        llvm::CallInst::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(ctx),
                                                                       {llvm::Type::getInt64Ty(ctx)},
                                                                       0),
                                               mod->getFunction(call->func_name),
                                               args,
                                               "",
                                               current_bb);
                    } else {
                        llvm::CallInst::Create(get_function_type(call->func),
                                               mod->getFunction(call->func_name),
                                               args,
                                               "",
                                               current_bb);
                    }
                    break;
                }
                case StatementKind::Assignment: {
                    Assignment *assignment = static_cast<Assignment *>(statement);
                    emit(assignment->expr);
                    llvm::AllocaInst *var = current_vars[assignment->var_name];
                    current_value = new llvm::StoreInst(current_value, var, current_bb);
                    break;
                }
                case StatementKind::Variable: {
                    Variable *variable = static_cast<Variable *>(statement);
                    llvm::AllocaInst *var = new llvm::AllocaInst(get_type(variable->type),
                                                                 0,
                                                                 variable->name,
                                                                 current_bb);
                    current_vars.insert({variable->name, var});
                    break;
                }
                case StatementKind::Block: {
                    for (Node *child : statement->children)
                        emit(child);
                    break;
                }
                case StatementKind::If: {
                    If *i = static_cast<If *>(statement);
                    emit(static_cast<Node *>(i->pred));
                    llvm::Value *pred_value = current_value;

                    llvm::BasicBlock *true_branch = llvm::BasicBlock::Create(ctx, "if.true", current_func);
                    llvm::BasicBlock *false_branch = llvm::BasicBlock::Create(ctx, "if.false", current_func);
                    llvm::BasicBlock *join_branch = llvm::BasicBlock::Create(ctx, "if.join", current_func);

                    llvm::BranchInst::Create(true_branch,
                                             i->negative ? false_branch : join_branch,
                                             pred_value,
                                             current_bb);
                    current_bb = true_branch;
                    emit(static_cast<Node *>(i->positive));
                    llvm::BranchInst::Create(join_branch, current_bb);
                    if (i->negative) {
                        current_bb = false_branch;
                        emit(static_cast<Node *>(i->negative));
                        llvm::BranchInst::Create(join_branch, current_bb);
                    }
                    current_bb = join_branch;

                    break;
                }
                case StatementKind::While: {
                    While *wh = static_cast<While *>(statement);
                    llvm::BasicBlock *loop = llvm::BasicBlock::Create(ctx, "while.loop", current_func);
                    llvm::BranchInst::Create(loop, current_bb);
                    current_bb = loop;
                    emit(static_cast<Node *>(wh->body));
                    emit(static_cast<Node *>(wh->pred));
                    llvm::Value *pred = current_value;
                    llvm::BasicBlock *next = llvm::BasicBlock::Create(ctx, "while.next", current_func);
                    llvm::BranchInst::Create(loop, next, pred, current_bb);
                    current_bb = next;
                    break;
                }
            }
            break;
        }
        default:
            assert(false);
    }
}
