CXXFLAGS=-O2 -std=c++20 -g -Wall -Wextra
LDFLAGS=-lLLVM-16

all: epica libepica.o
epica: parser.tab.o lexer.o main.o ast.o error.o semantic_analyser.o codegen_llvm.o
	g++ $(LDFLAGS) $^ -o epica
libepica.o: libepica.c
	gcc -c $<
.PHONY: clean
clean:
	rm -f parser.tab.cc parser.tab.hh location.hh lexer.c lex.yy.c *.o epica
%.o: %.cxx
	g++ $(CXXFLAGS) -c $<
parser.tab.cc: parser.yy
	bison parser.yy
parser.tab.o: parser.tab.cc
	g++ $(CXXFLAGS) -c $<
lexer.o: lexer.c parser.tab.cc
	g++ $(CXXFLAGS) -c $<
