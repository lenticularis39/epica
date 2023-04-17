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

%token <std::string> IDENT
%token
    ENDOFFILE 0 "end of file"
    IF          "if"
    THEN        "then"
    ELSE        "else"
    ENDIF       "endif"
    WHILE       "while"
    DO          "do"
    ENDWHILE    "endwhile"
    COMMENCE    "commence"
    END         "end"
    VAR         "var"
    FUNCTION    "function"
;

%%

%start id;
id: IDENT

%%

void yy::parser::error (const location_type &l, const std::string &m) {
  std::cerr << l << ": " << m << '\n';
}