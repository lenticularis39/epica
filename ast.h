#ifndef EPICA_AST_H
#define EPICA_AST_H

#include <vector>
#include <unordered_set>
#include "location.hh"

enum class Type {
    None,
    Int,
    Char,
    Bool,
    Void,
};
Type type_from_string(const std::string &type);
std::string type_to_string(Type type);

enum class NodeKind {
    Program,
    Function,
    Statement,
    Expression,
};
class Node {
public:
    Node(yy::location loc, NodeKind kind);
    ~Node();
    yy::location loc;
    NodeKind kind;
    std::vector<Node *> children;
};

class Function;
class Program : public Node {
public:
    Program(yy::location loc);
};

struct Parameter {
    Type type;
    std::string name;
};
std::ostream &operator <<(std::ostream &out, Parameter par);

class Variable;
class Block;
class Function : public Node {
public:
    Function(Type type, const std::string &name, std::vector<Parameter> params, Block *body, yy::location loc);
    Type type;
    std::string name;
    std::vector<Parameter> params;
    Block *body;
    std::vector<Variable *> vars;
};

enum class StatementKind {
    Block,
    Variable,
    Assignment,
    While,
    If,
    Call,
};
class Statement : public Node {
public:
    Statement(yy::location loc, StatementKind kind);
    StatementKind kind;
};

class Block : public Statement {
public:
    Block(std::vector<Statement *> statements, yy::location loc);
};

class Expression;
class Variable : public Statement {
public:
    Variable(Type type, std::string name, yy::location loc);
    Type type;
    std::string name;
};

class Assignment : public Statement {
public:
    Assignment(const std::string &var_name, Expression *expr, yy::location loc);
    std::string var_name;
    Expression *expr;
};

class While : public Statement {
public:
    While(Expression *pred, Statement *body, yy::location loc);
    Expression *pred;
    Statement *body;
};

class If : public Statement {
public:
    If(Expression *pred, Statement *positive, Statement *negative, yy::location loc);
    If(Expression *pred, Statement *positive, yy::location loc);
    Expression *pred;
    Statement *positive;
    Statement *negative;
};

class Call : public Statement {
public:
    Call(const std::string &func_name, std::vector<Expression *> args, yy::location loc);
    std::string func_name;
    Function *func;
    std::vector<Expression *> args;
};

enum class ExpressionKind {
    BinOp,
    UnOp,
    Integer,
    Boolean,
    Identifier,
    CallExpr,
};
class Expression : public Node {
public:
    Expression(yy::location loc, ExpressionKind kind);
    ExpressionKind kind;
    Type type;
};

enum class BinOpKind {
    LogOr,
    LogAnd,
    LogXor,
    Or,
    And,
    Xor,
    Eq,
    Gt,
    Geq,
    Lt,
    Leq,
    Add,
    Mult,
    Sub,
};
BinOpKind resolve_relation_operator(const std::string &op);

class BinOp : public Expression {
public:
    BinOp(BinOpKind kind, Expression *left, Expression *right, yy::location loc);
    BinOpKind kind;
    Expression *left;
    Expression *right;
};

enum class UnOpKind {
    Not,
    LogNot,
    Neg,
};
class UnOp : public Expression {
public:
    UnOp(UnOpKind kind, Expression *arg, yy::location loc);
    UnOpKind kind;
    Expression *arg;
};

class Integer : public Expression {
public:
    Integer(int value, yy::location loc);
    int value;
};

class Boolean : public Expression {
public:
    Boolean(bool value, yy::location loc);
    bool value;
};

class Identifier : public Expression {
public:
    Identifier(std::string name, yy::location loc);
    std::string name;
};

class CallExpr : public Expression {
public:
    CallExpr(const std::string &func_name, std::vector<Expression *> args, yy::location loc);
    std::string func_name;
    Function *func;
    std::vector<Expression *> args;
};

bool is_builtin(const std::string &name);

#endif //EPICA_AST_H
