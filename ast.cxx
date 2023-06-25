#include <algorithm>
#include <cassert>
#include <unordered_map>
#include "ast.h"

Node::Node(yy::location loc, NodeKind kind) : loc(loc), kind(kind) {}

Node::~Node() {
    for (Node *child : children)
        delete child;
}

Program::Program(yy::location loc) : Node(loc, NodeKind::Program) {}

Function::Function(Type type, const std::string &name, std::vector<Parameter> params, Block *body, yy::location loc)
    : Node(loc, NodeKind::Function), type(type), name(name), params(params), body(body) {
    children.emplace_back(body);
}

Statement::Statement(yy::location loc, StatementKind kind) : Node(loc, NodeKind::Statement), kind(kind) {}

Block::Block(std::vector<Statement *> statements, yy::location loc) : Statement(loc, StatementKind::Block) {
    std::transform(statements.begin(),
                   statements.end(),
                   std::back_inserter(children),
                   [](Statement *stmt){ return static_cast<Node *>(stmt); });
}

Variable::Variable(Type type, std::string name, yy::location loc)
    : Statement(loc, StatementKind::Variable), type(type), name(name) {}

Assignment::Assignment(const std::string &var_name, Expression *expr, yy::location loc)
    : Statement(loc, StatementKind::Assignment), var_name(var_name), expr(expr) {
    children.emplace_back(static_cast<Node *>(expr));
}

While::While(Expression *pred, Statement *body, yy::location loc)
    : Statement(loc, StatementKind::While), pred(pred), body(body) {
    children.emplace_back(static_cast<Node *>(pred));
    children.emplace_back(static_cast<Node *>(body));
}

If::If(Expression *pred, Statement *positive, Statement *negative, yy::location loc)
    : Statement(loc, StatementKind::If), pred(pred), positive(positive), negative(negative) {
    children.emplace_back(static_cast<Node *>(pred));
    children.emplace_back(static_cast<Node *>(positive));
    if (negative)
        children.emplace_back(static_cast<Node *>(negative));
}

If::If(Expression *pred, Statement *positive, yy::location loc) : If(pred, positive, nullptr, loc) {}

Call::Call(const std::string &func_name, std::vector<Expression *> args, yy::location loc)
    : Statement(loc, StatementKind::Call), func_name(func_name), args(args) {
    std::transform(args.begin(),
                   args.end(),
                   std::back_inserter(children),
                   [](Expression *expr){ return static_cast<Node *>(expr); });
}

Expression::Expression(yy::location loc, ExpressionKind kind) : Node(loc, NodeKind::Expression), kind(kind) {}

BinOp::BinOp(BinOpKind kind, Expression *left, Expression *right, yy::location loc)
    : Expression(loc, ExpressionKind::BinOp), kind(kind), left(left), right(right) {
    children.emplace_back(static_cast<Node *>(left));
    children.emplace_back(static_cast<Node *>(right));
}

UnOp::UnOp(UnOpKind kind, Expression *arg, yy::location loc)
        : Expression(loc, ExpressionKind::UnOp), kind(kind), arg(arg) {
    children.emplace_back(static_cast<Node *>(arg));
}

Integer::Integer(int value, yy::location loc) : Expression(loc, ExpressionKind::Integer), value(value) {}

Boolean::Boolean(bool value, yy::location loc) : Expression(loc, ExpressionKind::Boolean), value(value) {}

Identifier::Identifier(std::string name, yy::location loc) : Expression(loc, ExpressionKind::Identifier), name(name) {}

CallExpr::CallExpr(const std::string &func_name, std::vector<Expression *> args, yy::location loc)
        : Expression(loc, ExpressionKind::CallExpr), func_name(func_name), args(args) {
    std::transform(args.begin(),
                   args.end(),
                   std::back_inserter(children),
                   [](Expression *expr){ return static_cast<Node *>(expr); });
}

/* Utility functions */
Type type_from_string(const std::string &type) {
    static std::unordered_map<std::string, Type> map = {
            {"int", Type::Int},
            {"bool", Type::Bool},
            {"void", Type::Void},
    };
    return map[type];
}

std::string type_to_string(Type type) {
    switch (type) {
        case Type::None:
            return "none";
        case Type::Int:
            return "int";
        case Type::Bool:
            return "bool";
        case Type::Char:
            return "char";
        case Type::Void:
            return "void";
        default:
            assert(false);
    }
}

bool is_builtin(const std::string &name) {
    return name == "return" || name == "read" || name == "write";
}

BinOpKind resolve_relation_operator(const std::string &op) {
    static std::unordered_map<std::string, BinOpKind> map = {
       {">", BinOpKind::Gt},
       {"<", BinOpKind::Lt},
       {">=", BinOpKind::Geq},
       {"<=", BinOpKind::Leq},
    };
    return map[op];
}

std::ostream &operator <<(std::ostream &out, Parameter par) {
    return out << type_to_string(par.type) << " " << par.name;
}