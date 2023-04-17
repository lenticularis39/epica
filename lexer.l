%option noyywrap nounput noinput batch debug

%{
    #include "parser.tab.hh"
    #include "main.h"
    #define YY_USER_ACTION loc.columns(yyleng);
%}

id    [a-zA-Z][a-zA-Z_0-9]*
int   [0-9]+
blank [ \t\r]

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
"endif"     return yy::parser::make_ENDIF(loc);
"while"     return yy::parser::make_WHILE(loc);
"do"        return yy::parser::make_DO(loc);
"endwhile"  return yy::parser::make_ENDWHILE(loc);
"commence"  return yy::parser::make_COMMENCE(loc);
"end"       return yy::parser::make_END(loc);
"var"       return yy::parser::make_VAR(loc);
"function"  return yy::parser::make_FUNCTION(loc);

{id}        return yy::parser::make_IDENT(yytext, loc);

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
