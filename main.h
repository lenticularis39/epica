#ifndef EPICA_MAIN_H
#define EPICA_MAIN_H

#include <string>
#include <map>
#include "parser.tab.hh"

#define YY_DECL yy::parser::symbol_type yylex(Driver &drv)
YY_DECL;

class Driver {
public:
    Driver();
    int parse(const std::string &f);
    void scan_begin();
    void scan_end();

    std::string file;
    bool trace_parsing;
    bool trace_scanning;
    int result;
    yy::location location;
};

#endif //EPICA_MAIN_H
