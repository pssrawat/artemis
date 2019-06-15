FLEX=flex
BISON=bison
CC=g++

CFLAGS=-g -O3 -Wall -std=c++11
DEBUGFLAGS=-DDEBUG=true
OPTFLAGS=-DCONSOLIDATE=true

default : stencilgen

exprnode.o : exprnode.cpp exprnode.hpp utils.hpp 
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(OPTFLAGS) -c exprnode.cpp
funcdefn.o : funcdefn.cpp funcdefn.hpp arrdecl.hpp
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(OPTFLAGS) -c funcdefn.cpp
codegen.o : codegen.cpp codegen.hpp utils.hpp 
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(OPTFLAGS) -c codegen.cpp

stencilgen : lex.yy.c grammar.tab.c main.cpp utils.hpp exprnode.o funcdefn.o codegen.o
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(OPTFLAGS) -o stencilgen main.cpp utils.hpp exprnode.o codegen.o funcdefn.o lex.yy.c grammar.tab.c
 
all : lex.yy.c grammar.tab.c main.cpp
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(OPTFLAGS) -c exprnode.cpp
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(OPTFLAGS) -c funcdefn.cpp
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(OPTFLAGS) -c codegen.cpp
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(OPTFLAGS) -o stencilgen main.cpp exprnode.o codegen.o funcdefn.o lex.yy.c grammar.tab.c

lex.yy.c : scanner.l
	$(FLEX) scanner.l

grammar.tab.c : grammar.y
	$(BISON) -d grammar.y

clean:
	-@rm stencilgen *.o lex.yy.* grammar.tab.* out.cu orig_out.cu tests/out.cu tests/orig_out.cu 2>/dev/null || true
