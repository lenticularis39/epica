#include <algorithm>
#include "ast.h"

Node::Node(yy::location loc) : loc(loc) {}

Node::~Node() {
    for (Node *child : children)
        delete child;
}

Program::Program(yy::location loc) : Node(loc) {}

Function::Function(Type type, const std::string &name, std::vector<Parameter> params, Block *body, yy::location loc)
    : Node(loc), type(type), name(name), params(params), body(body) {}

Statement::Statement(yy::location loc) : Node(loc) {}

Block::Block(std::vector<Statement *> statements, yy::location loc) : Statement(loc) {
    std::transform(statements.begin(),
                   statements.end(),
                   std::back_inserter(children),
                   [](Statement *stmt){ return static_cast<Node *>(stmt); });
}

Variable::Variable(Type type, std::string name, yy::location loc)
    : Statement(loc), type(type), name(name) {}

Assignment::Assignment(const std::string &var_name, Expression *expr, yy::location loc)
    : Statement(loc), var_name(var_name), expr(expr) {
    children.emplace_back(static_cast<Node *>(expr));
}

While::While(Expression *pred, Statement *body, yy::location loc)
    : Statement(loc), pred(pred), body(body) {
    children.emplace_back(static_cast<Node *>(pred));
    children.emplace_back(static_cast<Node *>(body));
}

If::If(Expression *pred, Statement *positive, Statement *negative, yy::location loc)
    : Statement(loc), pred(pred), positive(positive), negative(negative) {
    children.emplace_back(static_cast<Node *>(pred));
    children.emplace_back(static_cast<Node *>(positive));
    if (negative)
        children.emplace_back(static_cast<Node *>(negative));
}

If::If(Expression *pred, Statement *positive, yy::location loc) : If(pred, positive, nullptr, loc) {}

Call::Call(const std::string &func_name, std::vector<Expression *> args, yy::location loc)
    : Statement(loc), func_name(func_name), args(args) {
    std::transform(args.begin(),
                   args.end(),
                   std::back_inserter(children),
                   [](Expression *expr){ return static_cast<Node *>(expr); });
}

Expression::Expression(yy::location loc) : Node(loc) {}

BinOp::BinOp(BinOpKind kind, Expression *left, Expression *right, yy::location loc)
    : Expression(loc), kind(kind), left(left), right(right) {
    children.emplace_back(static_cast<Node *>(left));
    children.emplace_back(static_cast<Node *>(right));
}

Integer::Integer(int value, yy::location loc) : Expression(loc), value(value) {}

Boolean::Boolean(bool value, yy::location loc) : Expression(loc), value(value) {}

Identifier::Identifier(std::string name) : Expression(loc), name(name) {}

CallExpr::CallExpr(const std::string &func_name, std::vector<Expression *> args, yy::location loc)
        : Expression(loc), func_name(func_name), args(args) {
    std::transform(args.begin(),
                   args.end(),
                   std::back_inserter(children),
                   [](Expression *expr){ return static_cast<Node *>(expr); });
}