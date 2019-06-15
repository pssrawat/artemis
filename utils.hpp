#ifndef __UTILS_HPP__
#define __UTILS_HPP__
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <map>
#include <cassert>
#include <vector>
#include <algorithm>
#include "datatypes.hpp"

inline DATA_TYPE get_data_type (int a) {
	DATA_TYPE ret = INT;
	switch (a) {
		case 0:
			ret = INT;
			break;
		case 1:
			ret = FLOAT;
			break;
		case 2:
			ret = DOUBLE;
			break;
		case 3:
			ret = BOOL;
			break;
		default:
			fprintf (stderr, "Data type not supported\n");
			exit (1);
	}
	return ret;
}

inline int get_data_size (DATA_TYPE a) {
	int ret = 8;
	switch (a) {
		case INT:
			ret = 4;
			break;
		case FLOAT:
			ret = 4;
			break;
		case DOUBLE:
			ret = 8;
			break;
		case BOOL:
			ret = 1;
			break;
		default:
			fprintf (stderr, "Data type not supported\n");
			exit (1);
	}
	return ret;
}

inline DATA_TYPE infer_data_type (DATA_TYPE a, DATA_TYPE b) {
	if (a == DOUBLE || b == DOUBLE) 
		return DOUBLE;
	if (a == FLOAT || b == FLOAT) 
		return FLOAT; 
	if (a == INT || b == INT)
		return INT;
	return BOOL;
}

inline std::string print_stmt_op (STMT_OP op_type) {
	std::string str;
	switch (op_type) {
		case ST_PLUSEQ:
			str = std::string (" += ");
			break;
		case ST_MINUSEQ:
			str = std::string (" -= ");
			break;
		case ST_MULTEQ:
			str = std::string (" *= ");
			break;
		case ST_DIVEQ:
			str = std::string (" /= ");
			break;
		case ST_EQ:
			str = std::string (" = ");
			break;
		default:
			fprintf (stderr, "Statement op type not supported\n");
			exit (1);
	}
	return str; 
}

inline std::string print_operator (OP_TYPE op, DATA_TYPE type) {
	std::string str;
	std::string tail = (type == DOUBLE) ? "pd" : "ps";
	switch (op) {
		case T_PLUS:
			str = std::string ("_mm256_add_") + tail;
			break;
		case T_MINUS:
			str = std::string ("_mm256_sub_") + tail;
			break;
		case T_MULT:
			str = std::string ("_mm256_mul_") + tail;
			break;
		case T_DIV:
			str = std::string ("_mm256_div_") + tail;
			break;
		case T_OR:
			str = std::string ("_mm256_or_") + tail;
			break;
		case T_AND:
			str = std::string ("_mm256_and_") + tail;
			break;
		default:
			fprintf (stderr, "Operator not supported\n");
			exit (1);
	}
	return str;
}

inline std::string print_operator (OP_TYPE op) {
	std::string str;
	switch (op) {
		case T_PLUS:
			str = std::string (" + ");
			break;
		case T_MINUS:
			str = std::string (" - ");
			break;
		case T_MULT:
			str = std::string (" * ");
			break;
		case T_DIV:
			str = std::string (" / ");
			break;
		case T_MOD:
			str = std::string (" % ");
			break;
		case T_EXP:
			str = std::string (" ^ ");
			break;
		case T_LEQ:
			str = std::string (" <= ");
			break;
		case T_GEQ:
			str = std::string (" >= ");
			break;
		case T_NEQ:
			str = std::string (" != ");
			break;
		case T_EQ:
			str = std::string (" == ");
			break;
		case T_LT:
			str = std::string (" < ");
			break;
		case T_GT:
			str = std::string (" > ");
			break;
		case T_OR:
			str = std::string (" | ");
			break;
		case T_AND:
			str = std::string (" & ");
			break;
		default:
			fprintf (stderr, "Operator not supported\n");
			exit (1);
	}
	return str;
}

inline std::string print_data_type (DATA_TYPE a) {
	std::string ret;
	switch (a) {
		case INT:
			ret = std::string ("int ");
			break;
		case FLOAT:
			ret = std::string ("float ");
			break;
		case DOUBLE:
			ret = std::string ("double ");
			break;
		case BOOL: 
			ret = std::string ("bool ");
			break;
		default:
			fprintf (stderr, "Data type not supported\n");
			exit (1);
	}
	return ret;
}

inline STMT_OP convert_op_to_stmt_op (OP_TYPE a) {
	STMT_OP ret = ST_EQ;
	switch (a) {
		case T_PLUS:
			ret = ST_PLUSEQ;
			break;
		case T_MINUS:
			ret = ST_MINUSEQ;
			break;
		case T_MULT:
			ret = ST_MULTEQ;
			break;
		case T_AND:
			ret = ST_ANDEQ;
			break;
		case T_OR:
			ret = ST_OREQ;
			break;
		case T_EQ:
			ret = ST_EQ;
			break;
		default:
			fprintf (stderr, "OP_TYPE not supported\n");
			exit (1);
	}
	return ret;
}

inline OP_TYPE convert_stmt_op_to_op (STMT_OP a) {
	OP_TYPE ret;
	switch (a) {
		case ST_PLUSEQ:
			ret = T_PLUS;
			break;
		case ST_MINUSEQ:
			ret = T_MINUS;
			break;
		case ST_MULTEQ:
			ret = T_MULT;
			break;
		case ST_ANDEQ:
			ret = T_AND;
			break;
		case ST_OREQ:
			ret = T_OR;
			break;
		case ST_EQ:
			ret = T_EQ;
			break;
		default:
			fprintf (stderr, "STMT_OP not supported\n");
			exit (1);
	}
	return ret;
}

inline int get_init_val (STMT_OP a) {
	int ret = 0;
	switch (a) {
		case ST_PLUSEQ:
			ret = 0;
			break;
		case ST_MINUSEQ:
			ret = 0;
			break;
		case ST_MULTEQ:
			ret = 1;
			break;
		case ST_DIVEQ:
			ret = 1;
			break;
		case ST_ANDEQ:
			ret = 1;
			break;
		case ST_OREQ:
			ret = 0;
			break;
		case ST_EQ:
			ret = 0;
			break;
		default:
			fprintf (stderr, "Operation not associative\n");
			exit (1);
	}
	return ret;
}

inline STMT_OP get_acc_op (OP_TYPE a, bool flip) {
	STMT_OP ret = ST_PLUSEQ;
	switch (a) {
		case T_PLUS:
			ret = flip ? ST_MINUSEQ : ST_PLUSEQ;
			break;
		case T_MINUS:
			ret = flip ? ST_PLUSEQ : ST_MINUSEQ;
			break;
		default:
			fprintf (stderr, "Operation not accumulation\n");
			exit (1);
	}
	return ret;
}

inline RESOURCE downgrade_resource (RESOURCE res, bool use_shmem) {
	RESOURCE ret = res;
	if (res == SHARED_MEM && !use_shmem) {
		ret = GLOBAL_MEM;
	}
	if (res == REGISTER) {
		ret = use_shmem ? SHARED_MEM : GLOBAL_MEM;
	}
	return ret;
}

inline std::string print_resource (RESOURCE res, int offset, bool stream) {
	std::string out ("");
	switch (res) {
		case GLOBAL_MEM:
			break;
		case SHARED_MEM:
			out = "_shm";
			if (stream) {
				std::string append = offset == 0 ? "_c" : (offset > 0 ? "_p" : "_m");
				out = out + append + std::to_string(std::abs(offset));	
			}
			break;
		case REGISTER:
			out = "_reg";
			if (stream) {
				std::string append = offset == 0 ? "_c" : (offset > 0 ? "_p" : "_m");
				out = out + append + std::to_string(std::abs(offset));	
			}
			break;
		default:
			fprintf (stderr, "No such resource (print_resource)\n");
			exit(1);
	}
	return out;
}

inline int flops_per_function (std::string func_name) {
	int ret;
	if (func_name.compare ("sin") == 0)
		ret = 14;
	else if (func_name.compare ("cos") == 0)
		ret = 14;
	else if (func_name.compare ("tan") == 0)
		ret = 14;
	else if (func_name.compare ("sqrt") == 0)
		ret = 6;
 	else if (func_name.compare ("exp") == 0)
		ret = 8;
	else if (func_name.compare ("atan") == 0)
		ret = 24;
	else 
		ret = 6;
	return ret;
}

inline int flops_per_op (OP_TYPE op) {
	int ret;
	switch (op) {
		case T_PLUS:
			ret = 1;
			break;
		case T_MINUS:
			ret = 1;
			break;
		case T_MULT:
			ret = 1;
			break;
		case T_DIV:
			ret = 4;
			break;
		case T_OR:
			ret = 1;
			break;
		case T_AND:
			ret = 1;
			break;
		default:
			fprintf (stderr, "Operator not supported\n");
			exit (1);
	}
	return ret;
}

inline std::string print_resource (RESOURCE res) {
	std::string out ("");
	switch (res) {
		case GLOBAL_MEM:
			out = "global memory";
			break;
		case SHARED_MEM:
			out = "shared memory";
			break;
		case REGISTER:
			out = "register";
			break;
		default:
			fprintf (stderr, "No such resource (print_resource)\n");
			exit(1);
	}
	return out;
}

inline std::string trim_string (std::string s, std::string trim_char) {
	return s.substr (0, s.find (trim_char));
}

inline std::string print_trimmed_string (std::string s, char trim) {
	s.erase(std::remove(s.begin(), s.end(), trim), s.end());	
	return s;
}  

//Create ordered permutations of udecls
inline std::vector<std::map<std::string,int>> create_permutations (std::map<std::string,int> udecls, std::vector<std::string> iterators) {
	// Create all possible unrolling scenarios
	std::vector<std::map<std::string,int>> unroll_perm;
	int num_perms = 1;
	for (std::vector<std::string>::iterator it=iterators.begin(); it!=iterators.end(); it++) {
		int uf = (udecls.find (*it) != udecls.end ()) ? udecls[*it] : 1;
		num_perms *= uf;
	}
	// Initialize all instances 
	for (int i=0; i<num_perms; i++) {
		std::map<std::string,int> val;
		for (std::vector<std::string>::iterator it=iterators.begin(); it!=iterators.end(); it++) {
			val[*it] = 0;	
		}
		unroll_perm.push_back (val);
	}
	// Now create the specific instances
	int replicate_factor = 1;
	for (std::vector<std::string>::reverse_iterator it=iterators.rbegin(); it!=iterators.rend(); it++) {
		int uf = (udecls.find (*it) != udecls.end ()) ?  udecls[*it] : 1;
		int uf_instance = uf-1;
		int replicated = 1;	
		for (auto &up : unroll_perm) {
			up[*it] = uf_instance;
			if (replicated == replicate_factor) {
				uf_instance = (uf_instance == 0) ? uf-1 : uf_instance-1;
				replicated = 1;
			}
			else
				replicated++;
		}
		replicate_factor *= uf;
	}
	return unroll_perm;
}

// Return true if a is lexicographically lower than b. Lexical order is defined by iterators
inline bool lexicographically_low (std::map<std::string,int> a, std::map<std::string,int> b, std::vector<std::string> iterators) {
	bool result = false;
	for (auto iter : iterators) {
		assert (a.find (iter) != a.end ());
		assert (b.find (iter) != b.end ());
		int a_val = a.find (iter) != a.end () ? a[iter] : 0;
		int b_val = b.find (iter) != b.end () ? b[iter] : 0;
		result |= (a_val < b_val); 			
	}
	return result;
}

#endif
