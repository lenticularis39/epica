%option noyywrap nounput noinput batch debug

%{
    #include "parser.tab.hh"
    #include "main.h"
    #define YY_USER_ACTION loc.columns(yyleng);
%}

id    [a-zA-Z][a-zA-Z_0-9]*
type  int|bool|void
int   [0-9]+
bool  true|false
blank [ \t\r]

rel   >|<|>=|<=

%%

%{
    yy::location &loc = drv.location;
    loc.step();
%}

{blank}+    loc.step();
\n+         loc.lines(yyleng); loc.step();
<<EOF>>     return yy::parser::make_ENDOFFILE(loc);

"if"        return yy::parser::make_IF(loc);
"then"      return yy::parser::make_THEN(loc);
"else"      return yy::parser::make_ELSE(loc);
"while"     return yy::parser::make_WHILE(loc);
"do"        return yy::parser::make_DO(loc);
"commence"  return yy::parser::make_COMMENCE(loc);
"end"       return yy::parser::make_END(loc);
"var"       return yy::parser::make_VAR(loc);

"("         return yy::parser::make_LPAREN(loc);
")"         return yy::parser::make_RPAREN(loc);
","         return yy::parser::make_COMMA(loc);
":="        return yy::parser::make_ASSIGN(loc);
"|"         return yy::parser::make_LOR(loc);
"&"         return yy::parser::make_LAND(loc);
"^"         return yy::parser::make_LXOR(loc);
"or"        return yy::parser::make_OR(loc);
"xor"       return yy::parser::make_XOR(loc);
"and"       return yy::parser::make_AND(loc);
"="         return yy::parser::make_EQ(loc);
{rel}       return yy::parser::make_REL(yytext, loc);
"+"         return yy::parser::make_ADD(loc);
"*"         return yy::parser::make_MULT(loc);

{type}      return yy::parser::make_TYPE(yytext, loc);
{bool}      return yy::parser::make_BOOL(yytext, loc);
{id}        return yy::parser::make_IDENT(yytext, loc);
{int}       return yy::parser::make_INT(yytext, loc);
%%

void Driver::scan_begin () {
  yy_flex_debug = trace_scanning;
  if (file.empty () || file == "-")
    yyin = stdin;
  else if (!(yyin = fopen (file.c_str (), "r"))) {
      std::cerr << "cannot open " << file << ": " << strerror (errno) << '\n';
      exit (EXIT_FAILURE);
  }
}

void Driver::scan_end() {
  fclose (yyin);
}
