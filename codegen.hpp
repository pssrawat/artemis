#ifndef __CODEGEN_HPP__
#define __CODEGEN_HPP__
#include <cstdio>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <tuple>
#include <algorithm>
#include "funcdefn.hpp"

class codegen {
	private:
		startNode *start;
		std::stringstream header;
		std::stringstream gpu_code;
		std::stringstream host_code;
		int dim;
		symtabTemplate <int> *n_dims, *block_dims, *unroll_decls;
		bool full_stream, stream, use_shmem, blocked_loads, generate_if, prefetch, decompose, retime, fold_computations, linearize_accesses, code_diff;
		std::map<std::string,std::tuple<int,int>> halo_dims;
		std::string stream_dim;
	public:
		codegen (startNode *);
		codegen (startNode *, symtabTemplate<int>*, symtabTemplate<int>*, symtabTemplate<int>*, bool, bool, std::string, bool, bool, bool, std::map<std::string,std::tuple<int,int>>, bool, bool, bool, bool, bool, bool);
		void print_stencil_details (void);
		std::string create_unified_stencil (void);
		void generate_header (std::stringstream &);
		void generate_code (std::stringstream &);
};

inline codegen::codegen (startNode *node) {
	start = node;
}

inline codegen::codegen (startNode *node, symtabTemplate<int> *nd, symtabTemplate<int> *bd, symtabTemplate<int> *uf, bool fstrm, bool strm, std::string str_dim, bool shm, bool bloads, bool gen_if, std::map<std::string,std::tuple<int,int>> hd, bool pfetch, bool decomp, bool rtime, bool fold, bool linearize, bool cdiff) {
	start = node;
	n_dims = nd;
	block_dims = bd;
	unroll_decls = uf;
	full_stream = fstrm;
	stream = strm;
	stream_dim = str_dim;
	use_shmem = shm;
	blocked_loads = bloads;
	generate_if = gen_if;
	halo_dims = hd;
	prefetch = pfetch & strm;
	decompose = decomp;
	retime = rtime;
	fold_computations = fold;
	linearize_accesses = linearize;
	code_diff = cdiff;
}

#endif
