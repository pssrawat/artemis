#include "codegen.hpp"
using namespace std;

string codegen::create_unified_stencil (void) {
	stringstream out;
	vector<funcDefn*> fd = start->get_func_defns ();
	for (auto f : fd) {
		// Print the parameters
		vector<string> param = f->get_parameters ();
		if (!param.empty ()) {
			out << "parameter ";
			for (vector<string>::iterator p=param.begin(); p!=param.end(); p++) {
				out << *p;
				if (p != prev(param.end()))
					out << ", ";
			}
			out << ";\n";
		}
		// Print the iterator
		vector<string> iters = f->get_iterators ();
		if (!iters.empty ()) {
			out << "iterator ";
			for (vector<string>::iterator i=iters.begin(); i!=iters.end(); i++) {
				out << *i;
				if (i != prev(iters.end())) 
					out << ", ";
			}
			out << ";\n";
		}
		map<string,DATA_TYPE> vd_map = f->get_var_decls ();
		if (!vd_map.empty ()) {
			for (map<string,DATA_TYPE>::iterator m=vd_map.begin(); m!=vd_map.end(); m++) {
				out << print_data_type (m->second) << m->first;
				out << ";\n";
			}
			out << "\n";
		}
		vector<arrayDecl*> ad = f->get_array_decls ();
		if (!ad.empty ()) {
			for (vector<arrayDecl*>::iterator a=ad.begin(); a!=ad.end(); a++) {
				string name = (*a)->get_array_name ();
				DATA_TYPE t = (*a)->get_array_type ();
				out << print_data_type (t) << name;
				vector<Range*> range = (*a)->get_array_range ();
				for (auto r : range) {
					exprNode *lo = r->get_lo_range ();
					stringstream lo_out;
					lo->print_node (lo_out);
					exprNode *hi = r->get_hi_range ();
					stringstream hi_out;
					hi->print_node (hi_out);
					out << "[" << lo_out.str() << ":" << hi_out.str() << "]";
				}
				out << ";\n";	 
			}
			out << "\n";
		}
		// Print the copyin list
		vector<string> copyin = f->get_copyins ();
		if (!copyin.empty ()) {
			out << "copyin ";
			for (vector<string>::iterator i=copyin.begin(); i!=copyin.end(); i++) {
				out << *i;
				if (i != prev(copyin.end())) 
					out << ", ";
			}
			out << ";\n";
		}
		// Print the constants 
		vector<string> consts = f->get_constants ();
		if (!consts.empty ()) {
			out << "constant ";
			for (vector<string>::iterator i=consts.begin(); i!=consts.end(); i++) {
				out << *i;
				if (i != prev(consts.end())) 
					out << ", ";
			}
			out << ";\n";
		}
		// Print the coefficients
		vector<string> coefs = f->get_coefficients ();
		if (!coefs.empty ()) {
			out << "coefficient ";
			for (vector<string>::iterator i=coefs.begin(); i!=coefs.end(); i++) {
				out << *i;
				if (i != prev(coefs.end())) 
					out << ", ";
			}
			out << ";\n";
		}
		// Now print each funcdefn separately
		map<string,stencilDefn*> sdefn = f->get_stencil_defns ();
		for (auto sd : sdefn) {
			string stencil_name = sd.first;
			stencilDefn *stencil = sd.second;
			vector<string> args = stencil->get_arg_list ();
			vector<stmtNode*> stmts = stencil->get_stmt_list ();
			out << "stencil " << stencil_name << " (";
			for (vector<string>::iterator a=args.begin(); a!=args.end(); a++) {
				out << *a;
				if (a != prev(args.end()))
					out << ", ";
			}
			out << ") {\n";
			// Print temporary variables
			map<string,DATA_TYPE> tvd_map = stencil->get_var_decls ();
			if (!tvd_map.empty ()) {
				// Print the var declarations
				for (map<string,DATA_TYPE>::iterator m=tvd_map.begin(); m!=tvd_map.end(); m++) {
					out << "\t" << print_data_type (m->second) << m->first << ";\n";	 
				}
			}
			// Print temporary array declarations
			vector<arrayDecl*> tad = stencil->get_array_decls ();
			if (!tad.empty ()) {
				for (vector<arrayDecl*>::iterator a=tad.begin(); a!=tad.end(); a++) {
					string name = (*a)->get_array_name ();
					DATA_TYPE t = (*a)->get_array_type ();
					out << "\t" << print_data_type (t) << name;
					vector<Range*> range = (*a)->get_array_range ();
					for (auto r : range) {
						exprNode *lo = r->get_lo_range ();
						stringstream lo_out;
						lo->print_node (lo_out);
						exprNode *hi = r->get_hi_range ();
						stringstream hi_out;
						hi->print_node (hi_out);
						out << "[" << lo_out.str() << ":" << hi_out.str() << "]";
					}
					out << ";\n";
				}
			}
			// Print each statement
			for (auto stmt : stmts) {
				stringstream lhs_out, rhs_out;
				exprNode *lhs = stmt->get_lhs_expr ();
				exprNode *rhs = stmt->get_rhs_expr ();
				STMT_OP op_type = stmt->get_op_type ();
				lhs->print_node (lhs_out);
				rhs->print_node (rhs_out);
				out << "\t" << lhs_out.str () << print_stmt_op (op_type) << rhs_out.str () << ";\n";
			}
			out << "}\n";
		}
		// Print the stencil call
		vector<stencilCall*> st_calls = f->get_stencil_calls ();
		for (auto sc : st_calls) {
			vector<Range *> dom = sc->get_domain ();
			string stencil_name = sc->get_name ();
			vector<string> args = sc->get_arg_list ();
			// Print the domain
			if (!dom.empty ()) {
				out << "[";
				for (vector<Range *>::iterator d=dom.begin(); d!=dom.end(); d++) {
					exprNode *lo = (*d)->get_lo_range ();
					exprNode *hi = (*d)->get_hi_range ();
					stringstream lo_out, hi_out;
					lo->print_node (lo_out);
					hi->print_node (hi_out);
					out << lo_out.str() << ":" << hi_out.str();
					if (d != prev(dom.end())) 
						out << ",";
				}
				out << "] : ";
			}
			out << stencil_name << " (";
			// Print the arguments
			for (vector<string>::iterator a=args.begin(); a!=args.end(); a++) {
				out << *a;
				if (a != prev(args.end()))
					out << ", ";
			}
			out << ");\n";
		}
		// Print the copyout values	  
		vector<string> copyout = f->get_copyouts ();
		if (!copyout.empty ()) {
			out << "copyout ";
			for (vector<string>::iterator o=copyout.begin(); o!=copyout.end(); o++) {
				out << *o;
				if (o != prev(copyout.end())) 
					out << ", ";
			}
			out << ";\n";
		}
		out << endl;
	}
	return out.str();
}

void codegen::print_stencil_details (void) {
	vector<funcDefn*> fd = start->get_func_defns ();
	for (auto f : fd) {
		// Print the parameters
		vector<string> param = f->get_parameters ();
		if (!param.empty ()) {
			cout << "\nparameter : (";
			for (vector<string>::iterator p=param.begin(); p!=param.end(); p++) {
				cout << *p;
				if (p != prev(param.end()))
					cout << ", ";
			}
			cout << ")\n";
		}
		// Print the iterator
		vector<string> iters = f->get_iterators ();
		if (!iters.empty ()) {
			cout << "iterator : (";
			for (vector<string>::iterator i=iters.begin(); i!=iters.end(); i++) {
				cout << *i;
				if (i != prev(iters.end())) 
					cout << ", ";
			}
			cout << ")\n";
		}
		// Print the constants 
		vector<string> consts = f->get_constants ();
		if (!consts.empty ()) {
			cout << "consant ";
			for (vector<string>::iterator i=consts.begin(); i!=consts.end(); i++) {
				cout << *i;
				if (i != prev(consts.end())) 
					cout << ", ";
			}
			cout << "\n";
		}
		// Print the coefficients
		vector<string> coefs = f->get_coefficients ();
		if (!coefs.empty ()) {
			cout << "coefficient : (";
			for (vector<string>::iterator i=coefs.begin(); i!=coefs.end(); i++) {
				cout << *i;
				if (i != prev(coefs.end())) 
					cout << ", ";
			}
			cout << ")\n";
		}
		// Print the unroll factors
		map<string,int> uf_map = f->get_unroll_decls ();
		if (!uf_map.empty ()) {
			// Print the var declarations
			cout << "unroll factor : (";
			for (map<string,int>::iterator m=uf_map.begin(); m!=uf_map.end(); m++) {
				cout << m->first << ":" << m->second;
				if (m != prev(uf_map.end()))
					cout << ", ";	 
			}
			cout << ")\n";
		}
		map<string,DATA_TYPE> vd_map = f->get_var_decls ();
		if (!vd_map.empty ()) {
			// Print the var declarations
			cout << "var decls : (";
			for (map<string,DATA_TYPE>::iterator m=vd_map.begin(); m!=vd_map.end(); m++) {
				cout << print_data_type (m->second) << m->first;
				if (m != prev(vd_map.end()))
					cout << ", ";	 
			}
			cout << ")\n";
		}
		map<string,DATA_TYPE> tvd_map = f->get_temp_var_decls ();
		if (!tvd_map.empty ()) {
			// Print the var declarations
			cout << "temporary var decls : (";
			for (map<string,DATA_TYPE>::iterator m=tvd_map.begin(); m!=tvd_map.end(); m++) {
				cout << print_data_type (m->second) << m->first;
				if (m != prev(tvd_map.end()))
					cout << ", ";	 
			}
			cout << ")\n";
		}
		vector<arrayDecl*> ad = f->get_array_decls ();
		if (!ad.empty ()) {
			// Print the array declarations
			cout << "array decls : (";
			for (vector<arrayDecl*>::iterator a=ad.begin(); a!=ad.end(); a++) {
				string name = (*a)->get_array_name ();
				DATA_TYPE t = (*a)->get_array_type ();
				cout << print_data_type (t) << name;
				vector<Range*> range = (*a)->get_array_range ();
				for (auto r : range) {
					exprNode *lo = r->get_lo_range ();
					stringstream lo_out;
					lo->print_node (lo_out);
					exprNode *hi = r->get_hi_range ();
					stringstream hi_out;
					hi->print_node (hi_out);
					cout << "[" << lo_out.str() << ":" << hi_out.str() << "]";
				}
				if (a != prev(ad.end()))
					cout << ", ";	 
			}
			cout << ")\n";
		}
		vector<arrayDecl*> tad = f->get_temp_array_decls ();
		if (!tad.empty ()) {
			// Print the array declarations
			cout << "temporary array decls : (";
			for (vector<arrayDecl*>::iterator a=tad.begin(); a!=tad.end(); a++) {
				string name = (*a)->get_array_name ();
				DATA_TYPE t = (*a)->get_array_type ();
				cout << print_data_type (t) << name;
				vector<Range*> range = (*a)->get_array_range ();
				for (auto r : range) {
					exprNode *lo = r->get_lo_range ();
					stringstream lo_out;
					lo->print_node (lo_out);
					exprNode *hi = r->get_hi_range ();
					stringstream hi_out;
					hi->print_node (hi_out);
					cout << "[" << lo_out.str() << ":" << hi_out.str() << "]";
				}
				if (a != prev(tad.end()))
					cout << ", ";	 
			}
			cout << ")\n";
		}
		// Print the copyin list
		vector<string> copyin = f->get_copyins ();
		if (!copyin.empty ()) {
			cout << "copy-in : (";
			for (vector<string>::iterator i=copyin.begin(); i!=copyin.end(); i++) {
				cout << *i;
				if (i != prev(copyin.end())) 
					cout << ", ";
			}
			cout << ")\n";
		}
		// Now print each funcdefn separately
		map<string,stencilDefn*> sdefn = f->get_stencil_defns ();
		for (auto sd : sdefn) {
			string stencil_name = sd.first;
			stencilDefn *stencil = sd.second;
			vector<string> args = stencil->get_arg_list ();
			vector<stmtNode*> stmts = stencil->get_stmt_list ();
			cout << "stencil " << stencil_name << " (";
			for (vector<string>::iterator a=args.begin(); a!=args.end(); a++) {
				cout << *a;
				if (a != prev(args.end()))
					cout << ", ";
			}
			cout << ") {\n";
			// Print temporary variables
			map<string,DATA_TYPE> tvd_map = stencil->get_var_decls ();
			if (!tvd_map.empty ()) {
				// Print the var declarations
				cout << "\ttemporary var decls : (";
				for (map<string,DATA_TYPE>::iterator m=tvd_map.begin(); m!=tvd_map.end(); m++) {
					cout << print_data_type (m->second) << m->first;
					if (m != prev(tvd_map.end()))
						cout << ", ";	 
				}
				cout << ")\n";
			}
			// Print temporary array declarations
			vector<arrayDecl*> tad = stencil->get_array_decls ();
			if (!tad.empty ()) {
				cout << "\ttemporary array decls : (";
				for (vector<arrayDecl*>::iterator a=tad.begin(); a!=tad.end(); a++) {
					string name = (*a)->get_array_name ();
					DATA_TYPE t = (*a)->get_array_type ();
					cout << print_data_type (t) << name;
					vector<Range*> range = (*a)->get_array_range ();
					for (auto r : range) {
						exprNode *lo = r->get_lo_range ();
						stringstream lo_out;
						lo->print_node (lo_out);
						exprNode *hi = r->get_hi_range ();
						stringstream hi_out;
						hi->print_node (hi_out);
						cout << "[" << lo_out.str() << ":" << hi_out.str() << "]";
					}
					if (a != prev(tad.end()))
						cout << ", ";	 
				}
				cout << ")\n";
			}
			// Print each statement
			for (auto stmt : stmts) {
				stringstream lhs_out, rhs_out;
				exprNode *lhs = stmt->get_lhs_expr ();
				exprNode *rhs = stmt->get_rhs_expr ();
				STMT_OP op_type = stmt->get_op_type ();
				lhs->print_node (lhs_out);
				rhs->print_node (rhs_out);
				cout << "\t" << lhs_out.str () << print_stmt_op (op_type) << rhs_out.str () << ";\n";
			}
			cout << "}\n";
		}
		// Print the stencil call
		vector<stencilCall*> st_calls = f->get_stencil_calls ();
		for (auto sc : st_calls) {
			vector<Range *> dom = sc->get_domain ();
			string stencil_name = sc->get_name ();
			vector<string> args = sc->get_arg_list ();
			// Print the domain
			if (!dom.empty ()) {
				cout << "[";
				for (vector<Range *>::iterator d=dom.begin(); d!=dom.end(); d++) {
					exprNode *lo = (*d)->get_lo_range ();
					exprNode *hi = (*d)->get_hi_range ();
					stringstream lo_out, hi_out;
					lo->print_node (lo_out);
					hi->print_node (hi_out);
					cout << lo_out.str() << ":" << hi_out.str();
					if (d != prev(dom.end())) 
						cout << ",";
				}
				cout << "] : ";
			}
			cout << stencil_name << " (";
			// Print the arguments
			for (vector<string>::iterator a=args.begin(); a!=args.end(); a++) {
				cout << *a;
				if (a != prev(args.end()))
					cout << ", ";
			}
			cout << ");\n";
		}
		// Print the copyout values	  
		vector<string> copyout = f->get_copyouts ();
		if (!copyout.empty ()) {
			cout << "copy-out : (";
			for (vector<string>::iterator o=copyout.begin(); o!=copyout.end(); o++) {
				cout << *o;
				if (o != prev(copyout.end())) 
					cout << ", ";
			}
			cout << ")\n";
		}

		cout << endl;
	}
}

// Header of the file, that defines routine functions.
void codegen::generate_header (stringstream &header) {
	header << "#include <stdio.h>\n";
	header << "#include \"cuda.h\"\n";
	header << "#define max(x,y)    ((x) > (y) ? (x) : (y))\n";
	header << "#define min(x,y)    ((x) < (y) ? (x) : (y))\n";
	//header << "#define min3(x,y,z) ((x) < (y) ? ((x) < (z) ? (x) : (z)) : ((y) < (z) ? (y) : (z)))\n";
	header << "#define ceil(a,b)   ((a) % (b) == 0 ? (a) / (b) : ((a) / (b)) + 1)\n\n";

	// Define the headers
	map<string,int> n_map = n_dims->get_symbol_map ();
	for (auto n : n_map) {
		header << "#define " << n.first << n.first << " " << n.second << "\n";
	}
	if (!n_map.empty ())
		header << "\n";

	map<string,int> block_map = block_dims->get_symbol_map ();
	for (auto b : block_map) {
		header << "#define b" << b.first << " " << b.second << "\n";
	}
	if (!block_map.empty ())
		header << "\n";

	string indent = "\t";
	header << "void check_error (const char* message) {\n";
	header << indent << "cudaError_t error = cudaGetLastError ();\n";
	header << indent << "if (error != cudaSuccess) {\n";
	header << indent << indent << "printf (\"CUDA error : %s, %s\\n\", message, cudaGetErrorString (error));\n";
	header << indent << indent << "exit(-1);\n";
	header << indent << "}\n";
	header << "}\n";
}

void codegen::generate_code (stringstream &opt) {
	// Set various parameters
	start->set_iterators ();
	start->set_parameters ();
	start->set_allocins ();
	start->set_copyins ();
	start->set_constants ();
	start->set_copyouts ();
	start->set_block_dims (block_dims);
	start->set_halo_dims (halo_dims);
	start->set_load_style (blocked_loads);
	if (stream) 
		start->set_streaming_dimension (full_stream, stream_dim);
	start->set_unroll_decls (unroll_decls);
	start->set_use_shmem (use_shmem);
	start->set_loop_gen_strategy (generate_if);
	start->set_loop_prefetch_strategy (prefetch);
	start->set_linearize_accesses (linearize_accesses);
	start->set_gen_code_diff (code_diff);
	// Verify the correctness
	start->verify_correctness ();
	if (DEBUG) 
		print_stencil_details ();
	//// Create a unified stencil
	//ofstream uf_out ("unified-in.idsl", ofstream::out);
	//string unified_stencil = create_unified_stencil ();
	//uf_out << unified_stencil;
	//uf_out.close ();	
	start->map_args_to_parameters ();
	start->decompose_statements (decompose, retime, fold_computations);
	start->compute_dependences ();
	start->create_trivial_stencil_versions ();
	generate_header (opt);
	start->generate_code (opt);
}
