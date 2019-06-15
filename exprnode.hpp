#ifndef __EXPRNODE_HPP__
#define __EXPRNODE_HPP__
#include <cstdio>
#include <cstdlib>
#include <string>
#include <climits>
#include <vector>
#include <tuple>
#include <map>
#include <set>
#include <iostream>
#include <cassert>
#include <sstream>
#include <algorithm>
#include "utils.hpp" 
#include "templates.hpp"
#include "arrdecl.hpp"

class exprNode {
	protected:
		ETYPE expr_type;
		DATA_TYPE type=BOOL;
		std::string name="";
		bool nested=false;
	public:
		ETYPE get_expr_type (void);
		void set_expr_type (ETYPE);
		virtual void set_type (DATA_TYPE) {}
		void set_name (std::string);
		void set_name (char *);
		std::string get_name (void);	
		DATA_TYPE get_type (void);
		void set_nested (bool val=true);
		bool is_nested (void);
		virtual bool same_expr (exprNode *) {return false;}
		virtual bool waw_dependence (exprNode *) {return false;}
		virtual bool war_dependence (exprNode *) {return false;}
		virtual bool raw_dependence (exprNode *) {return false;}
		virtual bool pointwise_occurrence (exprNode *) {return false;}
		virtual void compute_rbw (std::set<std::string> &, std::set<std::string> &, bool) {}
		virtual void compute_wbr (std::set<std::string> &, std::set<std::string> &, bool) {}
		virtual void compute_last_writes (std::map<std::string,exprNode*> &, bool, std::string) {}
		virtual bool is_last_write (std::map<std::string,exprNode*> &) {return false;}
		virtual void print_node (std::stringstream &) {}
		virtual void decompose_access_fsm (std::string &, int &) {}
		virtual void accessed_arrays (std::set<std::string> &) {};
		virtual void determine_stmt_resource_mapping (std::map<std::tuple<std::string,int,bool>,RESOURCE> &, std::vector<std::string>, bool, std::string, bool) {}
		virtual int compute_dimensionality (std::vector<std::string>) {return 0;}
		virtual bool verify_immutable_expr (std::vector<std::string>, bool) {return false;}
		virtual int maximum_streaming_offset (exprNode*, std::string) {return 0;}
		virtual int streaming_offset (std::string) {return 0;}
		virtual bool same_subscripts_for_array (std::string, std::vector<std::string> &, DATA_TYPE &) {return true;}
		virtual void replace_array_name (std::string, std::string) {}
		virtual int minimum_streaming_offset (std::string) {return 0;}
		virtual void offset_expr (int, std::string) {}
		virtual void print_node (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, bool, bool, std::string, std::vector<std::string>, std::map<std::string,int>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::string &) {}
		virtual void print_node (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, bool, bool, std::string, std::vector<std::string>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::map<std::string,std::tuple<int,int>>, int, std::string &) {}
		virtual void print_last_write (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, std::map<std::string,exprNode*>, bool, bool, std::string, std::vector<std::string>, std::map<std::string,int>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::string &) {}
		virtual void print_last_write (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, std::map<std::string,exprNode*>, bool, bool, std::string, std::vector<std::string>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::map<std::string,std::tuple<int,int>>, int, std::string &) {}
		virtual void compute_hull (std::map<std::string, std::vector<Range*>> &, std::vector<Range*> &, std::vector<Range*> &, bool, std::string, int) {}
		virtual void compute_hull (std::map<std::string, std::vector<Range*>> &, std::vector<Range*> &, std::vector<Range*> &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, std::string, int) {}

		virtual void shift_hull (std::vector<Range*> &) {}
		virtual void shift_hull (std::vector<Range*> &, bool, std::string) {}
		virtual void collect_accesses (std::set<std::string> &) {}
		virtual void collect_access_stats (std::map<std::tuple<std::string,int,bool>, std::tuple<int,int,int>> &, bool, std::map<std::string,int>, std::vector<std::string>, bool, std::string) {}
		virtual void collect_access_stats (std::map<std::tuple<std::string,int,bool>, std::tuple<int,int,std::map<int,std::vector<std::string>>>> &, bool, std::vector<std::string>, bool, std::string) {}
		virtual void set_data_type (std::map<std::string, DATA_TYPE>) {}
		virtual exprNode *unroll_expr (std::string, int, std::map<std::string,int> &, bool) {return this;}
		virtual void infer_expr_type (DATA_TYPE &);
		virtual void decompose_node (exprNode*, std::vector<std::tuple<exprNode*,STMT_OP,exprNode*>> &, bool) {}
		virtual exprNode *distribute_node (void) {return this;}
		virtual void remove_redundant_nesting (void) {}
		virtual bool same_operator (OP_TYPE p) {return true;}
		virtual void nonassoc_chain (bool &) {}
		virtual void assoc_chain (bool &) {}
		virtual bool is_data_node (void) {return false;}
		virtual bool is_id_node (void) {return false;}
		virtual bool is_binary_node (void) {return false;}
		virtual bool is_shiftvec_node (void) {return false;}
		virtual int get_retiming_offset (std::string) {return INT_MIN;}
		virtual bool retiming_feasible (std::string, std::vector<int> &) {return true;}
		virtual exprNode *retime_node (std::string, int) {return this;}
		virtual int get_flop_count (void) {return 0;}
		virtual bool identify_same_accesses (std::map<exprNode*,std::vector<exprNode*>> &, exprNode*) {return false;}
		virtual bool consolidate_same_accesses (std::vector<std::tuple<OP_TYPE,exprNode*>> &, std::map<exprNode*,std::vector<exprNode*>> &, std::map<exprNode*,exprNode*> &, exprNode*, bool) {return false;}
		virtual bool is_scalar_expr (void) {return true;}
};

inline DATA_TYPE exprNode::get_type (void) {
	return type;
}

inline void exprNode::set_name (std::string s) {
	name = s;
}

inline void exprNode::set_name (char *s) {
	name = std::string (s);
}

inline std::string exprNode::get_name (void) {
	return name;
}

inline ETYPE exprNode::get_expr_type (void) {
	return expr_type;
}

inline void exprNode::set_expr_type (ETYPE type) {
	expr_type = type;
}

inline void exprNode::infer_expr_type (DATA_TYPE &ret) {
	ret = infer_data_type (type, ret);
}			

inline void exprNode::set_nested (bool val) {
	nested = val;
}

inline bool exprNode::is_nested (void) {
	return nested;
}

class idNode : public exprNode {
	private:
	public:
		idNode (char *);
		idNode (std::string);
		idNode (char *, DATA_TYPE, bool);
		idNode (std::string, DATA_TYPE, bool);
		void set_type (DATA_TYPE);
		bool is_id_node (void);
		void print_node (std::stringstream &);
		void decompose_access_fsm (std::string &, int &);
		bool same_expr (exprNode *);
		bool waw_dependence (exprNode *);
		bool war_dependence (exprNode *);
		bool raw_dependence (exprNode *);
		bool pointwise_occurrence (exprNode *);
		void compute_rbw (std::set<std::string> &, std::set<std::string> &, bool) {}
		void compute_wbr (std::set<std::string> &, std::set<std::string> &, bool) {}
		void determine_stmt_resource_mapping (std::map<std::tuple<std::string,int,bool>,RESOURCE> &, std::vector<std::string>, bool, std::string, bool) {}
		int compute_dimensionality (std::vector<std::string>);
		bool verify_immutable_expr (std::vector<std::string>, bool);
		int maximum_streaming_offset (exprNode*, std::string);
		int minimum_streaming_offset (std::string);
		void offset_expr (int, std::string) {}
		void print_node (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, bool, bool, std::string, std::vector<std::string>, std::map<std::string,int>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::string &);
		void print_node (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, bool, bool, std::string, std::vector<std::string>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::map<std::string,std::tuple<int,int>>, int, std::string &);
		void print_last_write (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, std::map<std::string,exprNode*>, bool, bool, std::string, std::vector<std::string>, std::map<std::string,int>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::string &) {}
		void print_last_write (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, std::map<std::string,exprNode*>, bool, bool, std::string, std::vector<std::string>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::map<std::string,std::tuple<int,int>>, int, std::string &) {}
		void compute_hull (std::map<std::string, std::vector<Range*>> &, std::vector<Range*> &, std::vector<Range*> &, bool, std::string, int);
		void compute_hull (std::map<std::string, std::vector<Range*>> &, std::vector<Range*> &, std::vector<Range*> &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, std::string, int);
		void shift_hull (std::vector<Range*> &) {}
		void shift_hull (std::vector<Range*> &, bool, std::string) {}
		void collect_accesses (std::set<std::string> &); 
		void collect_access_stats (std::map<std::tuple<std::string,int,bool>, std::tuple<int,int,int>> &, bool, std::map<std::string,int>, std::vector<std::string>, bool, std::string) {}
		void collect_access_stats (std::map<std::tuple<std::string,int,bool>, std::tuple<int,int,std::map<int,std::vector<std::string>>>> &, bool, std::vector<std::string>, bool, std::string) {}
		void set_data_type (std::map<std::string, DATA_TYPE>); 
		exprNode *unroll_expr (std::string, int, std::map<std::string,int> &, bool);
		bool identify_same_accesses (std::map<exprNode*,std::vector<exprNode*>> &, exprNode*);
		bool consolidate_same_accesses (std::vector<std::tuple<OP_TYPE,exprNode*>> &, std::map<exprNode*,std::vector<exprNode*>> &, std::map<exprNode*,exprNode*> &, exprNode*, bool);
};

inline idNode::idNode (char *s) {
	name = std::string (s);
	expr_type = T_ID;
}

inline idNode::idNode (char *s, DATA_TYPE t, bool nest) {
	name = std::string (s);
	expr_type = T_ID;
	type = t;
	nested = nest;
}

inline idNode::idNode (std::string s) {
	name = s;
	expr_type = T_ID;
}

inline idNode::idNode (std::string s, DATA_TYPE t, bool nest) {
	name = s;
	expr_type = T_ID;
	type = t;
	nested = nest;
}

inline void idNode::set_type (DATA_TYPE dtype) {
	type = dtype;
}

inline bool idNode::is_id_node (void) {
	return true;
}

inline int idNode::compute_dimensionality (std::vector<std::string> iters) {
	return 0;
}

inline int idNode::minimum_streaming_offset (std::string stream_dim) {
	return 0;
}

inline int idNode::maximum_streaming_offset (exprNode *src_lhs, std::string stream_dim) {
	return 0;
}

class uminusNode : public exprNode {
	private:
		exprNode *base_expr;
	public:
		uminusNode (exprNode *);
		uminusNode (exprNode *, char *, DATA_TYPE, bool);
		uminusNode (exprNode *, DATA_TYPE, bool);
		uminusNode (exprNode *, std::string, DATA_TYPE, bool);
		exprNode *get_base_expr (void);
		bool is_data_node (void);
		bool is_id_node (void);
		bool is_binary_node (void);
		std::string get_name (void);
		bool is_shiftvec_node (void);
		void set_type (DATA_TYPE);
		void print_node (std::stringstream &);
		void decompose_access_fsm (std::string &, int &);
		bool same_expr (exprNode *);
		bool waw_dependence (exprNode *);
		bool war_dependence (exprNode *);
		bool raw_dependence (exprNode *);
		bool pointwise_occurrence (exprNode *);
		void compute_rbw (std::set<std::string> &, std::set<std::string> &, bool);
		void compute_wbr (std::set<std::string> &, std::set<std::string> &, bool);
		void compute_last_writes (std::map<std::string,exprNode*> &, bool, std::string);
		bool is_last_write (std::map<std::string,exprNode*> &);
		void determine_stmt_resource_mapping (std::map<std::tuple<std::string,int,bool>,RESOURCE> &, std::vector<std::string>, bool, std::string, bool);
		int compute_dimensionality (std::vector<std::string>);
		bool verify_immutable_expr (std::vector<std::string>, bool);
		int maximum_streaming_offset (exprNode*, std::string);
		int minimum_streaming_offset (std::string);
		int streaming_offset (std::string);
                bool same_subscripts_for_array (std::string, std::vector<std::string> &, DATA_TYPE &);
                void replace_array_name (std::string, std::string); 
		void offset_expr (int, std::string);
		void print_node (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, bool, bool, std::string, std::vector<std::string>, std::map<std::string,int>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::string &);
		void print_node (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, bool, bool, std::string, std::vector<std::string>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::map<std::string,std::tuple<int,int>>, int, std::string &);
		void print_last_write (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, std::map<std::string,exprNode*>, bool, bool, std::string, std::vector<std::string>, std::map<std::string,int>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::string &);
		void print_last_write (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, std::map<std::string,exprNode*>, bool, bool, std::string, std::vector<std::string>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::map<std::string,std::tuple<int,int>>, int, std::string &);
		void compute_hull (std::map<std::string, std::vector<Range*>> &, std::vector<Range*> &, std::vector<Range*> &, bool, std::string, int);
		void compute_hull (std::map<std::string, std::vector<Range*>> &, std::vector<Range*> &, std::vector<Range*> &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, std::string, int);
		void shift_hull (std::vector<Range*> &);
		void shift_hull (std::vector<Range*> &, bool, std::string);
		void collect_accesses (std::set<std::string> &);
		void collect_access_stats (std::map<std::tuple<std::string,int,bool>, std::tuple<int,int,int>> &, bool, std::map<std::string,int>, std::vector<std::string>, bool, std::string); 
		void collect_access_stats (std::map<std::tuple<std::string,int,bool>, std::tuple<int,int,std::map<int,std::vector<std::string>>>> &, bool, std::vector<std::string>, bool, std::string);
		void set_data_type (std::map<std::string, DATA_TYPE>); 
		exprNode *unroll_expr (std::string, int, std::map<std::string,int> &, bool);
		void infer_expr_type (DATA_TYPE &);
		void decompose_node (exprNode*, std::vector<std::tuple<exprNode*,STMT_OP,exprNode*>> &, bool);
		exprNode *distribute_node (void);
		void accessed_arrays (std::set<std::string> &);
		void remove_redundant_nesting (void);
		bool same_operator (OP_TYPE);
		void nonassoc_chain (bool &);
		void assoc_chain (bool &);
		bool retiming_feasible (std::string, std::vector<int> &);
		int get_retiming_offset (std::string);
		exprNode *retime_node (std::string, int);
		int get_flop_count (void);
		bool identify_same_accesses (std::map<exprNode*,std::vector<exprNode*>> &, exprNode*);
		bool consolidate_same_accesses (std::vector<std::tuple<OP_TYPE,exprNode*>> &, std::map<exprNode*,std::vector<exprNode*>> &, std::map<exprNode*,exprNode*> &, exprNode*, bool);
		bool is_scalar_expr (void);
};

inline uminusNode::uminusNode (exprNode *expr) {
	base_expr = expr;
	expr_type = T_UMINUS;
}

inline uminusNode::uminusNode (exprNode *expr, DATA_TYPE t, bool nest) {
	base_expr = expr;
	expr_type = T_UMINUS;
	type = t;
	nested = nest;
}

inline uminusNode::uminusNode (exprNode *expr, char *s, DATA_TYPE t, bool nest) {
	base_expr = expr;
	expr_type = T_UMINUS;
	name = std::string (s);
	type = t;
	nested = nest;
}

inline uminusNode::uminusNode (exprNode *expr, std::string s, DATA_TYPE t, bool nest) {
	base_expr = expr;
	expr_type = T_UMINUS;
	name = s;
	type = t;
	nested = nest;
}

inline void uminusNode::set_type (DATA_TYPE dtype) {
	base_expr->set_type (dtype);
	type = dtype;
}

inline exprNode *uminusNode::get_base_expr (void) {
	return base_expr;
}

inline std::string uminusNode::get_name (void) {
	return base_expr->get_name ();
}

inline bool uminusNode::waw_dependence (exprNode *node) {
	return base_expr->waw_dependence (node);
}

inline bool uminusNode::raw_dependence (exprNode *node) {
	return base_expr->raw_dependence (node);
}

inline void uminusNode::compute_rbw (std::set<std::string> &reads, std::set<std::string> &writes, bool is_read) {
	base_expr->compute_rbw (reads, writes, is_read);
}

inline void uminusNode::compute_wbr (std::set<std::string> &writes, std::set<std::string> &reads, bool is_write) {
	base_expr->compute_wbr (writes, reads, is_write);
}

inline void uminusNode::compute_last_writes (std::map<std::string,exprNode*> &last_writes, bool stream, std::string stream_dim) {
	base_expr->compute_last_writes (last_writes, stream, stream_dim);
}

inline bool uminusNode::is_last_write (std::map<std::string,exprNode*> &last_writes) {
	return base_expr->is_last_write (last_writes);
}

inline bool uminusNode::war_dependence (exprNode *node) {
	return base_expr->war_dependence (node);
}

inline void uminusNode::accessed_arrays (std::set<std::string> & arrays) {
	base_expr->accessed_arrays (arrays);
}

inline bool uminusNode::pointwise_occurrence (exprNode *node) {
	return base_expr->pointwise_occurrence (node);
}

inline bool uminusNode::is_scalar_expr (void) {
	return base_expr->is_scalar_expr ();
}

inline void uminusNode::determine_stmt_resource_mapping (std::map<std::tuple<std::string,int,bool>,RESOURCE> &rmap, std::vector<std::string> iters, bool stream, std::string stream_dim, bool use_shmem) {
	base_expr->determine_stmt_resource_mapping (rmap, iters, stream, stream_dim, use_shmem);
}

inline int uminusNode::compute_dimensionality (std::vector<std::string> iters) {
	return base_expr->compute_dimensionality (iters);
}

inline bool uminusNode::verify_immutable_expr (std::vector<std::string> writes, bool start_verification) {
	return base_expr->verify_immutable_expr (writes, start_verification);
}

inline int uminusNode::minimum_streaming_offset (std::string stream_dim) {
	return base_expr->minimum_streaming_offset (stream_dim);
}

inline int uminusNode::streaming_offset (std::string stream_dim) {
	return base_expr->streaming_offset (stream_dim);
}

inline int uminusNode::get_retiming_offset (std::string stream_dim) {
	return base_expr->get_retiming_offset (stream_dim);
}

inline bool uminusNode::retiming_feasible (std::string stream_dim, std::vector<int> &offset) {
	return base_expr->retiming_feasible (stream_dim, offset);
}

inline int uminusNode::maximum_streaming_offset (exprNode *src_lhs, std::string stream_dim) {
	return base_expr->maximum_streaming_offset (src_lhs, stream_dim);
}

inline void uminusNode::offset_expr (int offset, std::string stream_dim) {
	return base_expr->offset_expr (offset, stream_dim);
}

inline void uminusNode::compute_hull (std::map<std::string, std::vector<Range*>> &hull_map, std::vector<Range*> &hull, std::vector<Range*> &initial_hull, bool stream, std::string stream_dim, int offset) {
	base_expr->compute_hull (hull_map, hull, initial_hull, stream, stream_dim, offset);
}

inline void uminusNode::compute_hull (std::map<std::string, std::vector<Range*>> &hull_map, std::vector<Range*> &hull, std::vector<Range*> &initial_hull, std::map<std::tuple<std::string,int,bool>,RESOURCE> rmap, bool stream, std::string stream_dim, int offset) {
	base_expr->compute_hull (hull_map, hull, initial_hull, rmap, stream, stream_dim, offset);
}

inline void uminusNode::shift_hull (std::vector<Range*> &hull) {
	base_expr->shift_hull (hull);
}

inline void uminusNode::shift_hull (std::vector<Range*> &hull, bool stream, std::string stream_dim) {
	base_expr->shift_hull (hull, stream, stream_dim);
}

inline void uminusNode::collect_accesses (std::set<std::string> &accesses) {
	base_expr->collect_accesses (accesses);
}

inline void uminusNode::collect_access_stats (std::map<std::tuple<std::string,int,bool>,std::tuple<int,int,int>> &access_map, bool is_read, std::map<std::string,int> blockdims, std::vector<std::string> iters, bool stream, std::string stream_dim) {
	base_expr->collect_access_stats (access_map, is_read, blockdims, iters, stream, stream_dim);
}

inline void uminusNode::collect_access_stats (std::map<std::tuple<std::string,int,bool>, std::tuple<int,int,std::map<int,std::vector<std::string>>>> &access_map, bool is_read, std::vector<std::string> iters, bool stream, std::string stream_dim) {
	base_expr->collect_access_stats (access_map, is_read, iters, stream, stream_dim);
}

inline void uminusNode::set_data_type (std::map<std::string, DATA_TYPE> data_types) {
	base_expr->set_data_type (data_types);
}

inline void uminusNode::infer_expr_type (DATA_TYPE &ret) {
	base_expr->infer_expr_type (ret);
}

inline void uminusNode::nonassoc_chain (bool &ret) {
	base_expr->nonassoc_chain (ret);
}

inline void uminusNode::assoc_chain (bool &ret) {
	base_expr->assoc_chain (ret);
}

inline bool uminusNode::is_data_node (void) {
	return base_expr->is_data_node ();
}

inline bool uminusNode::is_id_node (void) {
	return base_expr->is_id_node ();
}

inline bool uminusNode::is_binary_node (void) {
	return base_expr->is_binary_node ();
}

inline bool uminusNode::is_shiftvec_node (void) {
	return base_expr->is_shiftvec_node ();
}

inline int uminusNode::get_flop_count (void) {
	return base_expr->get_flop_count ();
}

inline bool uminusNode::same_subscripts_for_array (std::string arr_name, std::vector<std::string> &subscripts, DATA_TYPE &arr_type) {
	return base_expr->same_subscripts_for_array (arr_name, subscripts, arr_type);
}

inline void uminusNode::replace_array_name (std::string old_name, std::string new_name) {
	base_expr->replace_array_name (old_name, new_name);
}

class binaryNode : public exprNode {
	private:
		OP_TYPE op;
		exprNode *lhs, *rhs;
	public:
		binaryNode (OP_TYPE, exprNode *, exprNode *);
		binaryNode (OP_TYPE, exprNode *, exprNode *, bool);
		binaryNode (OP_TYPE, exprNode *, exprNode *, DATA_TYPE, bool);
		binaryNode (OP_TYPE, exprNode *, exprNode *, std::string, DATA_TYPE, bool);
		binaryNode (OP_TYPE, exprNode *, exprNode *, char *, DATA_TYPE, bool);
		OP_TYPE get_operator (void);
		exprNode *get_rhs (void);
		exprNode *get_lhs (void);
		void set_type (DATA_TYPE);
		bool is_binary_node (void);
		void print_node (std::stringstream &);
		void decompose_access_fsm (std::string &, int &);
		bool same_expr (exprNode *);
		bool waw_dependence (exprNode *);
		bool war_dependence (exprNode *);
		bool raw_dependence (exprNode *);
		bool pointwise_occurrence (exprNode *);
		void compute_rbw (std::set<std::string> &, std::set<std::string> &, bool);
		void compute_wbr (std::set<std::string> &, std::set<std::string> &, bool); 
		void compute_last_writes (std::map<std::string,exprNode*> &, bool, std::string) {}
		void determine_stmt_resource_mapping (std::map<std::tuple<std::string,int,bool>,RESOURCE> &, std::vector<std::string>, bool, std::string, bool);
		int compute_dimensionality (std::vector<std::string>);
		bool verify_immutable_expr (std::vector<std::string>, bool);
		int maximum_streaming_offset (exprNode*, std::string);
		int minimum_streaming_offset (std::string);
                bool same_subscripts_for_array (std::string, std::vector<std::string> &, DATA_TYPE &); 
                void replace_array_name (std::string, std::string);
		void offset_expr (int, std::string);
		void print_node (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, bool, bool, std::string, std::vector<std::string>, std::map<std::string,int>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::string &);
		void print_node (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, bool, bool, std::string, std::vector<std::string>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::map<std::string,std::tuple<int,int>>, int, std::string &);
		void print_last_write (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, std::map<std::string,exprNode*>, bool, bool, std::string, std::vector<std::string>, std::map<std::string,int>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::string &);
		void print_last_write (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, std::map<std::string,exprNode*>, bool, bool, std::string, std::vector<std::string>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::map<std::string,std::tuple<int,int>>, int, std::string &);
		void compute_hull (std::map<std::string, std::vector<Range*>> &, std::vector<Range*> &, std::vector<Range*> &, bool, std::string, int);
		void compute_hull (std::map<std::string, std::vector<Range*>> &, std::vector<Range*> &, std::vector<Range*> &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, std::string, int);
		void shift_hull (std::vector<Range*> &);
		void shift_hull (std::vector<Range*> &, bool, std::string);
		void collect_accesses (std::set<std::string> &);
		void collect_access_stats (std::map<std::tuple<std::string,int,bool>, std::tuple<int,int,int>> &, bool, std::map<std::string,int>, std::vector<std::string>, bool, std::string);
		void collect_access_stats (std::map<std::tuple<std::string,int,bool>, std::tuple<int,int,std::map<int,std::vector<std::string>>>> &, bool, std::vector<std::string>, bool, std::string);
		void set_data_type (std::map<std::string, DATA_TYPE>);
		exprNode *unroll_expr (std::string, int, std::map<std::string,int> &, bool);
		void infer_expr_type (DATA_TYPE &);
		void decompose_node (exprNode*, std::vector<std::tuple<exprNode*,STMT_OP,exprNode*>> &, bool);
		exprNode *distribute_node (void);
		void accessed_arrays (std::set<std::string> &);
		void remove_redundant_nesting (void);
		bool same_operator (OP_TYPE);
		void nonassoc_chain (bool &);
		void assoc_chain (bool &);
		bool retiming_feasible (std::string, std::vector<int> &);
		int get_retiming_offset (std::string);
		exprNode *retime_node (std::string, int);
		int get_flop_count (void);
		bool identify_same_accesses (std::map<exprNode*,std::vector<exprNode*>> &, exprNode*);
		bool consolidate_same_accesses (std::vector<std::tuple<OP_TYPE,exprNode*>> &, std::map<exprNode*,std::vector<exprNode*>> &, std::map<exprNode*,exprNode*> &, exprNode*, bool);
		bool is_scalar_expr (void);
};

inline binaryNode::binaryNode (OP_TYPE a_type, exprNode *a_lhs, exprNode *a_rhs) {
	op = a_type;
	lhs = a_lhs;
	rhs = a_rhs;
	expr_type = T_BINARY;
}

inline binaryNode::binaryNode (OP_TYPE a_type, exprNode *a_lhs, exprNode *a_rhs, bool nest) {
	op = a_type;
	lhs = a_lhs;
	rhs = a_rhs;
	expr_type = T_BINARY;
	nested = nest;
}

inline binaryNode::binaryNode (OP_TYPE a_type, exprNode *a_lhs, exprNode *a_rhs, DATA_TYPE t, bool nest) {
	op = a_type;
	lhs = a_lhs;
	rhs = a_rhs;
	expr_type = T_BINARY;
	type = t;
	nested = nest;
}

inline binaryNode::binaryNode (OP_TYPE a_type, exprNode *a_lhs, exprNode *a_rhs, char *s, DATA_TYPE t, bool nest) {
	op = a_type;
	lhs = a_lhs;
	rhs = a_rhs;
	expr_type = T_BINARY;
	name = std::string (s);
	type = t;
	nested = nest;
}

inline binaryNode::binaryNode (OP_TYPE a_type, exprNode *a_lhs, exprNode *a_rhs, std::string s, DATA_TYPE t, bool nest) {
	op = a_type;
	lhs = a_lhs;
	rhs = a_rhs;
	expr_type = T_BINARY;
	name = s;
	type = t;
	nested = nest;
}

inline bool binaryNode::is_binary_node (void) {
	return true;
}

inline OP_TYPE binaryNode::get_operator (void) {
	return op;
}

inline exprNode *binaryNode::get_lhs (void) {
	return lhs;
}

inline exprNode *binaryNode::get_rhs (void) {
	return rhs;
}

inline void binaryNode::set_type (DATA_TYPE dtype) {
	lhs->set_type (dtype);
	rhs->set_type (dtype);
	type = dtype;
}

class shiftvecNode : public exprNode {
	private:
		std::vector<exprNode*> indices;
	public:
		shiftvecNode ();
		shiftvecNode (char *);
		shiftvecNode (std::string);
		shiftvecNode (char *, DATA_TYPE, bool);
		shiftvecNode (std::string, DATA_TYPE, bool);
		void set_type (DATA_TYPE);
		void push_index (exprNode *);
		void set_index (exprNode *, int);
		std::vector<exprNode*> get_indices (void);
		void set_indices (std::vector<exprNode*> &);
		void print_node (std::stringstream &);
		std::string print_array (void);
		std::string print_array (std::vector<std::string>, bool, bool, std::string, std::map<std::string,int>, bool, std::vector<std::string>, std::vector<std::string>, bool, bool, std::string &);
		std::string print_array (std::vector<std::string>, bool, bool, std::string, std::map<std::string,int>, bool, std::vector<std::string>, std::vector<std::string>, bool, bool, std::map<std::string,std::tuple<int,int>>, int, std::string &);
		bool is_shiftvec_node (void);
		bool same_expr (exprNode *);
		bool waw_dependence (exprNode *);
		bool war_dependence (exprNode *);
		bool raw_dependence (exprNode *);
		bool pointwise_occurrence (exprNode *);
		void compute_rbw (std::set<std::string> &, std::set<std::string> &, bool);
		void compute_wbr (std::set<std::string> &, std::set<std::string> &, bool);
		void compute_last_writes (std::map<std::string,exprNode*> &, bool, std::string);
		bool is_last_write (std::map<std::string,exprNode*> &);
		void determine_stmt_resource_mapping (std::map<std::tuple<std::string,int,bool>,RESOURCE> &, std::vector<std::string>, bool, std::string, bool);
		int compute_dimensionality (std::vector<std::string>);
                bool same_subscripts_for_array (std::string, std::vector<std::string> &, DATA_TYPE &); 
                void replace_array_name (std::string, std::string);
		bool verify_immutable_expr (std::vector<std::string>, bool);
		int maximum_streaming_offset (exprNode*, std::string);
		int streaming_offset (std::string);
		int minimum_streaming_offset (std::string); 
		void offset_expr (int, std::string);
		void accessed_arrays (std::set<std::string> &);
		void print_node (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, bool, bool, std::string, std::vector<std::string>, std::map<std::string,int>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::string &);
		void print_node (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, bool, bool, std::string, std::vector<std::string>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::map<std::string,std::tuple<int,int>>, int, std::string &);
		void print_last_write (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, std::map<std::string,exprNode*>, bool, bool, std::string, std::vector<std::string>, std::map<std::string,int>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::string &);
		void print_last_write (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, std::map<std::string,exprNode*>, bool, bool, std::string, std::vector<std::string>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::map<std::string,std::tuple<int,int>>, int, std::string &);
		std::string print_array (std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, bool, bool, std::string, std::vector<std::string>, std::map<std::string,int>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::string &);
		std::string print_array (std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, bool, bool, std::string, std::vector<std::string>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::map<std::string,std::tuple<int,int>>, int, std::string &);
		void compute_hull (std::map<std::string, std::vector<Range*>> &, std::vector<Range*> &, std::vector<Range*> &, bool, std::string, int);
		void compute_hull (std::map<std::string, std::vector<Range*>> &, std::vector<Range*> &, std::vector<Range*> &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, std::string, int);
		void shift_hull (std::vector<Range*> &);
		void shift_hull (std::vector<Range*> &, bool, std::string);
		void collect_accesses (std::set<std::string> &);
		void collect_access_stats (std::map<std::tuple<std::string,int,bool>, std::tuple<int,int,int>> &, bool, std::map<std::string,int>, std::vector<std::string>, bool, std::string);
		void collect_access_stats (std::map<std::tuple<std::string,int,bool>, std::tuple<int,int,std::map<int,std::vector<std::string>>>> &, bool, std::vector<std::string>, bool, std::string);
		void set_data_type (std::map<std::string, DATA_TYPE>);
		exprNode *unroll_expr (std::string, int, std::map<std::string,int> &, bool);
		bool retiming_feasible (std::string, std::vector<int> &);
		int get_retiming_offset (std::string);
		exprNode *retime_node (std::string, int);
		bool identify_same_accesses (std::map<exprNode*,std::vector<exprNode*>> &, exprNode*);
		bool consolidate_same_accesses (std::vector<std::tuple<OP_TYPE,exprNode*>> &, std::map<exprNode*,std::vector<exprNode*>> &, std::map<exprNode*,exprNode*> &, exprNode*, bool);
		bool is_scalar_expr (void);
};

inline shiftvecNode::shiftvecNode () {
	expr_type = T_SHIFTVEC;
}

inline shiftvecNode::shiftvecNode (char *s) {
	name = std::string (s);
	expr_type = T_SHIFTVEC;
}

inline shiftvecNode::shiftvecNode (std::string s) {
	name = s;
	expr_type = T_SHIFTVEC;
}

inline shiftvecNode::shiftvecNode (char *s, DATA_TYPE t, bool nest) {
	name = std::string (s);
	expr_type = T_SHIFTVEC;
	type = t;
	nested = nest;
}

inline shiftvecNode::shiftvecNode (std::string s, DATA_TYPE t, bool nest) {
	name = s;
	expr_type = T_SHIFTVEC;
	type = t;
	nested = nest;
}

inline void shiftvecNode::set_type (DATA_TYPE dtype) {
	type = dtype;
}

inline bool shiftvecNode::is_shiftvec_node (void) {
	return true;
}

inline void shiftvecNode::push_index (exprNode *node) {
	indices.push_back (node);
}

inline void shiftvecNode::set_index (exprNode *node, int pos) {
	indices[pos] = node;
}

inline void shiftvecNode::set_indices (std::vector<exprNode*> &nodes) {
	indices = nodes;
}

inline std::vector<exprNode*> shiftvecNode::get_indices (void) {
	return indices;
}

inline bool shiftvecNode::is_scalar_expr (void) {
	return false;
}

template <typename T>
class datatypeNode : public exprNode {
	private:
		T value;
	public:
		datatypeNode (T, DATA_TYPE);
		T get_value (void);
		void set_type (DATA_TYPE);
		void print_node (std::stringstream &);
		void decompose_access_fsm (std::string &, int &);
		bool same_expr (exprNode *);
		bool waw_dependence (exprNode *);
		bool war_dependence (exprNode *);
		bool raw_dependence (exprNode *);
		bool is_data_node (void);
		bool pointwise_occurrence (exprNode *);
		void compute_rbw (std::set<std::string> &, std::set<std::string> &, bool){}
		void compute_wbr (std::set<std::string> &, std::set<std::string> &, bool){}
		void compute_last_writes (std::map<std::string,exprNode*> &, bool, std::string) {}
		void determine_stmt_resource_mapping (std::map<std::tuple<std::string,int,bool>,RESOURCE> &, std::vector<std::string>, bool, std::string, bool) {}
		int compute_dimensionality (std::vector<std::string>);
		bool verify_immutable_expr (std::vector<std::string>, bool);
		int maximum_streaming_offset (exprNode*, std::string); 
		int minimum_streaming_offset (std::string);
		void offset_expr (int, std::string) {}
		void print_node (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, bool, bool, std::string, std::vector<std::string>, std::map<std::string,int>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::string &);
		void print_node (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, bool, bool, std::string, std::vector<std::string>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::map<std::string,std::tuple<int,int>>, int, std::string &);
		void print_last_write (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, std::map<std::string,exprNode*>, bool, bool, std::string, std::vector<std::string>, std::map<std::string,int>, std::vector<std::string>, bool, bool, bool, std::map<std::string,int>, std::vector<std::string>, std::string &) {}
		void print_last_write (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, std::map<std::string,exprNode*>, bool, bool, std::string, std::vector<std::string>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::map<std::string,std::tuple<int,int>>, int, std::string &) {}
		void compute_hull (std::map<std::string, std::vector<Range*>> &, std::vector<Range*> &, std::vector<Range*> &, bool, std::string, int) {}
		void compute_hull (std::map<std::string, std::vector<Range*>> &, std::vector<Range*> &, std::vector<Range*> &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, std::string, int) {}
		void shift_hull (std::vector<Range*> &) {}
		void shift_hull (std::vector<Range*> &, bool, std::string) {}
		void collect_accesses (std::set<std::string> &) {}
		void collect_access_stats (std::map<std::tuple<std::string,int,bool>, std::tuple<int,int,int>> &, bool, std::map<std::string,int>, std::vector<std::string>, bool, std::string) {}
		void collect_access_stats (std::map<std::tuple<std::string,int,bool>, std::tuple<int,int,std::map<int,std::vector<std::string>>>> &, bool, std::vector<std::string>, bool, std::string) {}
		void set_data_type (std::map<std::string, DATA_TYPE>) {}
		exprNode *unroll_expr (std::string, int, std::map<std::string,int> &, bool) {return this;}
		bool identify_same_accesses (std::map<exprNode*,std::vector<exprNode*>> &, exprNode*);
		bool consolidate_same_accesses (std::vector<std::tuple<OP_TYPE,exprNode*>> &, std::map<exprNode*,std::vector<exprNode*>> &, std::map<exprNode*,exprNode*> &, exprNode*, bool);
};

template <typename T>
inline datatypeNode<T>::datatypeNode (T val, DATA_TYPE dtype) {
	value = val;
	type = dtype;
	expr_type = T_DATATYPE;
}

template <typename T>
inline T datatypeNode<T>::get_value (void) {
	return value;
}			

template <typename T>
inline void datatypeNode<T>::set_type (DATA_TYPE dtype) {
	type = dtype;
}

template <typename T>
inline bool datatypeNode<T>::same_expr (exprNode *node) {
	bool same_expr = type == node->get_type ();
	same_expr &= expr_type == node->get_expr_type ();
	if (same_expr)
		same_expr &= value == dynamic_cast<datatypeNode*>(node)->get_value ();
	return same_expr;
}

template <typename T>
inline bool datatypeNode<T>::identify_same_accesses (std::map<exprNode*,std::vector<exprNode*>> &same_accesses, exprNode *access) {
	bool found = false;
	for (auto it : same_accesses) {
		if (same_expr(it.first)) {
			(same_accesses[it.first]).push_back (access);
			found = true;
		}
	}
	if (!found && access!=NULL) 
		(same_accesses[this]).push_back (access);
	return found;
}

template <typename T>
inline bool datatypeNode<T>::consolidate_same_accesses (std::vector<std::tuple<OP_TYPE,exprNode*>> &consolidated_exprs, std::map<exprNode*,std::vector<exprNode*>> &same_accesses, std::map<exprNode*,exprNode*> &cmap, exprNode* access, bool flip) {
	bool found = false;
	if (access != NULL) {
		for (auto it : same_accesses) {
			if (same_expr (it.first)) {
				for (auto jt : it.second) {
					if (access->same_expr (jt))
						found = true;
				} 
			}
		}   
		if (found) {
			bool rhs_exists = false;
			for (auto it : cmap) {
				if (same_expr (it.first)) {
					OP_TYPE op = flip ? T_MINUS : T_PLUS;
					cmap[it.first] = new binaryNode (op, it.second, access, infer_data_type(access->get_type(), (it.second)->get_type()), false);
					rhs_exists = true;
					break;
				}
			}
			if (!rhs_exists) {
				cmap[this] = flip ? new uminusNode (access) : access;
			}
		}
	}
	return found;
}

template <typename T>
inline bool datatypeNode<T>::waw_dependence (exprNode *node) {
	return false;
}

template <typename T>
inline bool datatypeNode<T>::raw_dependence (exprNode *node) {
	return false;
}

template <typename T>
inline bool datatypeNode<T>::war_dependence (exprNode *node) {
	return false;
}

template <typename T>
inline bool datatypeNode<T>::is_data_node (void) {
	return true;
}

template <typename T>
inline bool datatypeNode<T>::pointwise_occurrence (exprNode *node) {
	return true;
}

template<typename T>
inline void datatypeNode<T>::print_node (std::stringstream &out) {
	std::stringstream val;
	val << value;
	if (type == FLOAT) {
		if (val.str().find (".") == std::string::npos && 
			val.str().find("e") == std::string::npos && val.str().find("E") == std::string::npos)
			val << ".0";
		val << "f";
	}
	else if (type == DOUBLE) {
		if (val.str().find (".") == std::string::npos && 
			val.str().find("e") == std::string::npos && val.str().find("E") == std::string::npos)
			val << ".0";
	}
	out << val.str ();
}

template <typename T>
inline int datatypeNode<T>::compute_dimensionality (std::vector<std::string> iters) {
	return 0;
}

template <typename T>
inline bool datatypeNode<T>::verify_immutable_expr (std::vector<std::string> writes, bool start_verification) {
	return true;
}

template <typename T>
inline int datatypeNode<T>::minimum_streaming_offset (std::string stream_dim) {
	return 0;
}

template <typename T>
inline int datatypeNode<T>::maximum_streaming_offset (exprNode *src_lhs, std::string stream_dim) {
	return 0;
}

template<typename T>
inline void datatypeNode<T>::decompose_access_fsm (std::string &id, int &offset) {
	if (DEBUG) assert (type == INT && "Array access has non-int offset (array_access_info)");
	offset += value;
}

template<typename T>
inline void datatypeNode<T>::print_node (std::stringstream &out, std::map<std::tuple<std::string,int,bool>,RESOURCE> rmap, bool is_lhs, bool full_stream, bool stream, std::string stream_dim, std::vector<std::string> temp_arrays, std::map<std::string,int> unroll_instance, bool blocked_loads, std::map<std::string,int> udecls, std::vector<std::string> params, std::vector<std::string> iters, bool linearize_accesses, bool code_diff, std::string &cstream_shift) {
	print_node (out);
}

template<typename T>
inline void datatypeNode<T>::print_node (std::stringstream &out, std::map<std::tuple<std::string,int,bool>,RESOURCE> rmap, bool is_lhs, bool full_stream, bool stream, std::string stream_dim, std::vector<std::string> temp_arrays, bool blocked_loads, std::map<std::string,int> udecls, std::vector<std::string> params, std::vector<std::string> iters, bool linearize_accesses, bool code_diff, std::map<std::string,std::tuple<int,int>> halo_dims, int reg_count, std::string &cstream_shift) {
	print_node (out);
}

class functionNode : public exprNode {
	private:
		std::vector<exprNode*> arg_list;
	public:
		functionNode (char *, std::vector<exprNode*> *);
		functionNode (char *, std::vector<exprNode*> *, DATA_TYPE, bool);
		functionNode (std::string, std::vector<exprNode*> *);
		functionNode (std::string, std::vector<exprNode*> *, DATA_TYPE, bool);
		void set_type (DATA_TYPE);
		std::vector<exprNode*> get_arg_list (void);
		void set_arg (exprNode *, unsigned int);
		void print_node (std::stringstream &);
		bool same_expr (exprNode *);
		bool waw_dependence (exprNode *);
		bool war_dependence (exprNode *);
		bool raw_dependence (exprNode *);
		bool pointwise_occurrence (exprNode *);
		void compute_rbw (std::set<std::string> &, std::set<std::string> &, bool);
		void compute_wbr (std::set<std::string> &, std::set<std::string> &, bool);
		void compute_last_writes (std::map<std::string,exprNode*> &, bool, std::string) {}
		void determine_stmt_resource_mapping (std::map<std::tuple<std::string,int,bool>,RESOURCE> &, std::vector<std::string>, bool, std::string, bool);
		int compute_dimensionality (std::vector<std::string>);
		bool verify_immutable_expr (std::vector<std::string>, bool);
		int maximum_streaming_offset (exprNode*, std::string);
		int minimum_streaming_offset (std::string); 
		void offset_expr (int, std::string);
		void accessed_arrays (std::set<std::string> &);
		void print_node (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, bool, bool, std::string, std::vector<std::string>, std::map<std::string,int>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::string &);
		void print_node (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, bool, bool, std::string, std::vector<std::string>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::map<std::string,std::tuple<int,int>>, int, std::string &);
		void print_last_write (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, std::map<std::string,exprNode*>, bool, bool, std::string, std::vector<std::string>, std::map<std::string,int>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::string &);
		void print_last_write (std::stringstream &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, std::map<std::string,exprNode*>, bool, bool, std::string, std::vector<std::string>, bool, std::map<std::string,int>, std::vector<std::string>, std::vector<std::string>, bool, bool, std::map<std::string,std::tuple<int,int>>, int, std::string &);
		void compute_hull (std::map<std::string, std::vector<Range*>> &, std::vector<Range*> &, std::vector<Range*> &, bool, std::string, int);
		void compute_hull (std::map<std::string, std::vector<Range*>> &, std::vector<Range*> &, std::vector<Range*> &, std::map<std::tuple<std::string,int,bool>,RESOURCE>, bool, std::string, int);
		void shift_hull (std::vector<Range*> &);
		void shift_hull (std::vector<Range*> &, bool, std::string);
		void collect_accesses (std::set<std::string> &);
                bool same_subscripts_for_array (std::string, std::vector<std::string> &, DATA_TYPE &); 
                void replace_array_name (std::string, std::string);
		void collect_access_stats (std::map<std::tuple<std::string,int,bool>, std::tuple<int,int,int>> &, bool, std::map<std::string,int>, std::vector<std::string>, bool, std::string);
		void collect_access_stats (std::map<std::tuple<std::string,int,bool>, std::tuple<int,int,std::map<int,std::vector<std::string>>>> &, bool, std::vector<std::string>, bool, std::string);
		void set_data_type (std::map<std::string, DATA_TYPE>);
		exprNode *unroll_expr (std::string, int, std::map<std::string,int> &, bool);
		void infer_expr_type (DATA_TYPE &);
		bool retiming_feasible (std::string, std::vector<int> &);
		int get_retiming_offset (std::string);
		exprNode *retime_node (std::string, int);
		int get_flop_count (void);
		bool is_scalar_expr (void);
};

inline functionNode::functionNode (char *s, std::vector<exprNode*> *l) {
	name = std::string (s);
	arg_list = *l;
	expr_type = T_FUNCTION; 
}

inline functionNode::functionNode (char *s, std::vector<exprNode*> *l, DATA_TYPE t, bool nest) {
	name = std::string (s);
	arg_list = *l;
	expr_type = T_FUNCTION; 
	type = t;
	nested = nest;
}

inline functionNode::functionNode (std::string s, std::vector<exprNode*> *l) {
	name = s;
	arg_list = *l;
	expr_type = T_FUNCTION; 
}

inline functionNode::functionNode (std::string s, std::vector<exprNode*> *l, DATA_TYPE t, bool nest) { 
	name = s;
	arg_list = *l; 
	expr_type = T_FUNCTION;
	type = t;
	nested = nest;
}

inline void functionNode::set_type (DATA_TYPE dtype) {
	for (auto l : arg_list) 	
		l->set_type (dtype);
	type = dtype;
}

inline bool functionNode::same_expr (exprNode *node) {
	return false;
}

inline std::vector<exprNode*> functionNode::get_arg_list (void) {
	return arg_list;
}

inline void functionNode::set_arg (exprNode *n, unsigned int pos) {
	if (pos < arg_list.size())
		arg_list[pos] = n;
}

#endif
