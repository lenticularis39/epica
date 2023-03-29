CXXFLAGS=-O2 -std=c++17

epica: main.o lexer.o parser.tab.o
	g++ $(LDFLAGS) $^ -o epica
%.o: %.cxx
	g++ $(CXXFLAGS) -c $<
parser.tab.cc: parser.yy
	bison parser.yy
parser.tab.o: parser.tab.cc
	g++ $(CXXFLAGS) -c $<
lexer.o: lexer.c parser.tab.cc
	g++ $(CXXFLAGS) -c $<
