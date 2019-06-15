#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <fstream>
#include "grammar.hpp"
#include "codegen.hpp"

using namespace std;

startNode * grammar::start = NULL;

int main (int argc, char **argv) {
	string outfile ("--out-file");
	string out_name ("out.cu");
	string ndim ("--ndim");
	symtabTemplate <int> *n_dims = new symtabTemplate<int>;
	string blockdim ("--blockdim");
	symtabTemplate <int> *block_dims = new symtabTemplate<int>;
	block_dims->push_symbol ((char*)"x", 16);
	block_dims->push_symbol ((char*)"y", 8);
	block_dims->push_symbol ((char*)"z", 8);
	string unroll ("--unroll");
	symtabTemplate <int> *unroll_decls = new symtabTemplate<int>;
	string streaming ("--stream");
	string stream_dim="";
	bool stream = false;
	string fstream ("--full-stream");
	bool full_stream = false;
	string nshm ("--no-shmem");
	bool use_shmem = true;
	string blockloads ("--blocked-loads");
	bool blocked_loads = false;
	string halo ("--halo");
	map<string,tuple<int,int>> halo_dim;
	string gen_if ("--generate-if");
	bool generate_if = false;
	string prefetch_loads ("--prefetch");
	bool prefetch = false;
	string decompose_stmts ("--decompose");
	bool decompose = false;
	string retime_stmts ("--retime");
	bool retime = false;
	string fold_computations ("--fold-computation");
	bool fold = false;
	string linearize_accesses ("--linearize-accesses");
	bool linearize = false;
	string code_differencing ("--code-diff");
	bool code_diff = false;

	for (int i=2; i<argc; i++) {
		if (outfile.compare (argv[i]) == 0) {
			if (i == argc-1) 
				printf ("Missing output file name, using default out.cu\n");
			else 
				out_name = argv[++i];
		}
		else if (ndim.compare (argv[i]) == 0) {
			if (i == argc-1) 
				printf ("Missing problem dims, using defaults\n");
			else {
				string decls = argv[++i];
				while (decls.find (",") != string::npos) {
					size_t pos = decls.find (",");
					string decl = decls.substr(0,pos);
					size_t br = decl.find ("=");
					assert (br != string::npos);
					string n_id = decl.substr(0,br); 
					int n_dim = atoi (decl.substr(br+1).c_str());
					n_dims->delete_symbol (n_id);
					n_dims->push_symbol (n_id, n_dim);
					cout << "problem dim " << n_id << " = " << n_dim << endl;
					decls = decls.substr(pos+1);
				}
				// Extract the last
				if (!decls.empty ()) {
					size_t br = decls.find ("=");
					assert (br != string::npos);
					string n_id = decls.substr(0,br); 
					int n_dim = atoi (decls.substr(br+1).c_str());
					n_dims->delete_symbol (n_id);
					n_dims->push_symbol (n_id, n_dim);
					cout << "problem dim " << n_id << " = " << n_dim << endl;
				}
			}
		}
		else if (blockdim.compare (argv[i]) == 0) {
			if (i == argc-1) 
				printf ("Missing block dims, using defaults\n");
			else {
				string decls = argv[++i];
				while (decls.find (",") != string::npos) {
					size_t pos = decls.find (",");
					string decl = decls.substr(0,pos);
					size_t br = decl.find ("=");
					assert (br != string::npos);
					string block_id = decl.substr(0,br); 
					int block_dim = atoi (decl.substr(br+1).c_str());
					block_dims->delete_symbol (block_id);
					block_dims->push_symbol (block_id, block_dim);
					cout << "block dim " << block_id << " = " << block_dim << endl;
					decls = decls.substr(pos+1);
				}
				// Extract the last
				if (!decls.empty ()) {
					size_t br = decls.find ("=");
					assert (br != string::npos);
					string block_id = decls.substr(0,br); 
					int block_dim = atoi (decls.substr(br+1).c_str());
					block_dims->delete_symbol (block_id);
					block_dims->push_symbol (block_id, block_dim);
					cout << "block dim " << block_id << " = " << block_dim << endl;
				}
			}
		}
		else if (unroll.compare (argv[i]) == 0) {
			if (i == argc-1) 
				printf ("Missing unroll factors, using default 1 along each dimension\n");
			else {
				string decls = argv[++i];
				while (decls.find (",") != string::npos) {
					size_t pos = decls.find (",");
					string decl = decls.substr(0,pos);
					size_t br = decl.find ("=");
					assert (br != string::npos);
					string unroll_id = decl.substr(0,br); 
					int unroll_factor = atoi (decl.substr(br+1).c_str());
					assert (unroll_factor > 0 && "Unroll factor must be a natural number");
					unroll_decls->push_symbol (unroll_id, unroll_factor);
					cout << "Unroll factor " << unroll_id << " = " << unroll_factor << endl;
					decls = decls.substr(pos+1);
				}
				// Extract the last
				if (!decls.empty ()) {
					size_t br = decls.find ("=");
					assert (br != string::npos);
					string unroll_id = decls.substr(0,br); 
					int unroll_factor = atoi (decls.substr(br+1).c_str());
					assert (unroll_factor > 0 && "Unroll factor must be a natural number");
					unroll_decls->push_symbol (unroll_id, unroll_factor);
					cout << "Unroll factor " << unroll_id << " = " << unroll_factor << endl;
				}
			}
		}
		else if (streaming.compare (argv[i]) == 0) {
			if (i == argc-1)
				printf ("Missing streaming value, using default outermost iterator\n");
			else 
				stream_dim = argv[++i];
			stream = true;
		}
		else if (fstream.compare (argv[i]) == 0) {
			if (i == argc-1)
				printf ("Missing streaming value, using default outermost iterator\n");
			else 
				stream_dim = argv[++i];
			full_stream = true;
			stream = true;
		}
		else if (prefetch_loads.compare (argv[i]) == 0) {
			prefetch = true;
		}
		else if (decompose_stmts.compare (argv[i]) == 0) {
			decompose = true;
		}
		else if (retime_stmts.compare (argv[i]) == 0) {
			decompose = true;
			retime = true;
		}
		else if (fold_computations.compare (argv[i]) == 0) {
			decompose = true;
			fold = true;
		}
		else if (linearize_accesses.compare (argv[i]) == 0) {
			linearize = true;
		}
		else if (code_differencing.compare (argv[i]) == 0) {
			code_diff = true;
		}
		else if (nshm.compare (argv[i]) == 0) {
			use_shmem = false;
		}
		else if (gen_if.compare (argv[i]) == 0) {
			generate_if = true;
		}
		else if (blockloads.compare (argv[i]) == 0) {
			blocked_loads = true;
		}
		else if (halo.compare (argv[i]) == 0) {
			if (i == argc-1) 
				printf ("Missing halos, using default 0\n");
			else {
				string decls = argv[++i];
				while (decls.find (",") != string::npos) {
					size_t pos = decls.find (",");
					string decl = decls.substr(0,pos);
					size_t br = decl.find ("=");
					assert (br != string::npos);
					string halo_id = decl.substr(0,br); 
					string halo_factor = decl.substr(br+1);
					size_t tr = halo_factor.find (":");
					assert (tr != string::npos);
					int lo_halo = atoi (halo_factor.substr(0,tr).c_str());
					int hi_halo = atoi (halo_factor.substr(tr+1).c_str());
					assert (lo_halo <= 0 && "Low halo factor be <= 0");
					assert (hi_halo >= 0 && "High halo factor be >= 0");
					halo_dim[halo_id] = make_tuple (lo_halo, hi_halo);
					cout << "Halo dim for " << halo_id << " = (" << lo_halo << ", " << hi_halo << ")\n";
					decls = decls.substr(pos+1);
				}
				// Extract the last
				if (!decls.empty ()) {
					size_t br = decls.find ("=");
					assert (br != string::npos);
					string halo_id = decls.substr(0,br); 
					string halo_factor = decls.substr(br+1);
					size_t tr = halo_factor.find (":");
					assert (tr != string::npos);
					int lo_halo = atoi (halo_factor.substr(0,tr).c_str());
					int hi_halo = atoi (halo_factor.substr(tr+1).c_str());
					assert (lo_halo <= 0 && "Low halo factor be <= 0");
					assert (hi_halo >= 0 && "High halo factor be >= 0");
					halo_dim[halo_id] = make_tuple (lo_halo, hi_halo);
					cout << "Halo dim for " << halo_id << " = (" << lo_halo << ", " << hi_halo << ")\n";
				}
			}
		}
	}
	FILE *in = fopen (argv[1], "r");
	grammar::set_input (in);

	ofstream opt_out (out_name.c_str(), ofstream::out);
	grammar::parse ();
	codegen *sp_gen = new codegen (grammar::start, n_dims, block_dims, unroll_decls, full_stream, stream, stream_dim, use_shmem, blocked_loads, generate_if, halo_dim, prefetch, decompose, retime, fold, linearize, code_diff);
	stringstream opt;
	sp_gen->generate_code (opt);
	opt_out << opt.rdbuf ();
	opt_out.close ();
	fclose (in);
	return 0;
}
