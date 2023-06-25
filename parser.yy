%skeleton "lalr1.cc"
%require "3.8.1"
%header

%define api.token.raw
%define api.token.constructor
%define api.value.type variant
%define parse.trace
%define parse.error detailed
%define parse.lac full

%code requires {
    #include <string>
    #include "ast.h"
    #include "error.h"
    class Driver;
}

%param { Driver &drv }
%locations
%printer { yyo << $$; } <*>;

%code {
    #include "main.h"
}

%token <std::string> IDENT "identifier"
%token <std::string> TYPE "type"
%token <std::string> INT "integer literal"
%token <std::string> BOOL "boolean literal"
%token <std::string> REL "relational operator"
%token
    ENDOFFILE 0 "end of file"
    IF          "if"
    THEN        "then"
    ELSE        "else"
    WHILE       "while"
    DO          "do"
    COMMENCE    "commence"
    END         "end"
    VAR         "var"

    LPAREN      "("
    RPAREN      ")"
    COMMA       ","
    ASSIGN      ":="
    LOR         "|"
    LAND        "&"
    LXOR        "^"
    OR          "or"
    XOR         "xor"
    AND         "and"
    EQ          "="
    ADD         "+"
    MULT        "*"
    SUB         "-"
    NOT         "not"
    LNOT        "!"
;

%type <Node *> program;
%type <Function *> function;
%type <std::vector<Parameter> *> parameters;
%type <Parameter> parameter;
%type <Block *> block;
%type <std::vector<Statement *> *> statements;
%type <Statement *> statement;
%type <Variable *> declaration;
%type <Assignment *> assignment;
%type <std::vector<Expression *> *> arguments;
%type <If *> if;
%type <While *> while;
%type <Call *> call;
%type <Expression *> expression simple literal logical_or logical_xor logical_and or xor and equality relation add multiply;
%type <Identifier *> variable;
%type <CallExpr *> call_expr;
%type <Integer *> integer;
%type <Boolean *> bool;

%left LOR LAND LXOR OR XOR AND EQ REL ADD MULT SUB
%right THEN ELSE /* to avoid nested if-else SR conflict */

%%

%start program;
program: program function { $1->children.emplace_back($2); $$ = $1; }
         | function       { drv.root = $$ = new Program(@$); $$->children.emplace_back($1); }
         ;
function: TYPE IDENT "(" parameters ")" block  {
            $$ = new Function(type_from_string($1), $2, *$4, $6, @$);
            delete $4;
          }
          | TYPE IDENT "(" ")" block {
            $$ = new Function(type_from_string($1), $2, {}, $5, @$);
          }
          ;
parameters: parameters "," parameter { $1->emplace_back($3); $$ = $1; }
            | parameter              { $$ = new std::vector<Parameter>; $$->emplace_back($1); }
            ;
parameter: TYPE IDENT { $$ = {type_from_string($1), $2}; }
           ;
block: COMMENCE statements END {
         $$ = new Block(*$2, @$);
         delete $2;
       }
       | COMMENCE END          { $$ = new Block({}, @$); }
       ;
statements: statements statement { $1->emplace_back($2); $$ = $1; }
            | statement          { $$ = new std::vector<Statement *>; $$->emplace_back($1); }
            ;

statement: block         { $$ = static_cast<Statement *>($1); }
           | declaration { $$ = static_cast<Statement *>($1); }
           | assignment  { $$ = static_cast<Statement *>($1); }
           | if          { $$ = static_cast<Statement *>($1); }
           | while       { $$ = static_cast<Statement *>($1); }
           | call        { $$ = static_cast<Statement *>($1); }
           ;
declaration: VAR TYPE IDENT { $$ = new Variable(type_from_string($2), $3, @$); }
             ;
assignment: IDENT ":=" expression { $$ = new Assignment($1, $3, @$); }
            ;
arguments: arguments "," expression { $1->emplace_back($3); $$ = $1; }
           | expression             { $$ = new std::vector<Expression *>; $$->emplace_back($1); }
           ;

if: IF expression THEN statement                  { $$ = new If($2, $4, @$); }
    | IF expression THEN statement ELSE statement { $$ = new If($2, $4, $6, @$); }
    ;
while: WHILE expression DO statement { $$ = new While($2, $4, @$); }
       ;
call: IDENT "(" arguments ")" {
        $$ = new Call($1, *$3, @$);
        delete $3;
      }
      | IDENT "(" ")"         { $$ = new Call($1, {}, @$); }
      ;

expression: logical_or { $$ = static_cast<Expression *>($1); }
            ;
simple: literal              { $$ = $1; }
        | variable           { $$ = static_cast<Expression *>($1); }
        | call_expr          { $$ = static_cast<Expression *>($1); }
        | "-" simple         { $$ = static_cast<Expression *>(new UnOp(UnOpKind::Neg, $2, @$)); }
        | NOT simple         { $$ = static_cast<Expression *>(new UnOp(UnOpKind::Not, $2, @$)); }
        | "!" simple         { $$ = static_cast<Expression *>(new UnOp(UnOpKind::LogNot, $2, @$)); }
        | "(" expression ")" { $$ = $2; }
        ;
literal: integer { $$ = static_cast<Expression *>($1); }
         | bool  { $$ = static_cast<Expression *>($1); }
         ;
integer: INT { $$ = new Integer(std::stoi($1), @$); }
         ;
bool: BOOL { $$ = new Boolean($1 == "true" ? true : false, @$); }
      ;
variable: IDENT { $$ = new Identifier($1, @$); }
          ;
call_expr: IDENT "(" arguments ")" {
             $$ = new CallExpr($1, *$3, @$);
             delete $3;
           }
           | IDENT "(" ")"         { $$ = new CallExpr($1, {}, @$); }
           ;
/* Cascade operation grammar rules according to precedence (from lowest
   to highest, like in E -> E + T, ... grammar) */
logical_or: logical_xor                  { $$ = $1; }
            | logical_or "|" logical_xor { $$ = static_cast<Expression *>(new BinOp(BinOpKind::LogOr, $1, $3, @$)); }
            ;
logical_xor: logical_and                   { $$ = $1; }
             | logical_xor "^" logical_and { $$ = static_cast<Expression *>(new BinOp(BinOpKind::LogXor, $1, $3, @$)); }
             ;
logical_and: or                   { $$ = $1; }
             | logical_and "&" or { $$ = static_cast<Expression *>(new BinOp(BinOpKind::LogAnd, $1, $3, @$)); }
             ;
or: xor            { $$ = $1; }
    | xor "or" and { $$ = static_cast<Expression *>(new BinOp(BinOpKind::Or, $1, $3, @$)); }
    ;
xor: and             { $$ = $1; }
     | and "xor" xor { $$ = static_cast<Expression *>(new BinOp(BinOpKind::Xor, $1, $3, @$)); }
     ;
and: equality             { $$ = $1; }
     | and "and" equality { $$ = static_cast<Expression *>(new BinOp(BinOpKind::And, $1, $3, @$)); }
     ;
equality: relation                { $$ = $1; }
          | equality "=" relation { $$ = static_cast<Expression *>(new BinOp(BinOpKind::Eq, $1, $3, @$)); }
          ;
relation: add                { $$ = $1; }
          | relation REL add { $$ = static_cast<Expression *>(new BinOp(resolve_relation_operator($2), $1, $3, @$)); }
          ;
add: multiply            { $$ = $1; }
     | add "+" multiply { $$ = static_cast<Expression *>(new BinOp(BinOpKind::Add, $1, $3, @$)); }
     | add "-" multiply { $$ = static_cast<Expression *>(new BinOp(BinOpKind::Sub, $1, $3, @$)); }
     ;
multiply: simple                { $$ = $1; }
          | multiply "*" simple { $$ = static_cast<Expression *>(new BinOp(BinOpKind::Mult, $1, $3, @$)); }
          ;

%%

void yy::parser::error (const location_type &l, const std::string &m) {
  ast_error(m, l);
}