#ifndef __DATATYPES_HPP__
#define __DATATYPES_HPP__
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <map>
#include <cassert>

// All the enums used
enum DATA_TYPE {
	INT=0,
	FLOAT,
	DOUBLE,
	BOOL
};

enum ETYPE {
	T_DATATYPE=0,
	T_UMINUS,
	T_BINARY,
	T_ID,
	T_SHIFTVEC,
	T_FUNCTION	
};

enum STMT_OP {
	ST_PLUSEQ=0,
	ST_MINUSEQ,
	ST_MULTEQ,
	ST_DIVEQ,
	ST_ANDEQ,
	ST_OREQ,
	ST_EQ
};

enum OP_TYPE {
	T_PLUS=0,
	T_MINUS,
	T_MULT,
	T_DIV,
	T_MOD,
	T_EXP,
	T_LEQ,
	T_GEQ,
	T_NEQ,
	T_EQ,
	T_LT,
	T_GT,
	T_OR,
	T_AND
};

enum DECL_TYPE {
	VAR_DECL=0,
	ARRAY_DECL
};

enum RESOURCE {
	REGISTER=0,
	SHARED_MEM,
	GLOBAL_MEM
};

#endif
