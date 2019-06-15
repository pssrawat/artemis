#ifndef __ARRDECL_HPP__
#define __ARRDECL_HPP__
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include "utils.hpp"
#include "templates.hpp"

class exprNode;

//Defines the [lo:hi] range
class Range {
	private:
		exprNode *lo;
		exprNode *hi;
	public:
		Range (exprNode *, exprNode *);
		exprNode *get_lo_range ();
		exprNode *get_hi_range ();
		void set_lo_range (exprNode *);
		void set_hi_range (exprNode *);
};

inline Range::Range (exprNode *l, exprNode *h) {
	lo = l;
	hi = h;
}

inline void Range::set_lo_range (exprNode *l) {
	lo = l;
}

inline exprNode *Range::get_lo_range (void) {
	return lo;
}

inline void Range::set_hi_range (exprNode *h) {
	hi = h;
}

inline exprNode *Range::get_hi_range (void) {
	return hi;
}

//An array declaration. Contains the type, name, and range
class arrayDecl {
	private:
		DATA_TYPE data_type;
		std::string name;
		std::vector<Range*> range;
		bool temp_array = false;
	public:
		arrayDecl (std::string, std::vector<Range*>, DATA_TYPE);
		void push_range (Range *);
		void set_array_type (DATA_TYPE);
		std::vector<Range *> get_array_range (void);
		std::string get_array_name (void);
		void set_array_name (std::string);
		DATA_TYPE get_array_type (void);
		void set_temp_array (bool);
		bool is_temp_array (void);
};

inline arrayDecl::arrayDecl (std::string str, std::vector<Range*> r, DATA_TYPE t=INT) {
	data_type = t;
	range = r;
	name = str;
}

inline void arrayDecl::set_array_type (DATA_TYPE t) {
	data_type = t;
}

inline void arrayDecl::push_range (Range *r) {
	range.push_back (r);
}

inline DATA_TYPE arrayDecl::get_array_type (void) {
	return data_type;
}

inline bool arrayDecl::is_temp_array (void) {
	return temp_array;
}

inline void arrayDecl::set_temp_array (bool t) {
	temp_array = t;
}

inline std::vector<Range *> arrayDecl::get_array_range (void) {
	return range;
}

inline std::string arrayDecl::get_array_name (void) {
	return name;
}

inline void arrayDecl::set_array_name (std::string s) {
	name = s;
}

class stencilCall {
	private:
		std::string stencil_name;
		std::vector<std::string> args;
		std::vector<Range*> domain;
		Range *iterations;
	public:
		stencilCall (std::string, std::vector<std::string>&);
		stencilCall (std::string, std::vector<std::string>&, std::vector<Range*>&);
		stencilCall (std::string, std::vector<std::string>&, std::vector<Range*>&, Range *);
		void set_name (std::string);
		std::string get_name (void);
		void push_arg (std::string);
		std::vector<std::string>& get_arg_list (void);
		std::vector<Range*>& get_domain (void);
		void set_domain (std::vector<Range*>&);
		void set_iterations (Range *);
		Range* get_iterations (void); 
};

inline stencilCall::stencilCall (std::string s, std::vector<std::string> &arg) {
	stencil_name = s;
	args = arg;
}

inline stencilCall::stencilCall (std::string s, std::vector<std::string> &arg, std::vector<Range*> &dom) {
	stencil_name = s;
	domain = dom;
	args = arg;
}

inline stencilCall::stencilCall (std::string s, std::vector<std::string> &arg, std::vector<Range*> &dom, Range *iter) {
	stencil_name = s;
	domain = dom;
	args = arg;
	iterations = iter;
}

inline void stencilCall::set_name (std::string s) {
	stencil_name = s;
}

inline void stencilCall::set_iterations (Range *iter) {
	iterations = iter;
}

inline Range* stencilCall::get_iterations (void) {
	return iterations;
}

inline std::string stencilCall::get_name (void) {
	return stencil_name;
}

inline void stencilCall::push_arg (std::string s) {
	args.push_back (s);
}

inline std::vector<std::string>& stencilCall::get_arg_list (void) {
	return args;
}

inline void stencilCall::set_domain (std::vector<Range*> &in) {
	domain = in;
}

inline std::vector<Range*>& stencilCall::get_domain (void) {
	return domain;
}

#endif
