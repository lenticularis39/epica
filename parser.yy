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
    REL         "relational operator"
    ADD         "+"
    MULT        "*"
;

%left LOR LAND LXOR OR XOR AND EQ REL ADD MULT
%right THEN ELSE /* to avoid nested if-else SR conflict */

%%

%start program;
program: function program | function;
function: TYPE IDENT "(" parameters ")" block;
parameters: parameter "," parameters | parameter;
parameter: TYPE IDENT;
block: COMMENCE statements END;
statements: statement statements | statement;

statement: block | declaration | assignment | if | while | call;
declaration: VAR TYPE IDENT;
assignment: IDENT ":=" expression;
arguments: expression "," arguments | expression;

if: IF expression THEN statement | IF expression THEN statement ELSE statement;
while: WHILE expression DO statement;

expression: logical_or;
simple: literal | variable | call | "(" expression ")";
literal: integer | bool;
integer: INT;
bool: BOOL;
variable: IDENT;
call: IDENT "(" arguments ")" | IDENT "(" ")";
/* Cascade operation grammar rules according to precedence (from lowest
   to highest, like in E -> E + T, ... grammar) */
logical_or: logical_xor | logical_or "|" logical_xor
logical_xor: logical_and | logical_xor "^" logical_and;
logical_and: or | logical_and "&" or;
or: xor | xor "or" and;
xor: and | and "xor" xor;
and: equality | and "and" equality;
equality: relation | equality "=" relation;
relation: add | relation REL add;
add: multiply | add "+" multiply;
multiply: simple | multiply "*" simple;

%%

void yy::parser::error (const location_type &l, const std::string &m) {
  std::cerr << l << ": " << m << '\n';
}