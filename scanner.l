%{
	#include <cstdio>
	#include <vector>
	#include <string>
	#include "grammar.tab.h"
	#include "utils.hpp"
%}
%option noyywrap
%option yylineno

DIGIT [0-9]+
ID [a-zA-Z][a-zA-Z0-9_]*
%%

coefficient {
	return COEFFICIENT;
}

parameter {
	return PARAMETER;
}

stencil {
	return STENCIL;
}

iterate {
	return ITERATE;
}

inout {
	return INOUT;
}

function {
	return FUNCTION;
}

allocin {
	return ALLOCIN;
}

copyin {
	return COPYIN;
}

copyout {
	return COPYOUT;
}

constant {
	return CONSTANT;
}

shmem {
	return SHMEM;
}

no-shmem {
	return NOSHMEM;
}

gmem {
	return GMEM;
}

iterator {
	return ITERATOR;
}

bool {
	yylval.ival = BOOL;
	return DATATYPE;
}

float {
    yylval.ival = FLOAT;
	return DATATYPE;
}

double {
	yylval.ival = DOUBLE;
	return DATATYPE;
}

int {
    yylval.ival = INT;
	return DATATYPE;
}

true {
	yylval.bval = true;
	return TRUE;
}

false {
	yylval.bval = false;
	return FALSE;
}

{ID} {
	yylval.str = strdup (yytext);
	return ID;
}

{DIGIT} {
	yylval.ival = atoi(yytext);
	return T_INT;
}

{DIGIT}"."{DIGIT}[fF] {
    yylval.fval = atof(yytext);
    return T_FLOAT;
}

{DIGIT}[fF] {
    yylval.fval = atof(yytext);
    return T_FLOAT;
}

{DIGIT}"."{DIGIT} {
    yylval.dval = atof(yytext);
    return T_DOUBLE;
}

{DIGIT}[eE][-+]?{DIGIT} {
	yylval.dval = atof(yytext);
	return T_DOUBLE;
}

{DIGIT}"."{DIGIT}[eE][-+]?{DIGIT} {
	yylval.dval = atof(yytext);
	return T_DOUBLE;
}

{DIGIT}[eE][-+]?{DIGIT}[fF] {
	yylval.fval = atof(yytext);
	return T_FLOAT;
}

{DIGIT}"."{DIGIT}[eE][-+]?{DIGIT}[fF] {
	yylval.fval = atof(yytext);
	return T_FLOAT;
}

";" |
"+" |
"(" |
")" |
"{" |
"}" |
"|" |
"[" |
"]" |
"&" |
"%" |
"^" |
"," |
"-" |
">" |
"<" |
"=" |
"*" |
"/" {
	return yytext[0];
}

"<=" {
	return LEQ;
}

">=" {
	return GEQ;
}

"==" {
	return EQ;
}

"!=" {
	return NEQ;
}

"+=" {
	return PLUSEQ;
}

"-=" {
	return MINUSEQ;
}

"*=" {
	return MULTEQ;
}

"/=" {
	return DIVEQ;
}

"|=" {
	return OREQ;
}

"&=" {
	return ANDEQ;
}

":" {
	return COLON;
}

".." {
	return DDOTS;
}

"//" {
	return COMMENT;
}

[ \t\n]+ {}

<<EOF>> {
	return 0;
}
%%
