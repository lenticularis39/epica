#include <cassert>
#include <format>
#include <sstream>
#include "semantic_analyser.h"
#include "error.h"

SemanticAnalyser::SemanticAnalyser(Program *program) : program(program) {}

bool SemanticAnalyser::scan_functions() {
    for (Node *child : program->children) {
        assert(child->kind == NodeKind::Function);
        Function *func = static_cast<Function *>(child);
        auto existing_func = function_map.find(func->name);
        if (function_map.find(func->name) != function_map.end()) {
            std::stringstream loc_stream;
            loc_stream << existing_func->second->loc;
            ast_error(std::format("function {} redefined (previous definition: {})",
                                           func->name, loc_stream.str()), func->loc);
            return false;
        }
        function_map.insert({func->name, func});
    }
    return true;
}

bool SemanticAnalyser::resolve_types(Node *node) {
    /* Resolve inherited attributes */
    switch (node->kind) {
        case NodeKind::Function:
            current_func = static_cast<Function *>(node);
            current_vars.clear();
            current_params.clear();
            for (Parameter &param : current_func->params)
                current_params.insert({param.name, param});
            break;
        case NodeKind::Statement: {
            Statement *statement = static_cast<Statement *>(node);
            if (statement->kind == StatementKind::Variable) {
                Variable *var = static_cast<Variable *>(statement);
                auto existing_var = current_vars.find(var->name);
                if (var->type == Type::Void) {
                    ast_error(std::format("variable {} is of type void", var->name), var->loc);
                    return false;
                }
                if (existing_var != current_vars.end()) {
                    std::stringstream loc_stream;
                    loc_stream << existing_var->second->loc;
                    ast_error(std::format("variable {} redefined (previous definition: {})",
                                                  var->name, loc_stream.str()), var->loc);
                    return false;
                }
                auto existing_param = current_params.find(var->name);
                if (existing_param != current_params.end()) {
                    ast_error(std::format("variable {} conflicts with function parameter",
                                                   var->name), var->loc);
                    return false;
                }
                current_vars.insert({var->name, var});
            }
            break;
        }
        default:
            /* Handled below */
            ;
    }

    for (Node *child : node->children) {
        if (!resolve_types(child))
            return false;
    }

    /* Resolve synthesised attributes */
    switch (node->kind) {
        case NodeKind::Statement: {
            Statement *statement = static_cast<Statement *>(node);
            switch (statement->kind) {
                case StatementKind::While: {
                    While *wh = static_cast<While *>(statement);
                    if (wh->pred->type != Type::Bool) {
                        ast_error(std::format("while predicate is of type {}, bool expected",
                                              type_to_string(wh->pred->type)), wh->loc);
                        return false;
                    }
                    break;
                }
                case StatementKind::If: {
                    If *i = static_cast<If *>(statement);
                    if (i->pred->type != Type::Bool) {
                        ast_error(std::format("if predicate is of type {}, bool expected",
                                              type_to_string(i->pred->type)), i->loc);
                        return false;
                    }
                    break;
                }
                case StatementKind::Assignment: {
                    Assignment *assignment = static_cast<Assignment *>(statement);
                    auto variable = current_vars.find(assignment->var_name);
                    auto parameter = current_params.find(assignment->var_name);
                    Type type;
                    if (variable != current_vars.end()) {
                        type = variable->second->type;
                    } else if (parameter != current_params.end()) {
                        type = parameter->second.type;
                    } else {
                        ast_error(std::format("identifier {} undeclared",
                                              assignment->var_name), assignment->loc);
                        return false;
                    }
                    if (type != assignment->expr->type) {
                        ast_error(std::format("assigning {} to {}, which is of type {}",
                                              type_to_string(assignment->expr->type), assignment->var_name,
                                              type_to_string(type)),
                                  assignment->loc);
                    }
                    break;
                }
                case StatementKind::Call: {
                    Call *call = static_cast<Call *>(statement);
                    if (!resolve_call(call->func_name, call->args, call->func, call->loc))
                        return false;
                    break;
                }
                default:
                    ;
            }
            break;
        }
        case NodeKind::Expression: {
            Expression *expr = static_cast<Expression *>(node);
            switch (expr->kind) {
                case ExpressionKind::Identifier: {
                    Identifier *ident = static_cast<Identifier *>(expr);
                    auto variable = current_vars.find(ident->name);
                    auto parameter = current_params.find(ident->name);
                    if (variable != current_vars.end()) {
                        ident->type = variable->second->type;
                    } else if (parameter != current_params.end()) {
                        ident->type = parameter->second.type;
                    } else {
                        ast_error(std::format("identifier {} undeclared",
                                              ident->name), ident->loc);
                        return false;
                    }
                    break;
                }
                case ExpressionKind::Boolean:
                    expr->type = Type::Bool;
                    break;
                case ExpressionKind::Integer:
                    expr->type = Type::Int;
                    break;
                case ExpressionKind::BinOp: {
                    BinOp *binop = static_cast<BinOp *>(expr);
                    switch (binop->kind) {
                        case BinOpKind::Leq:
                        case BinOpKind::Geq:
                        case BinOpKind::Gt:
                        case BinOpKind::Lt:
                            if (binop->left->type != Type::Int || binop->right->type != Type::Int) {
                                ast_error("relation operator arguments must be int", binop->loc);
                                return false;
                            }
                            binop->type = Type::Bool;
                            break;
                        case BinOpKind::Eq:
                            if (binop->left->type != binop->right->type) {
                                ast_error("only values of same type may be compared", binop->loc);
                                return false;
                            }
                            binop->type = Type::Bool;
                            break;
                        case BinOpKind::LogOr:
                        case BinOpKind::LogXor:
                        case BinOpKind::LogAnd:
                            if (binop->left->type != Type::Bool || binop->right->type != Type::Bool) {
                                ast_error("logical operator arguments must be bool", binop->loc);
                                return false;
                            }
                            binop->type = Type::Bool;
                            break;
                        case BinOpKind::Or:
                        case BinOpKind::Xor:
                        case BinOpKind::And:
                        case BinOpKind::Add:
                        case BinOpKind::Mult:
                            if (binop->left->type != Type::Int || binop->right->type != Type::Int) {
                                ast_error("arithmetic operator arguments must be bool", binop->loc);
                                return false;
                            }
                            binop->type = Type::Int;
                            break;
                    }
                    break;
                }
                case ExpressionKind::CallExpr: {
                    CallExpr *call = static_cast<CallExpr *>(expr);
                    if (!resolve_call(call->func_name, call->args, call->func, call->loc))
                        return false;
                    expr->type = call->func->type;
                    break;
                }
            }
            break;
        }
        default:
            /* Handled above */
            ;
    }

    return true;
}

bool SemanticAnalyser::resolve_call(const std::string &func_name, std::vector<Expression *> args,
                                    Function *&func, yy::location loc) {
    /* Check if function is defined */
    auto func_iter = function_map.find(func_name);
    if (func_iter == function_map.end()) {
        ast_error(std::format("function {} not defined", func_name), loc);
        return false;
    }
    func = func_iter->second;

    /* Check if arguments are correct */
    if (args.size() != func->params.size()) {
        ast_error(std::format("function {} takes {} arguments, {} given",
                               func_name, func->params.size(), args.size()), loc);
        return false;
    }
    int i = 0;
    for (Expression *arg : args) {
        if (arg->type != func->params[i].type) {
            ast_error(std::format("argument {} has type {}, {} expected",
                                  i, type_to_string(arg->type),
                                  type_to_string(func->params[i].type)), arg->loc);
            return false;
        }
    }

    return true;
}

bool SemanticAnalyser::resolve_types() {
    return resolve_types(static_cast<Node *>(program));
}

bool SemanticAnalyser::analyse() {
    return scan_functions() && resolve_types();
}