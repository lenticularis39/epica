CXXFLAGS=-O2 -std=c++17 -g

epica: parser.tab.o lexer.o main.o ast.o
	g++ $(LDFLAGS) $^ -o epica
.PHONY: clean
clean:
	rm -f parser.tab.cc parser.tab.hh location.hh lexer.c lex.yy.c
%.o: %.cxx
	g++ $(CXXFLAGS) -c $<
parser.tab.cc: parser.yy
	bison parser.yy
parser.tab.o: parser.tab.cc
	g++ $(CXXFLAGS) -c $<
lexer.o: lexer.c parser.tab.cc
	g++ $(CXXFLAGS) -c $<
