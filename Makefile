CXXFLAGS=-O2 -std=c++20 -g -Wall -Wextra

epica: parser.tab.o lexer.o main.o ast.o error.o semantic_analyser.o
	g++ $(LDFLAGS) $^ -o epica
.PHONY: clean
clean:
	rm -f parser.tab.cc parser.tab.hh location.hh lexer.c lex.yy.c *.o
%.o: %.cxx
	g++ $(CXXFLAGS) -c $<
parser.tab.cc: parser.yy
	bison parser.yy
parser.tab.o: parser.tab.cc
	g++ $(CXXFLAGS) -c $<
lexer.o: lexer.c parser.tab.cc
	g++ $(CXXFLAGS) -c $<
