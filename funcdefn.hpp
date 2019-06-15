#ifndef __STENCILDEFN_HPP__
#define __STENCILDEFN_HPP__
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <cstdlib>
#include <climits>
#include <cstdio>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <sstream>
#include <fstream>
#include <cassert>
#include <tuple>
#include <algorithm>
#include <cmath>
#include "exprnode.hpp"

/* A class representing a statement of the form a = b + c;
   The lhs is a string, and rhs is an expression node. */
class stmtNode {
	private:
		exprNode *lhs_node, *rhs_node;
		STMT_OP op_type;
		int stmt_num;
		std::vector<Range*> stmt_domain;
	public:
		stmtNode (STMT_OP, exprNode *, exprNode *);
		stmtNode (STMT_OP, exprNode *, exprNode *, int);
		stmtNode (STMT_OP, exprNode *, exprNode *, std::vector<Range*>);
		stmtNode (STMT_OP, exprNode *, exprNode *, int, std::vector<Range*>);
		STMT_OP get_op_type (void);
		void set_op_type (STMT_OP);
		exprNode *get_lhs_expr (void);
		exprNode *get_rhs_expr (void);
		void set_lhs_expr (exprNode *);
		void set_rhs_expr (exprNode *);
		int get_stmt_num (void);
		void set_stmt_num (int);
		void set_stmt_domain (std::vector<Range*>);
		std::vector<Range*> get_stmt_domain (void);
		DATA_TYPE infer_stmt_type (void);
		void decompose_statements (std::vector<stmtNode*> &, bool, std::map<std::string, std::set<int>> &, std::vector<std::tuple<std::string,std::string>> &, std::vector<stmtNode*> &);
		void fold_symmetric_computations (std::vector<stmtNode*> &, std::map<std::string, std::set<int>> &, std::vector<std::tuple<std::string,std::string>> &, std::vector<stmtNode*> &);
		void accessed_arrays (std::set<std::string> &);
		void retime_statement (std::string, int);
		bool is_scalar_stmt (void);
};

inline stmtNode::stmtNode (STMT_OP op, exprNode *lhs, exprNode *rhs) {
	op_type = op;
	lhs_node = lhs;
	rhs_node = rhs;
}

inline stmtNode::stmtNode (STMT_OP op, exprNode *lhs, exprNode *rhs, int num) {
	op_type = op;
	lhs_node = lhs;
	rhs_node = rhs;
	stmt_num = num;
}

inline stmtNode::stmtNode (STMT_OP op, exprNode *lhs, exprNode *rhs, std::vector<Range*> domain) {
	op_type = op;
	lhs_node = lhs;
	rhs_node = rhs;
	stmt_domain = domain;
}

inline stmtNode::stmtNode (STMT_OP op, exprNode *lhs, exprNode *rhs, int num, std::vector<Range*> domain) {
	op_type = op;
	lhs_node = lhs;
	rhs_node = rhs;
	stmt_num = num;
	stmt_domain = domain;
}

inline std::vector<Range*> stmtNode::get_stmt_domain (void) {
	return stmt_domain;
}

inline void stmtNode::set_stmt_domain (std::vector<Range*> domain) {
	stmt_domain = domain;
}

inline STMT_OP stmtNode::get_op_type (void) {
	return op_type;
}

inline void stmtNode::set_op_type (STMT_OP op) {
	op_type = op;
}

inline exprNode* stmtNode::get_lhs_expr (void) {
	return lhs_node;
}

inline exprNode* stmtNode::get_rhs_expr (void) {
	return rhs_node;
}

inline void stmtNode::set_lhs_expr (exprNode *lhs) {
	lhs_node = lhs;
}

inline void stmtNode::set_rhs_expr (exprNode *rhs) {
	rhs_node = rhs;
}

inline int stmtNode::get_stmt_num (void) {
	return stmt_num;
}

inline void stmtNode::set_stmt_num (int val) {
	stmt_num = val;
}

class stmtList {
	private:
		std::vector<stmtNode*> stmt_list;
	public:
		void push_stmt (stmtNode *);
		void push_stmt (std::vector<stmtNode *>);
		void insert_stmt (stmtNode *, int pos=0);
		void insert_stmt (std::vector<stmtNode *>, int pos=0);
		std::vector<stmtNode*> get_stmt_list (void);
		void set_stmt_list (std::vector<stmtNode*>);
};

inline void stmtList::push_stmt (stmtNode *stmt) {
	int size = stmt_list.size ();
	stmt->set_stmt_num (size);
	stmt_list.push_back (stmt);
}

inline void stmtList::push_stmt (std::vector<stmtNode *> stmt_vec) {
	for (auto stmt : stmt_vec) { 
		int size = stmt_list.size ();
		stmt->set_stmt_num (size);
		stmt_list.push_back (stmt);
	}
}

inline void stmtList::insert_stmt (stmtNode *stmt, int pos) {
	stmt_list.insert (stmt_list.begin()+pos, stmt);
	int index = 0;
	for (auto s : stmt_list) {
		s->set_stmt_num (index);
		index++;
	}
}

inline void stmtList::insert_stmt (std::vector<stmtNode *> stmt_vec, int pos) {
	std::cout << "Before insert, stmt list size = " << stmt_list.size() << "\n";
	stmt_list.insert (stmt_list.begin()+pos, stmt_vec.begin(), stmt_vec.end());
	int index = 0;
	for (auto s : stmt_list) {
		s->set_stmt_num (index);
		index++;
	}
	std::cout << "After insert, stmt list size = " << stmt_list.size() << "\n";
}

inline std::vector<stmtNode*> stmtList::get_stmt_list (void) {
	return stmt_list;
}

inline void stmtList::set_stmt_list (std::vector<stmtNode*> sl) {
	stmt_list = sl;
}

class stencilDefn {
	private:
		int dim=0, total_stmts=0;
		int shmem_size=48, stream_uf = 4;
		bool full_stream=false, stream=false, use_shmem=false, blocked_loads=false, generate_if=false, prefetch=false, retiming_feasible = false, linearize_accesses = false, code_diff = false;
		std::string stream_dim;
		std::string indent = "\t", dindent = "\t\t";
		std::vector<std::string> parameters, iterators, allocins, copyins, copyouts, constants, coefficients;
		std::map<std::string,std::tuple<int,int>> halo_dims;
		symtabTemplate <DATA_TYPE> *var_decls;
		std::vector<arrayDecl*> array_decls;
		std::vector<std::tuple<DATA_TYPE, std::string, exprNode*>> var_init_decls;
		symtabTemplate <int> *unroll_decls;
		symtabTemplate <int> *block_dims;
		argList *arg_list;
		std::vector<std::string> shmem_decl, noshmem_decl, gmem_decl;
		stmtList *stmt_list;
		std::map<int, std::vector<stmtNode*>> decomposed_stmts;
		std::map<int, std::vector<int>> raw_dependence_graph;
		std::map<int, std::vector<int>> war_dependence_graph;
		std::map<int, std::vector<int>> waw_dependence_graph;
		std::map<int, std::vector<int>> accumulation_dependence_graph;
		std::map<int,int> plane_offset;
		// The first field is the array name, the second is the offset in the  
		// streaming dimension, the third indicates if the streaming dimension 
		// actually exists in the access (because the offset will be 0 by default)
		std::map<int, std::map<std::tuple<std::string,int,bool>,RESOURCE>> stmt_resource_map;
		std::map<std::tuple<std::string,int,bool>,RESOURCE> resource_map;
		std::map<std::tuple<std::string,int,bool>, std::tuple<int,int,std::map<int,std::vector<std::string>>>> access_stats;
		std::map<int, std::tuple<std::vector<Range*>,std::vector<Range*>>> stmt_hull; 
		std::map<int, std::tuple<std::vector<Range*>,std::vector<Range*>>> resource_hull; 
		std::vector<Range*> initial_hull, recompute_hull, output_hull;
	public:
		stencilDefn (stmtList *, argList *);
		void push_arg (char *);
		void set_iterators (std::vector<std::string>);
		void set_parameters (std::vector<std::string>);
		void set_constants (std::vector<std::string>);
		void set_allocins (std::vector<std::string>);
		void set_copyins (std::vector<std::string>);
		void set_copyouts (std::vector<std::string>);
		std::vector<std::string> get_copyouts (void);
		void set_coefficients (std::vector<std::string>);
		void set_var_decls (symtabTemplate <DATA_TYPE> *);
		void set_streaming_dimension (bool, std::string);
		void set_use_shmem (bool);
		void set_linearize_accesses (bool);
	  void set_gen_code_diff (bool);
		void set_array_decls (std::vector<arrayDecl*>);
		void set_unroll_decls (symtabTemplate <int> *);
		void set_block_dims (symtabTemplate <int> *);
		void set_data_type (void);
		void set_load_style (bool);
		void set_loop_gen_strategy (bool);
		void set_loop_prefetch_strategy (bool);
		void set_halo_dims (std::map<std::string,std::tuple<int,int>>);
		void reset_halo_dims (void);
		std::map<std::string,std::tuple<int,int>> get_halo_dims (void);
		std::map<std::string,int> get_unroll_decls (void);
		std::map<std::string,int> get_block_dims (void);
		void set_dim (int);
		int get_dim (void);
		bool perform_streaming (void);
		std::string get_stream_dim (void);
		std::vector<std::string> get_iterators (void);
		std::vector<std::string> get_arg_list (void);
		std::vector<std::string> get_inout_list (void);
		std::vector<stmtNode*> get_stmt_list (void);
		void push_var_decl (std::string, DATA_TYPE);
		void push_var_init_decl (std::tuple <DATA_TYPE, std::string, exprNode*>);
		void push_shmem_decl (std::vector<std::string>);
		void push_noshmem_decl (std::vector<std::string>);
		void push_gmem_decl (std::vector<std::string>);
		std::vector<std::string> get_shmem_decl (void);
		std::vector<std::string> get_noshmem_decl (void);
		std::vector<std::string> get_gmem_decl (void);
		void push_array_decl (arrayDecl *);
		std::map<std::string,DATA_TYPE> get_var_decls (void);
		std::vector<arrayDecl*> get_array_decls (void);
		void verify_immutable_indices (void);
		void offset_dependent_planes (void);
		void compute_dependences (void);
		void decompose_statements (bool, bool, bool);
		void unify_dependences (std::map<int,std::vector<int>> &);
		void compute_unified_dependences (void);
		void compute_pointwise_occurrences (std::vector<int> &);
		void print_dependences (std::string);
		bool raw_dependence_exists (int, int);
		bool war_dependence_exists (int, int);
		bool waw_dependence_exists (int, int);
		bool accumulation_dependence_exists (int, int);
		void compute_dimensionality (void);
		void determine_stmt_resource_mapping (void);
		void print_stmt_resource_map (std::string);
		void amend_resource_map (void);
		void print_stencil_defn (std::string);
		void print_stencil_header (std::string, std::stringstream &);
		void print_mapped_stencil_stmts (std::stringstream &, std::vector<std::string>);
		void print_declarations (std::string, std::stringstream &);
		void print_reg_initializations (std::stringstream &, bool);
		bool print_shm_initializations (std::stringstream &);
		void print_prefetch_loads_without_halo (std::stringstream &);
		void print_prefetch_loads_with_halo (std::stringstream &);
		void print_loading_planes_without_halo (std::stringstream &, std::map<std::tuple<std::string,bool>, std::tuple<int,int>>, std::string);
		void print_loading_planes_with_halo (std::stringstream &, std::map<std::tuple<std::string,bool>, std::tuple<int,int>>, std::string);
		std::string print_loading_planes_with_nonseparable_halo (std::stringstream &, std::map<std::tuple<std::string,bool>, std::tuple<int,int>>, std::string);
		void print_prefetch_loads (void);
		void print_rotating_planes (std::stringstream &, std::map<std::tuple<std::string,bool>, std::tuple<int,int>>);
		void print_rotating_planes_without_halo (std::stringstream &, std::map<std::tuple<std::string,bool>, std::tuple<int,int>>);
		void print_rotating_planes_with_halo (std::stringstream &, std::map<std::tuple<std::string,bool>, std::tuple<int,int>>);
		void print_rotating_planes_with_nonseparable_halo (std::stringstream &, std::map<std::tuple<std::string,bool>, std::tuple<int,int>>);
		void print_rotating_planes (std::stringstream &);
		void compute_hull (std::vector<Range*> &, std::vector<int>);
		void compute_resource_hull (std::vector<Range*> &, std::vector<int>);
		bool is_array_decl (std::string);
		bool is_temp_array (std::string);
		bool is_copyout_array (std::string);
		bool is_inout_array (std::string);
		bool is_stencil_arg (std::string);
		std::vector<Range*> get_array_range (std::string);
		DATA_TYPE get_array_type (std::string);
		bool is_var_decl (std::string);
		DATA_TYPE get_var_type (std::string);
		void set_initial_hull (std::vector<Range*> &);
		void unroll_stmts (void);
		std::vector<Range*> get_recompute_hull (void);
		std::vector<Range*> get_starting_hull (void);
		bool halo_exists (void);
		void identify_separable_clusters (std::string, std::map<int,std::vector<int>> &);
		void print_stencil_body (std::string, std::map<int,std::vector<int>> &, std::stringstream &);
		std::vector<stmtNode*> reconstruct_retimed_rhs (std::vector<stmtNode*>, stmtNode*);
};

inline stencilDefn::stencilDefn (stmtList *stmts, argList *args) {
	stmt_list = stmts;
	arg_list = args;
	var_decls = new symtabTemplate <DATA_TYPE>;
	shmem_size *= 1024;
}

inline void stencilDefn::push_arg (char *str) {
	arg_list->push_back (std::string(str));
}

inline void stencilDefn::set_iterators (std::vector<std::string> s) {
	iterators = s;
}

inline std::vector<std::string> stencilDefn::get_iterators (void) {
	return iterators;
}

inline void stencilDefn::set_parameters (std::vector<std::string> s) {
	parameters = s;
}

inline std::map<std::string,std::tuple<int,int>> stencilDefn::get_halo_dims (void) {
	return halo_dims;
}

inline void stencilDefn::set_initial_hull (std::vector<Range*> &d) {
	initial_hull = d;
}

inline void stencilDefn::set_streaming_dimension (bool fs, std::string s) {
	stream_dim = s;
	full_stream = fs;
	stream = true;
}

inline bool stencilDefn::perform_streaming (void) {
	return stream;
}

inline std::string stencilDefn::get_stream_dim (void) {
	return stream_dim;
}

inline void stencilDefn::set_use_shmem (bool shm) {
	use_shmem = shm;
}

inline void stencilDefn::set_linearize_accesses (bool linearize) {
	linearize_accesses = linearize;
}

inline void stencilDefn::set_gen_code_diff (bool diff) {
	code_diff = diff;
}

inline std::vector<std::string> stencilDefn::get_copyouts (void) {
	return copyouts;
} 

inline void stencilDefn::set_coefficients (std::vector<std::string> s) {
	coefficients = s;
}

inline void stencilDefn::set_var_decls (symtabTemplate <DATA_TYPE> *v) {
	var_decls = v;
}

inline void stencilDefn::set_allocins (std::vector<std::string> s) {
	allocins = s;
}

inline void stencilDefn::set_copyins (std::vector<std::string> s) {
	copyins = s;
}

inline void stencilDefn::set_constants (std::vector<std::string> s) {
	constants = s;
}

inline void stencilDefn::set_copyouts (std::vector<std::string> s) {
	copyouts = s;
}

inline bool stencilDefn::is_copyout_array (std::string s) {
	return (find (copyouts.begin(), copyouts.end(), s) != copyouts.end ());
}

inline bool stencilDefn::is_inout_array (std::string s) {
	std::vector<std::string> inouts = get_inout_list ();
	return (find (inouts.begin(), inouts.end(), s) != inouts.end ());
}

inline std::vector<Range*> stencilDefn::get_recompute_hull (void) {
	return recompute_hull;
}

inline void stencilDefn::push_var_decl (std::string s, DATA_TYPE t) {
	var_decls->push_symbol (s, t);
}

inline void stencilDefn::push_var_init_decl (std::tuple<DATA_TYPE, std::string, exprNode*> v) {
	var_init_decls.push_back (v);
}


inline std::map<std::string,DATA_TYPE> stencilDefn:: get_var_decls (void) {
	return var_decls->get_symbol_map ();
}

inline void stencilDefn::set_array_decls (std::vector<arrayDecl*> a) {
	array_decls = a;
}

inline void stencilDefn::push_shmem_decl (std::vector<std::string> s) {
	shmem_decl = s;
}

inline void stencilDefn::push_noshmem_decl (std::vector<std::string> s) {
	noshmem_decl = s;
}

inline void stencilDefn::push_gmem_decl (std::vector<std::string> s) {
	gmem_decl = s;
}

inline std::vector<std::string> stencilDefn::get_shmem_decl (void) {
	return shmem_decl;
}

inline std::vector<std::string> stencilDefn::get_noshmem_decl (void) {
	return noshmem_decl;
}

inline std::vector<std::string> stencilDefn::get_gmem_decl (void) {
	return gmem_decl;
}

inline void stencilDefn::push_array_decl (arrayDecl *a) {
	array_decls.push_back (a);
}

inline std::vector<arrayDecl*> stencilDefn:: get_array_decls (void) {
	return array_decls;
}

inline std::map<std::string,int> stencilDefn::get_unroll_decls (void) {
	return unroll_decls->get_symbol_map ();
}

inline void stencilDefn::set_block_dims (symtabTemplate <int> * b) {
	block_dims = b;
}

inline std::map<std::string,int> stencilDefn::get_block_dims (void) {
	return block_dims->get_symbol_map ();
}

inline void stencilDefn::set_dim (int d) {
	dim = d;
}

inline int stencilDefn::get_dim (void) {
	return dim;
}

inline std::vector<std::string> stencilDefn::get_arg_list (void) {
	return arg_list->get_list (); 
}

inline std::vector<std::string> stencilDefn::get_inout_list (void) {
	return arg_list->get_inouts (); 
}

inline std::vector<stmtNode*> stencilDefn::get_stmt_list (void) {
	return stmt_list->get_stmt_list ();
}


class funcDefn {
	private:
		bool full_stream=false, stream=false;
		std::string stream_dim;
		symtabTemplate<stencilDefn*> *stencil_defns;
		std::vector<std::string> parameters, iterators, shmem_decl, noshmem_decl, gmem_decl;
		symtabTemplate <DATA_TYPE> *var_decls, *temp_var_decls;
		std::vector<std::tuple<DATA_TYPE, std::string, exprNode*>> temp_var_init_decls;
		std::vector<arrayDecl*> array_decls, temp_array_decls;
		std::vector<std::string> sc_iterators;
		std::vector<std::string> coefficients;
		std::vector<stencilCall*> stencil_calls;
		std::vector<std::string> allocins, copyins, constants, copyouts;
		symtabTemplate <int> *unroll_decls;
		symtabTemplate <int> *block_dims;
		std::string indent = "\t";
	public:
		funcDefn ();
		void push_stencil_defn (char *, stencilDefn *);
		void push_parameter (std::vector<std::string>);
		void push_parameter (char *);
		void push_var_decl (std::vector<std::string>, DATA_TYPE);
		void push_var_decl (char *, DATA_TYPE);
		void push_temp_var_decl (std::vector<std::string>, DATA_TYPE);
		void push_temp_var_decl (char *, DATA_TYPE);
		void push_temp_var_init_decl (DATA_TYPE, char *, exprNode *);
		void push_array_decl (std::vector<arrayDecl*>&, DATA_TYPE);
		void push_array_decl (arrayDecl *);
		void push_temp_array_decl (std::vector<arrayDecl*>&, DATA_TYPE);
		void push_temp_array_decl (arrayDecl *);
		bool is_array_decl (std::string);
		DATA_TYPE get_array_type (std::string);
		std::vector<Range*> get_array_range (std::string);
		void push_stencil_call (stencilCall *);
		void push_iterator (std::vector<std::string>);
		void push_iterator (char *);
		void set_streaming_dimension (bool, std::string);
		std::vector<std::string> get_coefficients (void);
		void push_coefficient (std::vector<std::string>);
		void push_coefficient (char *);
		void push_allocin (std::vector<std::string>);
		void push_copyin (std::vector<std::string>);
		void push_constant (std::vector<std::string>);
		void push_copyout (std::vector<std::string>);
		void set_data_type (void);
		void push_shmem_decl (std::vector<std::string>);
		void push_noshmem_decl (std::vector<std::string>);
		void push_gmem_decl (std::vector<std::string>);
		void set_unroll_decls (symtabTemplate <int> *);
		std::map<std::string,int> get_unroll_decls (void);
		void set_block_dims (symtabTemplate <int> *);
		std::map<std::string,int> get_block_dims (void);
		std::vector<std::string> get_parameters (void);
		std::vector<std::string> get_iterators (void);
		std::map<std::string,DATA_TYPE> get_var_decls (void);
		std::map<std::string,DATA_TYPE> get_temp_var_decls (void);
		std::vector<arrayDecl*> get_array_decls (void);
		arrayDecl* get_array_decl (std::string);
		std::vector<arrayDecl*> get_temp_array_decls (void);
		std::map<std::string,stencilDefn*> get_stencil_defns (void);
		stencilDefn *get_stencil_defn (std::string);
		std::vector<stencilCall*> get_stencil_calls (void);
		std::vector<std::string> get_allocins (void);
		std::vector<std::string> get_copyins (void);
		std::vector<std::string> get_constants (void);
		void print_constant_memory (std::stringstream &);
		std::vector<std::string> get_copyouts (void);
		void verify_correctness (std::map<std::string,int>&);
		void map_args_to_parameters (void);
		void map_args_to_parameters (std::map<std::tuple<std::string,std::string>,std::string> &);
		void get_arg_info (std::string, DECL_TYPE &, DATA_TYPE &);
		void print_host_code (std::stringstream &);
		void print_device_code (std::stringstream &);
		void determine_stmt_resource_mapping (void);
		void compute_hull (void);
		void create_unique_stencil_defns (void);
		void print_stencil_calls (std::map<std::string, std::map<int,std::vector<int>>> &, std::stringstream &);
};

inline funcDefn::funcDefn () {
	stencil_defns = new symtabTemplate <stencilDefn*>;
	var_decls = new symtabTemplate <DATA_TYPE>;
	temp_var_decls = new symtabTemplate <DATA_TYPE>;
}

inline void funcDefn::push_stencil_defn (char *s, stencilDefn *def) {
	// Copy all the temporary arrays and declarations to stencildef
	def->push_shmem_decl (shmem_decl);
	def->push_noshmem_decl (noshmem_decl);
	def->push_gmem_decl (gmem_decl);
	std::map<std::string, DATA_TYPE> tdecl = temp_var_decls->get_symbol_map ();   
	for (auto td : tdecl) {
		def->push_var_decl (td.first, td.second);
	}
	for (auto a : temp_array_decls) {
		def->push_array_decl (a);
	}
	for (auto tid : temp_var_init_decls) {
		def->push_var_init_decl (tid);
	}
	temp_var_decls->delete_symbols ();
	temp_array_decls.clear ();
	temp_var_init_decls.clear ();
	stencil_defns->push_symbol (s, def);
}

inline void funcDefn::push_array_decl (std::vector<arrayDecl*> &list, DATA_TYPE t) {
	for (auto l : list) {
		l->set_array_type (t);
		array_decls.push_back (l);
	}
}

inline void funcDefn::set_streaming_dimension (bool fs, std::string s) {
	stream_dim = s;
	full_stream = fs;
	stream = true;
}

inline void funcDefn::push_array_decl (arrayDecl *a) {
	array_decls.push_back (a);
}

inline void funcDefn::push_temp_array_decl (std::vector<arrayDecl*> &list, DATA_TYPE t) {
	for (auto l : list) {
		l->set_array_type (t);
		l->set_temp_array (true);
		temp_array_decls.push_back (l);
	}
}

inline void funcDefn::push_temp_array_decl (arrayDecl *a) {
	a->set_temp_array (true);	
	temp_array_decls.push_back (a);
}

inline void funcDefn::push_parameter (char *s) {
	parameters.push_back (std::string(s));
}

inline void funcDefn::push_parameter (std::vector<std::string> s) {
	parameters = s;
}

inline void funcDefn::push_var_decl (std::vector<std::string> list, DATA_TYPE t) {
	for (auto l : list) { 
		var_decls->push_symbol (l, t);
	}
}

inline void funcDefn::push_var_decl (char *s, DATA_TYPE t) {
		var_decls->push_symbol (s, t);
}

inline void funcDefn::push_temp_var_decl (std::vector<std::string> list, DATA_TYPE t) {
	for (auto l : list) { 
		temp_var_decls->push_symbol (l, t);
	}
}

inline void funcDefn::push_temp_var_decl (char *s, DATA_TYPE t) {
	temp_var_decls->push_symbol (s, t);
}

inline void funcDefn::push_temp_var_init_decl (DATA_TYPE t, char *lhs, exprNode *rhs) {
	temp_var_init_decls.push_back (make_tuple (t, std::string(lhs), rhs));
}

inline void funcDefn::push_stencil_call (stencilCall *st_call) {
	stencil_calls.push_back (st_call);
}

inline void funcDefn::push_iterator (std::vector<std::string> s) {
	iterators = s;
}

inline void funcDefn::push_iterator (char *s) {
	iterators.push_back (std::string (s));
}

inline void funcDefn::push_allocin (std::vector<std::string> s) {
	allocins = s;
}

inline void funcDefn::push_copyin (std::vector<std::string> s) {
	copyins = s;
}

inline void funcDefn::push_constant (std::vector<std::string> s) {
	constants = s;
}

inline void funcDefn::push_copyout (std::vector<std::string> s) {
	copyouts = s;
}

inline void funcDefn::push_shmem_decl (std::vector<std::string> s) {
	shmem_decl = s;
}

inline void funcDefn::push_noshmem_decl (std::vector<std::string> s) {
	noshmem_decl = s;
}

inline void funcDefn::push_gmem_decl (std::vector<std::string> s) {
	gmem_decl = s;
}

inline void funcDefn::push_coefficient (std::vector<std::string> s) {
	coefficients = s;
}

inline void funcDefn::push_coefficient (char *s) {
	coefficients.push_back (std::string (s));
}

inline void funcDefn::set_unroll_decls (symtabTemplate <int> * u) {
	unroll_decls = u;
	//Fill in the missing values
	for (auto iter: get_iterators ()) {
		if (!(unroll_decls->symbol_present (iter))) {
			unroll_decls->push_symbol (iter, 1);
		}
	}
}

inline std::map<std::string,int> funcDefn::get_unroll_decls (void) {
	return unroll_decls->get_symbol_map ();
}

inline void funcDefn::set_block_dims (symtabTemplate <int> * b) {
	block_dims = b;
}

inline std::map<std::string,int> funcDefn::get_block_dims (void) {
	return block_dims->get_symbol_map ();
}

inline std::vector<std::string> funcDefn::get_parameters (void) {
	return parameters;
}

inline std::vector<std::string> funcDefn::get_iterators (void) {
	return iterators;
}

inline std::vector<std::string> funcDefn::get_coefficients (void) {
	return coefficients;
}

inline std::map<std::string,DATA_TYPE> funcDefn:: get_var_decls (void) {
	return var_decls->get_symbol_map ();
}

inline std::map<std::string,DATA_TYPE> funcDefn:: get_temp_var_decls (void) {
	return temp_var_decls->get_symbol_map ();
}

inline std::vector<arrayDecl*> funcDefn:: get_array_decls (void) {
	return array_decls;
}

inline std::vector<arrayDecl*> funcDefn:: get_temp_array_decls (void) {
	return temp_array_decls;
}

inline std::map<std::string,stencilDefn*> funcDefn::get_stencil_defns (void) {
	return stencil_defns->get_symbol_map ();
}

inline std::vector<stencilCall*> funcDefn::get_stencil_calls (void) {
	return stencil_calls;
}

inline std::vector<std::string> funcDefn::get_allocins (void) {
	return allocins;
}

inline std::vector<std::string> funcDefn::get_copyins (void) {
	return copyins;
}

inline std::vector<std::string> funcDefn::get_constants (void) {
	return constants;
}

inline std::vector<std::string> funcDefn::get_copyouts (void) {
	return copyouts;
} 


// The class that starts it all. This contains a list of all the
// function definitions.
class startNode {
	private:
		std::vector<funcDefn*> func_defns;
		Range *stencil_call_iterations;
		int func_defn_count;
	public:
		startNode ();
		void set_stencil_call_iterations (Range *);
		void reset_stencil_call_iterations (void);
		void increment_func_defn_count (void);
		void push_parameter (std::vector<std::string> *);
		void push_parameter (char *);
		void push_var_decl (std::vector<std::string> *, DATA_TYPE);
		void push_var_decl (char *, DATA_TYPE);
		void push_temp_var_decl (std::vector<std::string> *, DATA_TYPE);
		void push_temp_var_decl (char *, DATA_TYPE);
		void push_temp_var_init_decl (DATA_TYPE, char *, exprNode*);
		void push_array_decl (std::vector<arrayDecl*> *, DATA_TYPE);
		void push_array_decl (arrayDecl *);
		void push_temp_array_decl (std::vector<arrayDecl*> *, DATA_TYPE);
		void push_temp_array_decl (arrayDecl *);
		void push_stencil_call (stencilCall *);
		void push_iterator (std::vector<std::string> *);
		void push_iterator (char *);
		void push_coefficient (std::vector<std::string> *);
		void push_coefficient (char *);
		void push_allocin (std::vector<std::string> *);
		void push_copyin (std::vector<std::string> *);
		void push_constant (std::vector<std::string> *);
		void push_copyout (std::vector<std::string> *);
		void push_stencil_defn (char *, stencilDefn *);
		void create_func_defn (void);
		std::vector<funcDefn*> get_func_defns (void);
		void verify_correctness (void);
		void map_args_to_parameters (void);
		void generate_code (std::stringstream &);
		void set_streaming_dimension (bool, std::string);
		void set_use_shmem (bool);
		void set_linearize_accesses (bool);
	  void set_gen_code_diff (bool);
		void set_iterators (void);
		void set_parameters (void);
		void set_constants (void);
		void set_allocins (void);
		void set_copyins (void);
		void set_copyouts (void);
		void set_unroll_decls (symtabTemplate <int> *);
		void push_shmem_decl (std::vector<std::string> *);
		void push_noshmem_decl (std::vector<std::string> *);
		void push_gmem_decl (std::vector<std::string> *);
		void set_block_dims (symtabTemplate <int> *);
		void set_load_style (bool);
		void set_loop_gen_strategy (bool);
		void set_loop_prefetch_strategy (bool);
		void set_halo_dims (std::map<std::string,std::tuple<int,int>>);
		void compute_dependences (void);
		void decompose_statements (bool, bool, bool);
		void create_trivial_stencil_versions (void);
};

inline startNode::startNode () {
	func_defn_count = 0;
	reset_stencil_call_iterations ();
}

inline void startNode::create_func_defn (void) {
	func_defns.push_back (new funcDefn ());	
}

inline std::vector<funcDefn*> startNode::get_func_defns (void) {
	return func_defns;
}

inline void startNode::increment_func_defn_count (void) {
	func_defn_count++;
}

inline void startNode::push_parameter (std::vector<std::string> *s) {
	func_defns[func_defn_count]->push_parameter (*s);
}

inline void startNode::push_parameter (char *s) {
	func_defns[func_defn_count]->push_parameter (s);
}

inline void startNode::push_var_decl (std::vector<std::string> *s, DATA_TYPE t) {
	func_defns[func_defn_count]->push_var_decl (*s, t);
}

inline void startNode::push_var_decl (char *s, DATA_TYPE t) {
	func_defns[func_defn_count]->push_var_decl (s, t);
}

inline void startNode::push_array_decl (std::vector<arrayDecl*> *list, DATA_TYPE t) {
	func_defns[func_defn_count]->push_array_decl (*list, t);
}

inline void startNode::push_array_decl (arrayDecl *a) {
	func_defns[func_defn_count]->push_array_decl (a);
}

inline void startNode::push_temp_var_decl (std::vector<std::string> *s, DATA_TYPE t) {
	func_defns[func_defn_count]->push_temp_var_decl (*s, t);
}

inline void startNode::push_temp_var_decl (char *s, DATA_TYPE t) {
	func_defns[func_defn_count]->push_temp_var_decl (s, t);
}

inline void startNode::push_temp_var_init_decl (DATA_TYPE t, char *lhs, exprNode *rhs) {
	func_defns[func_defn_count]->push_temp_var_init_decl (t, lhs, rhs);
}

inline void startNode::push_temp_array_decl (std::vector<arrayDecl*> *list, DATA_TYPE t) {
	func_defns[func_defn_count]->push_temp_array_decl (*list, t);
}

inline void startNode::push_temp_array_decl (arrayDecl *a) {
	func_defns[func_defn_count]->push_temp_array_decl (a);
}

inline void startNode::set_stencil_call_iterations (Range *a) {
	stencil_call_iterations = a;
}

inline void startNode::reset_stencil_call_iterations (void) {
	exprNode *a = new datatypeNode<int> (0, INT);
	stencil_call_iterations = new Range (a, a);
}

inline void startNode::push_stencil_call (stencilCall *st_call) {
	st_call->set_iterations (stencil_call_iterations);
	func_defns[func_defn_count]->push_stencil_call (st_call);
}

inline void startNode::push_iterator (std::vector<std::string> *s) {
	func_defns[func_defn_count]->push_iterator (*s);
}

inline void startNode::push_iterator (char *s) {
	func_defns[func_defn_count]->push_iterator (s);
}

inline void startNode::push_coefficient (std::vector<std::string> *s) {
	func_defns[func_defn_count]->push_coefficient (*s);
}

inline void startNode::push_coefficient (char *s) {
	func_defns[func_defn_count]->push_coefficient (s);
}

inline void startNode::push_allocin (std::vector<std::string> *s) {
	func_defns[func_defn_count]->push_allocin (*s);
}

inline void startNode::push_copyin (std::vector<std::string> *s) {
	func_defns[func_defn_count]->push_copyin (*s);
}

inline void startNode::push_constant (std::vector<std::string> *s) {
	func_defns[func_defn_count]->push_constant (*s);
}

inline void startNode::push_copyout (std::vector<std::string> *s) {
	func_defns[func_defn_count]->push_copyout (*s);
}

inline void startNode::push_shmem_decl (std::vector<std::string> *s) {
	func_defns[func_defn_count]->push_shmem_decl (*s);
}

inline void startNode::push_noshmem_decl (std::vector<std::string> *s) {
	func_defns[func_defn_count]->push_noshmem_decl (*s);
}

inline void startNode::push_gmem_decl (std::vector<std::string> *s) {
	func_defns[func_defn_count]->push_gmem_decl (*s);
}

inline void startNode::push_stencil_defn (char *s, stencilDefn *def) {
	func_defns[func_defn_count]->push_stencil_defn (s, def);
}

#endif
