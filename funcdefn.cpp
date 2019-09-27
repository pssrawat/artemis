#include "funcdefn.hpp"
using namespace std;

bool stmtNode::is_scalar_stmt (void) {
	bool ret = true;
	ret &= lhs_node->is_scalar_expr ();
	ret &= rhs_node->is_scalar_expr ();
	return ret;
}

DATA_TYPE stmtNode::infer_stmt_type (void) {
	DATA_TYPE ret = BOOL;
	lhs_node->infer_expr_type (ret);
	rhs_node->infer_expr_type (ret);
	return ret;			
}

void stmtNode::retime_statement (string stream_dim, int offset) {
	stringstream before, after;
	if (DEBUG) {
		before << "Before retiming, statement : ";
		lhs_node->print_node (before);
		before << print_stmt_op (op_type);
		rhs_node->print_node (before);
		cout << before.str() << endl;
	}
	lhs_node = lhs_node->retime_node (stream_dim, offset);
	rhs_node = rhs_node->retime_node (stream_dim, offset);
	if (DEBUG) {
		after << "After retiming, statement : ";
		lhs_node->print_node (after);
		after << print_stmt_op (op_type);
		rhs_node->print_node (after);
		cout << after.str() << endl;
	}
}

void stmtNode::accessed_arrays (set<string> &arrays) {
	//lhs_node->accessed_arrays (arrays);
	rhs_node->accessed_arrays (arrays);	
}

void stmtNode::decompose_statements (vector<stmtNode*> &substmts, bool fold_computations, map<string, set<int>> &array_use_map, vector<tuple<string,string>> &sym_initializations, vector<stmtNode*> &init_stmts) {
	exprNode *dist_rhs = rhs_node->distribute_node ();
	dist_rhs->remove_redundant_nesting ();
	stringstream temp;
	dist_rhs->print_node (temp);
	if (DEBUG) 
		cout << "distributed rhs = " << temp.str () << endl;
	vector<tuple<exprNode*,STMT_OP,exprNode*>> subexprs;
	if (op_type == ST_EQ || op_type == ST_PLUSEQ || op_type == ST_MINUSEQ) {
		bool flip = (op_type == ST_MINUSEQ) ? true : false;
		dist_rhs->decompose_node (lhs_node, subexprs, flip);
		if (DEBUG) {
			cout << "decomposed stmt = " << endl;
			stringstream decomp;
			for (auto s : subexprs) {
				get<0>(s)->print_node (decomp);
				decomp << print_stmt_op (get<1>(s));
				get<2>(s)->print_node (decomp);
				decomp << "\n"; 
			}
			cout << decomp.str();
		}
	}
	if (subexprs.empty())
		subexprs.push_back (make_tuple (lhs_node, op_type, dist_rhs));
	// Now create the sub statements
	for (auto s : subexprs) {
		substmts.push_back (new stmtNode (get<1>(s), get<0>(s), get<2>(s), stmt_domain));
	}
	if (fold_computations) {
		cout << "FOLD COMPUTATIONS IS TRUE\n";
		fold_symmetric_computations (substmts, array_use_map, sym_initializations, init_stmts);
	}
}

void stmtNode::fold_symmetric_computations (vector<stmtNode*> &substmts, map<string, set<int>> &array_use_map, vector<tuple<string,string>> &sym_initializations, vector<stmtNode*> &init_stmts) {
	//Try to find symmetry in the substatements. This is experimental.
	//Symmetry can be found only if there are at least two accesses that are unique to this statement
	int unique_accesses = 0;
	for (auto a : array_use_map) {
		if (a.second.size() != 1)
			continue;
		for (auto b : a.second) {
			if (b == stmt_num)
				unique_accesses++;
		}
	}
	bool explore_symmetry = (unique_accesses >= 2 && substmts.size() >= 2);
	if (substmts.size() == 2) {
		exprNode *rhs1 = substmts[0]->get_rhs_expr();
		exprNode *rhs2 = substmts[1]->get_rhs_expr();
		explore_symmetry &= !(rhs1->is_shiftvec_node() && rhs2->is_shiftvec_node());
	}
	if (explore_symmetry) {
		//Right now, the algorithm is based on string matching
		vector<string> stringified_substmt_rhs;
		vector<set<string>> substmt_accessed_arrays;
		map<string,set<int>> substmt_array_use_map;
		int substmt_num = 0;
		for (auto s : substmts) {
			stringstream rhs_out;
			exprNode *rhs = s->get_rhs_expr ();
			rhs->print_node (rhs_out);
			stringified_substmt_rhs.push_back (rhs_out.str());
			//Compute the accesses for each substatement
			set<string> arrays;
			s->accessed_arrays (arrays);
			//Retain only arrays that are unique to this statement
			for (set<string>::iterator it=arrays.begin(); it!=arrays.end();) {
				if (array_use_map[*it].size() == 1)
					it++;
				else
					it = arrays.erase(it);
			}
			substmt_accessed_arrays.push_back (arrays);
			for (auto a : arrays) {
				if (substmt_array_use_map.find (a) != substmt_array_use_map.end())
					substmt_array_use_map[a].insert (substmt_num);
				else {
					set<int> st; 
					st.insert (substmt_num);
					substmt_array_use_map[a] = st;
				}
			}
			substmt_num++;
			cout << "stringified substmt = " << rhs_out.str() << "\n";
			cout << "Accessed arrays = ";
			for (auto a : arrays) 
				cout << a << " ";
			cout << "\n";
		}
		if (DEBUG) {
			cout << "Substmt accessed array map :\n";
			for (auto a : substmt_array_use_map) {
				cout << a.first << " : (";
				for (auto b : a.second) {
					cout << b << " ";
				}
				cout << ")\n";
			}
		}
		//Now create disjoint subsets of the substatements based on the array use
		//These are the limitations: Right now, we assume that only two disjoint
		//groups can be created, based on two arrays
		set<int> substmt_union;
		for (auto a : substmt_array_use_map) {
			substmt_union.insert (a.second.begin(), a.second.end());
		}
		if ((substmt_array_use_map.size() == 2) && ((int)substmt_union.size() == substmt_num)) {
			//Try to make pairs
			set<OP_TYPE> final_op;
			set<int> domain, range;
			string domain_arr, range_arr;
			int index = 0;
			for (auto a : substmt_array_use_map) {
				if (index == 0) {
					domain_arr = trim_string (a.first, "[");
					domain = a.second;
				}
				else if (index == 1) {
					range_arr = trim_string (a.first, "[");
					range = a.second;
				}
				index++;
			}
			bool symmetrical = (domain.size() == range.size());
			if (symmetrical) {
				set<int> range_mirror;
				for (auto d : domain) {
					string domain_expr = stringified_substmt_rhs[d];
					bool found = false; 
					for (set<int>::iterator r=range.begin(); r!=range.end(); r++) {
						if (range_mirror.find (*r) != range_mirror.end())
							continue;
						string range_expr = stringified_substmt_rhs[*r];
						//Replace all occurrences of range_arr with domain_arr
						boost::replace_all (range_expr, range_arr+"[", domain_arr+"[");	
						if (range_expr.compare(domain_expr) == 0) {
							cout << "Found same strings for domain " << d << " and range " << *r << endl;
							STMT_OP domain_op = substmts[d]->get_op_type(); 
							STMT_OP range_op = substmts[*r]->get_op_type(); 
							if ((domain_op == ST_EQ && range_op == ST_MINUSEQ) ||
									(domain_op == ST_PLUSEQ && range_op == ST_MINUSEQ) ||
									(domain_op == ST_MINUSEQ && range_op == ST_PLUSEQ))
								final_op.insert (T_MINUS);
							else if ((domain_op == ST_EQ && range_op == ST_PLUSEQ) ||
									(domain_op == ST_MINUSEQ && range_op == ST_MINUSEQ) ||
									(domain_op == ST_PLUSEQ && range_op == ST_PLUSEQ))
								final_op.insert (T_PLUS);
							else
								symmetrical = false;
							found = true;
							range_mirror.insert (*r);	
						}
					}
					symmetrical &= found && (final_op.size() == 1);
					if (!symmetrical) break;
				}
			}
			if (symmetrical) {
				//Get the dimensionality and subscripts of the domain array
				cout << "SYMMETRICAL\n";
				bool same_subscripts = true;
				vector<string> iters;
				DATA_TYPE type;	
				for (auto d : domain) {
					exprNode *rhs = substmts[d]->get_rhs_expr();
					same_subscripts &= rhs->same_subscripts_for_array (domain_arr, iters, type); 
				}
				if (same_subscripts) {
					//Make changes to the IR: retain only the substatements in domain
					vector<stmtNode*> dom_stmts; 
					for (auto d : domain) {
						dom_stmts.push_back (substmts[d]);
					}
					substmts.clear(); 
					substmts = dom_stmts;
					//Replace all the occurrences of domain_arr with a new array
					string sym_arr = "sym_" + domain_arr;
					for (auto s : substmts) {
						exprNode *rhs = s->get_rhs_expr();
						rhs->replace_array_name (domain_arr, sym_arr);
					}
					//Add initialization for the new array
					vector<exprNode*> index;
					for (auto it : iters) {
						if (all_of(it.begin(), it.end(), ::isdigit)) {
							int value = atoi(it.c_str());
							exprNode *idx = new datatypeNode<int>(value, INT);
							index.push_back (idx);
						}
						else {
							exprNode *idx = new idNode (it, type, false);
							index.push_back (idx);
						}
					}
					exprNode *lhs = new shiftvecNode (sym_arr, type, false);
					dynamic_cast<shiftvecNode*>(lhs)->set_indices (index);
					exprNode *rhs1 = new shiftvecNode (domain_arr, type, false);
					dynamic_cast<shiftvecNode*>(rhs1)->set_indices (index);
					exprNode *rhs2 = new shiftvecNode (range_arr, type, false);
					dynamic_cast<shiftvecNode*>(rhs2)->set_indices (index);
					exprNode *rhs = new binaryNode(*(final_op.begin()), rhs1, rhs2);
					stmtNode *init_stmt = new stmtNode (ST_EQ, lhs, rhs);
					//Insert initialization to substmts 
					//substmts.insert (substmts.begin(), init_stmt);
					init_stmts.push_back (init_stmt);
					//Mark the new sym array for declaration
					sym_initializations.push_back (make_tuple(domain_arr, sym_arr));	
				}
			}
		}
	}
}

bool stencilDefn::halo_exists (void) {
	bool halo_exists = false;
	for (auto iter : iterators) {
		int halo_lo = (halo_dims.find (iter) != halo_dims.end()) ? get<0>(halo_dims[iter]) : 0;
		int halo_hi = (halo_dims.find (iter) != halo_dims.end()) ? get<1>(halo_dims[iter]) : 0;
		halo_exists |= (halo_lo != 0 || halo_hi != 0);
	}
	return halo_exists;
}

void stencilDefn::set_halo_dims (map<string,tuple<int,int>> hd) {
	halo_dims = hd;
	// Fill in the missing values
	for (auto iter : get_iterators ()) {
		if (halo_dims.find (iter) == halo_dims.end ()) 
			halo_dims[iter] = make_tuple (0, 0);
		// Remove the halo along the streaming dimension
		else if (stream && iter.compare (stream_dim) == 0) 
			halo_dims[iter] == make_tuple (0, 0);
	}
	//// Make the halo along coalescing dimension 0
	//halo_dims[(get_iterators()).back()] = make_tuple (0, 0);
}

void stencilDefn::reset_halo_dims (void) {
	for (auto iter : get_iterators ()) {
		halo_dims[iter] = make_tuple (0, 0);
	}
}

void stencilDefn::set_loop_gen_strategy (bool b) {
	//for (auto hd : halo_dims) {
	//	b &= (get<0>(hd.second) == 0 && get<1>(hd.second) == 0);	
	//}
	generate_if = b;
}

void stencilDefn::set_loop_prefetch_strategy (bool p) {
	prefetch = p;
}

void stencilDefn::set_load_style (bool b) {
	//bool no_halo = true;
	//for (auto hd : halo_dims) {
	//	no_halo &= (get<0>(hd.second) == 0 && get<1>(hd.second) == 0);	
	//}
	blocked_loads = b;
}

void stencilDefn::set_unroll_decls (symtabTemplate <int> *u) {
	unroll_decls = u;
	//Fill in the missing values
	for (auto iter: get_iterators ()) {
		if (!(unroll_decls->symbol_present (iter))) {
			unroll_decls->push_symbol (iter, 1);
		}
		// Remove any unrolling along the streaming dimension
		if (stream && iter.compare (stream_dim) == 0) {
			int uf = unroll_decls->find_symbol (iter);
			stream_uf = uf > 1 ? uf : stream_uf;
			unroll_decls->delete_symbol (iter);
			unroll_decls->push_symbol (iter, 1);
		}
	}
}

void stencilDefn::unroll_stmts (void) {
	map<string,int> udecls = get_unroll_decls ();
	for (auto u : udecls) {
		map<string,int> scalar_version_map;
		stmtList *new_stmts = new stmtList ();
		for (int val=1; val<u.second; val++) {
			for (auto stmt : stmt_list->get_stmt_list ()) {
				exprNode *lhs = stmt->get_lhs_expr ();
				exprNode *rhs = stmt->get_rhs_expr ();
				STMT_OP stmt_type = stmt->get_op_type ();
				// Generate unrolled statements
				exprNode *unroll_lhs = lhs->unroll_expr (u.first, val, scalar_version_map, true);
				exprNode *unroll_rhs = rhs->unroll_expr (u.first, val, scalar_version_map, false);
				new_stmts->push_stmt (new stmtNode (stmt_type, unroll_lhs, unroll_rhs, stmt->get_stmt_domain()));
			}
		}
		stmt_list->push_stmt (new_stmts->get_stmt_list ());
		// Add the new variables to var_decls
		for (auto svm : scalar_version_map) {
			string var_name = svm.first;
			assert (is_var_decl (var_name));
			DATA_TYPE type = get_var_type (var_name);
			for (int val = 1; val <svm.second; val++) {
				string new_name = var_name + "_" + to_string (val);
				push_var_decl (new_name, type);
			}
		}
	}
}

void stencilDefn::set_data_type (void) {
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	map<string, DATA_TYPE> data_types;
	for (auto ad : array_decls) {
		data_types[ad->get_array_name()] = ad->get_array_type ();	
	}
	for (auto vd : var_decls->get_symbol_map ()) {
		data_types[vd.first] = vd.second;
	}
	for (auto vd : var_init_decls) {
		data_types[get<1>(vd)] = get<0>(vd);
	} 
	for (auto stmt : stmts) {
		exprNode *lhs = stmt->get_lhs_expr ();
		exprNode *rhs = stmt->get_rhs_expr ();
		lhs->set_data_type (data_types);
		rhs->set_data_type (data_types);
	}
}

void stencilDefn::print_stencil_body (string stencil_name, map<int,vector<int>> &clusters, stringstream &out) {
	vector<string> args = get_arg_list ();
	vector<stmtNode*> stmts = get_stmt_list ();
	int cluster_num = 0;
	for (map<int,vector<int>>::iterator c=clusters.begin(); c!=clusters.end(); c++,cluster_num++) {
		vector<int> cluster_stmts = c->second;
		cluster_stmts.push_back (c->first);
		// Get the accesess in the statements
		set<string> accesses;
		for (auto stmt : stmts) {
			int stmt_num = stmt->get_stmt_num ();
			if (find (cluster_stmts.begin(), cluster_stmts.end(), stmt_num) == cluster_stmts.end())
				continue;	
			stringstream lhs_out, rhs_out;
			exprNode *lhs = stmt->get_lhs_expr ();
			exprNode *rhs = stmt->get_rhs_expr ();
			lhs->collect_accesses (accesses);
			rhs->collect_accesses (accesses);
		}
		out << "\nstencil " << stencil_name;
		if (clusters.size() > 0)
			out << cluster_num;
		out << " (";
		bool first = true;
		for (vector<string>::iterator a=args.begin(); a!=args.end(); a++) {
			if (accesses.find (*a) == accesses.end())
				continue;
			if (!first)
				out << ", ";
			out << *a;
			first = false;
		}
		out << ") {\n";
		// Print temporary variables
		map<string,DATA_TYPE> tvd_map = get_var_decls ();
		if (!tvd_map.empty ()) {
			for (map<string,DATA_TYPE>::iterator m=tvd_map.begin(); m!=tvd_map.end(); m++) {
				if (accesses.find (m->first) == accesses.end())
					continue;	
				out << "\t" << print_data_type (m->second) << m->first << ";\n";	 
			}
		}
		// Print temporary array declarations
		vector<arrayDecl*> tad = get_array_decls ();
		if (!tad.empty ()) {
			for (vector<arrayDecl*>::iterator a=tad.begin(); a!=tad.end(); a++) {
				string name = (*a)->get_array_name ();
				if (accesses.find (name) == accesses.end())
					continue;	
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
		// Print temporary variables with initializations
		for (auto vd : var_init_decls) {
			if (accesses.find (get<1>(vd)) == accesses.end())
				continue;	
			out << indent << print_data_type (get<0>(vd));
			out << get<1>(vd); 
			out << " = ";
			(get<2>(vd))->print_node (out);
			out << ";\n";   
		}
		// Print the shared memory placements
		vector<string> shmem_decls = get_shmem_decl ();	
		if (!shmem_decls.empty()) {
			string shmem_list;
			bool first = true;
			for (vector<string>::iterator s=shmem_decls.begin(); s!=shmem_decls.end(); s++) {
				if (accesses.find (*s) == accesses.end())
					continue;
				if (!first) 
					shmem_list = shmem_list + ", ";
				shmem_list = shmem_list + *s;
				first = false;
			}
			if (!shmem_list.empty ()) {
				out << indent << "shmem ";
				out << shmem_list;
				out << ";\n";
			}
		}
		// Print the no-shared memory placements
		vector<string> noshmem_decls = get_noshmem_decl ();	
		if (!noshmem_decls.empty()) {
			string noshmem_list;
			bool first = true;
			for (vector<string>::iterator s=noshmem_decls.begin(); s!=noshmem_decls.end(); s++) {
				if (accesses.find (*s) == accesses.end())
					continue;
				if (!first) 
					noshmem_list = noshmem_list + ", ";
				noshmem_list = noshmem_list + *s;
				first = false;
			}
			if (!noshmem_list.empty ()) {
				out << indent << "no-shmem ";
				out << noshmem_list;
				out << ";\n";
			}
		}
		// Print the global memory placements
		vector<string> gmem_decls = get_gmem_decl ();	
		if (!gmem_decls.empty()) {
			string gmem_list;
			bool first = true;
			for (vector<string>::iterator s=gmem_decls.begin(); s!=gmem_decls.end(); s++) {
				if (accesses.find (*s) == accesses.end())
					continue;
				if (!first) 
					gmem_list = gmem_list + ", ";
				gmem_list = gmem_list + *s;
				first = false;
			}
			if (!gmem_list.empty ()) {
				out << indent << "gmem ";
				out << gmem_list;
				out << ";\n";
			}
		}

		// Print each statement
		for (auto stmt : stmts) {
			int stmt_num = stmt->get_stmt_num ();
			if (find (cluster_stmts.begin(), cluster_stmts.end(), stmt_num) == cluster_stmts.end())
				continue;	
			stringstream lhs_out, rhs_out;
			exprNode *lhs = stmt->get_lhs_expr ();
			exprNode *rhs = stmt->get_rhs_expr ();
			STMT_OP op_type = stmt->get_op_type ();
			lhs->print_node (lhs_out);
			rhs->print_node (rhs_out);
			out << indent << lhs_out.str () << print_stmt_op (op_type) << rhs_out.str () << ";\n";
		}
		out << "}\n";
	}
}

void stencilDefn::identify_separable_clusters (string name, map<int,vector<int>> &clusters) {
	map<int, vector<int>> unified_dependences;
	unify_dependences (unified_dependences);
	if (DEBUG) {
		cout << "\nUnified dependence graph for stencil " << name << " : " << endl;
		if (!unified_dependences.empty ()) {
			for (auto d : unified_dependences) {
				cout << "\t" << d.first << " - ";
				vector<int> rhs = d.second;
				for (auto s : d.second) 
					cout << s << " ";
				cout << endl;
			}
		}
	}
	// Are there multiple output statements that are not source?
	vector<int> sinks, dep_dest, dep_src;
	for (auto d : unified_dependences) {
		dep_dest.push_back (d.first);
		dep_src.insert (dep_src.end(), (d.second).begin(), (d.second).end());	
	}
	for (auto d : dep_dest) {
		if (find (dep_src.begin(), dep_src.end(), d) == dep_src.end())
			sinks.push_back (d);
	}
	if (DEBUG) {
		cout << "Independent statements for stencil " << name << " : ";
		for (auto o : sinks)
			cout << o << " ";
		cout << endl;
	}
	map<int, vector<int>> expanded_deps;
	expanded_deps.insert (unified_dependences.begin(), unified_dependences.end());
	for (auto d : unified_dependences) {
		int key = d.first;
		for (auto e : expanded_deps) {
			vector<int> &val = e.second;
			if (find (val.begin(), val.end(), key) != val.end()) {
				//val.erase (remove(val.begin(), val.end(), key), val.end());
				for (auto f : d.second) {
					if (find (val.begin(), val.end(), f) == val.end())
						val.push_back (f);
				}
			}
			expanded_deps[e.first] = val;
		}
	}
	for (auto s : sinks) {
		clusters[s] = expanded_deps[s];
	}

	// If there are two clusers such that they have the same 
	// array LHS, and one of them is a +=, then merge them
	map<int, vector<tuple<string,STMT_OP>>> lhs_op_map;
	vector<stmtNode*> orig_stmts = stmt_list->get_stmt_list ();
	for (auto c : clusters) {
		vector<tuple<string,STMT_OP>> lhs_acc;
		vector<int> stmt_vec = c.second;
		stmt_vec.push_back (c.first);
		for (auto p : stmt_vec) {
			string lhs_name = (orig_stmts[p]->get_lhs_expr())->get_name ();
			if (is_array_decl (lhs_name)) {
				STMT_OP op_type = orig_stmts[p]->get_op_type ();
				tuple<string,STMT_OP> tp = make_tuple (lhs_name, op_type);
				if (find (lhs_acc.begin(), lhs_acc.end(), tp) == lhs_acc.end())
					lhs_acc.push_back (tp);
			}
		}
		lhs_op_map[c.first] = lhs_acc;
	}

	for (map<int,vector<int>>::iterator it=clusters.begin(); it!=clusters.end(); it++) {
		vector<tuple<string,STMT_OP>> it_vec = lhs_op_map[it->first];
		for (map<int,vector<int>>::iterator jt=next(it); jt!=clusters.end(); jt++) {
			vector<tuple<string,STMT_OP>> jt_vec = lhs_op_map[jt->first];
			// Check if it and jt have the same lhs with one (or both) +=. Then they can't be separated
			bool merge = false;
			for (auto a : it_vec) {
				string a_lhs = get<0>(a);
				STMT_OP a_op = get<1>(a);
				for (auto b : jt_vec) {
					string b_lhs = get<0>(b);
					STMT_OP b_op = get<1>(b);
					if ((a_lhs.compare(b_lhs)==0) && ((a_op != ST_EQ) || (b_op != ST_EQ))) 
						merge = true;	
				}
			}
			if (merge) {
				// Put jt_vec into it_vec
				for (auto b : jt_vec) {
					if (find (it_vec.begin(), it_vec.end(), b) == it_vec.end())
						it_vec.push_back (b);
				}
				lhs_op_map[it->first] = it_vec;
				// Mere jt and it in clusters
				for (auto rhs : jt->second) {
					if (find ((it->second).begin(), (it->second).end(), rhs) == (it->second).end())
						(it->second).push_back (rhs);
				}
				int lhs = jt->first;
				if (find ((it->second).begin(), (it->second).end(), lhs) == (it->second).end())
					(it->second).push_back (lhs);
				jt = clusters.erase (jt); 
			}
		}
	}

	if (DEBUG) {
		cout << "Clusters for stencil " << name << " :\n";
		for (auto c : clusters) {
			cout << "\t" << c.first << " : ";
			for (auto l : c.second) {
				cout << l << " ";
			}
			cout << endl;
		}
	}
}

void stencilDefn::print_stencil_defn (string stencil_name) {
	cout << "\nAfter offsetting planes, stencil " << stencil_name << "{\n";
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	for (auto st: stmts) {
		for (auto stmt : decomposed_stmts[st->get_stmt_num()]) {
			stringstream lhs_out, rhs_out;
			exprNode *lhs = stmt->get_lhs_expr ();
			exprNode *rhs = stmt->get_rhs_expr ();
			STMT_OP op_type = stmt->get_op_type ();
			lhs->print_node (lhs_out);
			rhs->print_node (rhs_out);
			cout << "\t" << lhs_out.str () << print_stmt_op (op_type) << rhs_out.str () << ";\n";
		}
	}
	cout << "}\n";
}

void stencilDefn::compute_hull (vector<Range*> &initial_domain, vector<int> pointwise_occurrences) {
	// Map from array name to its hull
	map<string, vector<Range*>> hull_map;
	output_hull = vector<Range*> (initial_domain);

	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	// Go through the statements and compute hull for each
	for (auto st : stmts) {
		int stmt_num = st->get_stmt_num ();
		vector<Range*> lhs_hull = vector<Range*> (initial_domain);
		vector<Range*> rhs_hull = vector<Range*> (initial_domain);
		string lhs_name = (st->get_lhs_expr())->get_name ();
                // Get the offset if the accesses along stream dimension need amendment
                int offset = (stream && (plane_offset.find(stmt_num) != plane_offset.end ())) ? plane_offset[stmt_num] : 0;
                                
		for (vector<stmtNode*>::iterator it=decomposed_stmts[stmt_num].begin(); it!=decomposed_stmts[stmt_num].end(); it++) {
			exprNode *lhs = (*it)->get_lhs_expr ();
			exprNode *rhs = (*it)->get_rhs_expr ();
			rhs->compute_hull (hull_map, rhs_hull, initial_domain, stream, stream_dim, offset);
			if (!retiming_feasible && (*it)->get_op_type() != ST_EQ) 
				lhs->compute_hull (hull_map, rhs_hull, initial_domain, stream, stream_dim, offset);

			// Intersect lhs_hull with the statement domain if non empty
			vector<Range*> initial_stmt_domain = (*it)->get_stmt_domain ();
			if (!initial_stmt_domain.empty ()) {
				int pos = 0;
				vector<Range*>::iterator b_dom = initial_stmt_domain.begin();
				for (vector<Range*>::iterator a_dom=lhs_hull.begin(); a_dom!=lhs_hull.end(); a_dom++,b_dom++,pos++) {
					// Intersection of lo_a and lo_b;
					exprNode *lo_a = (*a_dom)->get_lo_range ();
					exprNode *lo_b = (*b_dom)->get_lo_range ();
					string lo_a_id = "", lo_b_id = "";
					int lo_a_val = 0, lo_b_val = 0;
					lo_a->decompose_access_fsm (lo_a_id, lo_a_val);
					lo_b->decompose_access_fsm (lo_b_id, lo_b_val);
					assert (lo_a_id.compare (lo_b_id) == 0);
					exprNode *lo = (lo_a_val >= lo_b_val) ? lo_a : lo_b; 
					// Intersection of hi_a and hi_b;
					exprNode *hi_a = (*a_dom)->get_hi_range ();
					exprNode *hi_b = (*b_dom)->get_hi_range ();
					string hi_a_id = "", hi_b_id = "";	
					int hi_a_val = 0, hi_b_val = 0;
					hi_a->decompose_access_fsm (hi_a_id, hi_a_val);
					hi_b->decompose_access_fsm (hi_b_id, hi_b_val);
					assert (hi_a_id.compare (hi_b_id) == 0);
					exprNode *hi = (hi_a_val <= hi_b_val) ? hi_a : hi_b; 	
					lhs_hull[pos] = new Range (lo, hi);
				}
			}

			map<string, vector<Range*>> empty_map;
			if (retiming_feasible) {
				empty_map[lhs_name] = vector<Range*> (rhs_hull);
			}
			lhs->compute_hull (empty_map, lhs_hull, initial_domain, stream, stream_dim, offset);

			// Intersect lhs_hull with rhs_hull
			int pos = 0;
			vector<Range*>::iterator b_dom = rhs_hull.begin();
			for (vector<Range*>::iterator a_dom=lhs_hull.begin(); a_dom!=lhs_hull.end(); a_dom++,b_dom++,pos++) {
				// Intersection of lo_a and lo_b;
				exprNode *lo_a = (*a_dom)->get_lo_range ();
				exprNode *lo_b = (*b_dom)->get_lo_range ();
				string lo_a_id = "", lo_b_id = "";
				int lo_a_val = 0, lo_b_val = 0;
				lo_a->decompose_access_fsm (lo_a_id, lo_a_val);
				lo_b->decompose_access_fsm (lo_b_id, lo_b_val);
				assert (lo_a_id.compare (lo_b_id) == 0);
				exprNode *lo = (lo_a_val >= lo_b_val) ? lo_a : lo_b; 
				// Intersection of hi_a and hi_b;
				exprNode *hi_a = (*a_dom)->get_hi_range ();
				exprNode *hi_b = (*b_dom)->get_hi_range ();
				string hi_a_id = "", hi_b_id = "";	
				int hi_a_val = 0, hi_b_val = 0;
				hi_a->decompose_access_fsm (hi_a_id, hi_a_val);
				hi_b->decompose_access_fsm (hi_b_id, hi_b_val);
				assert (hi_a_id.compare (hi_b_id) == 0);
				exprNode *hi = (hi_a_val <= hi_b_val) ? hi_a : hi_b; 	
				lhs_hull[pos] = new Range (lo, hi);
			}
		}
		// Put the lhs hull into hull_map
		hull_map[lhs_name] = vector<Range*> (lhs_hull);
		// Populate the stmt_hull map
		stmt_hull[stmt_num] = make_tuple (lhs_hull, rhs_hull);
		// Intersect lhs_hull with output_hull 
		int pos = 0;
		vector<Range*>::iterator d_dom = lhs_hull.begin();
		for (vector<Range*>::iterator c_dom=output_hull.begin(); c_dom!=output_hull.end(); c_dom++,d_dom++,pos++) {
			// Intersection of lo_c and lo_b;
			exprNode *lo_c = (*c_dom)->get_lo_range ();
			exprNode *lo_d = (*d_dom)->get_lo_range ();
			string lo_c_id = "", lo_d_id = "";
			int lo_c_val = 0, lo_d_val = 0;
			lo_c->decompose_access_fsm (lo_c_id, lo_c_val);
			lo_d->decompose_access_fsm (lo_d_id, lo_d_val);
			assert (lo_c_id.compare (lo_d_id) == 0);
			exprNode *lo = (lo_c_val >= lo_d_val) ? lo_c : lo_d;
			// Intersection of hi_c and hi_d;
			exprNode *hi_c = (*c_dom)->get_hi_range ();
			exprNode *hi_d = (*d_dom)->get_hi_range ();
			string hi_c_id = "", hi_d_id = "";  
			int hi_c_val = 0, hi_d_val = 0;
			hi_c->decompose_access_fsm (hi_c_id, hi_c_val);
			hi_d->decompose_access_fsm (hi_d_id, hi_d_val);
			assert (hi_c_id.compare (hi_d_id) == 0);
			exprNode *hi = (hi_c_val <= hi_d_val) ? hi_c : hi_d;
			output_hull[pos] = new Range (lo, hi); 
		}
	}

	// Adjust hull for pointwise occurrences
	for (int it=stmts.size()-2; it>=0; it--) {
		// Is the lhs of this statement a pointwise occurrence?
		int src_num = stmts[it]->get_stmt_num ();
		if (find (pointwise_occurrences.begin(), pointwise_occurrences.end(), src_num) == pointwise_occurrences.end()) 
			continue;
		// Check all the dependences, take their intersection with the lhs of this statement
		vector<Range*> &src_hull = get<0>(stmt_hull[src_num]); 
		for (int jt=it+1; jt<(int)stmts.size(); jt++) {
			int dest_num = stmts[jt]->get_stmt_num ();
			if (raw_dependence_exists (src_num, dest_num) || waw_dependence_exists (src_num, dest_num) || accumulation_dependence_exists (src_num, dest_num) || war_dependence_exists (src_num, dest_num)) {
				vector<Range*> dest_hull = get<0>(stmt_hull[dest_num]);
				// Take intersection of src_hull and dest_hull
				vector<Range*>::iterator b_dom = dest_hull.begin ();
				int pos = 0;
				for (vector<Range*>::iterator a_dom=src_hull.begin(); a_dom!=src_hull.end(); a_dom++,b_dom++,pos++) {
					// Intersection of lo_a and lo_b;
					exprNode *lo_a = (*a_dom)->get_lo_range ();
					exprNode *lo_b = (*b_dom)->get_lo_range ();
					string lo_a_id = "", lo_b_id = "";
					int lo_a_val = 0, lo_b_val = 0;
					lo_a->decompose_access_fsm (lo_a_id, lo_a_val);
					lo_b->decompose_access_fsm (lo_b_id, lo_b_val);
					assert (lo_a_id.compare (lo_b_id) == 0);
					exprNode *lo = (lo_a_val >= lo_b_val) ? lo_a : lo_b;
					// Intersection of hi_a and hi_b;
					exprNode *hi_a = (*a_dom)->get_hi_range ();
					exprNode *hi_b = (*b_dom)->get_hi_range ();
					string hi_a_id = "", hi_b_id = "";  
					int hi_a_val = 0, hi_b_val = 0;
					hi_a->decompose_access_fsm (hi_a_id, hi_a_val);
					hi_b->decompose_access_fsm (hi_b_id, hi_b_val);
					assert (hi_a_id.compare (hi_b_id) == 0);
					exprNode *hi = (hi_a_val <= hi_b_val) ? hi_a : hi_b;
					src_hull[pos] = new Range (lo, hi); 
				}
				break;
			}
		}
	}

	// Print the hull after correction
	if (DEBUG) {
		for (auto hull : hull_map) {
			cout << indent << "hull for array " << hull.first << " : [";
			for (vector<Range *>::iterator d=(hull.second).begin(); d!=(hull.second).end(); d++) {
				exprNode *lo = (*d)->get_lo_range ();
				exprNode *hi = (*d)->get_hi_range ();
				string lo_id = "", hi_id = "";
				int lo_val = 0, hi_val = 0;
				lo->decompose_access_fsm (lo_id, lo_val);
				hi->decompose_access_fsm (hi_id, hi_val);
				stringstream lo_out, hi_out;
				if (!lo_id.empty ()) { 
					cout << lo_id;
					if (lo_val > 0)
						cout << "+";
				}
				cout << lo_val;
				cout << ":";
				if (!hi_id.empty()) {
					cout << hi_id;
					if (hi_val > 0)
						cout << "+";
				}
				cout << hi_val;
				if (d != prev((hull.second).end())) 
					cout << " ,";
			}
			cout << "]\n";
		}
		cout << indent << "overall domain hull : [";
		for (vector<Range *>::iterator d=output_hull.begin(); d!=output_hull.end(); d++) {
			exprNode *lo = (*d)->get_lo_range ();
			exprNode *hi = (*d)->get_hi_range ();
			string lo_id = "", hi_id = "";
			int lo_val = 0, hi_val = 0;
			lo->decompose_access_fsm (lo_id, lo_val);
			hi->decompose_access_fsm (hi_id, hi_val);
			stringstream lo_out, hi_out;
			if (!lo_id.empty ()) { 
				cout << lo_id;
				if (lo_val > 0)
					cout << "+";
			}
			if (lo_val != 0)
				cout << lo_val;
			cout << ":";
			if (!hi_id.empty()) {
				cout << hi_id;
				if (hi_val > 0)
					cout << "+";
			}
			if (hi_val != 0)
				cout << hi_val;
			if (d != prev(output_hull.end())) 
				cout << " ,";
		}
		cout << "]\n";
	}

}

// Iterate over all the producers in the DAG that have no predecessors, and take 
// the intersection of their hull. At present, this is not enough, since there may
// be cases where the dependence is pointwise.
vector<Range*> stencilDefn::get_starting_hull (void) {
	vector<Range*> ret = vector<Range*> (output_hull);
	bool ret_set = false;
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	for (auto stmt : stmts) {
		int stmt_num = stmt->get_stmt_num ();
		if ((raw_dependence_graph.find (stmt_num) == raw_dependence_graph.end ()) && 
				(raw_dependence_graph.find (stmt_num) == raw_dependence_graph.end ()) && 
				(raw_dependence_graph.find (stmt_num) == raw_dependence_graph.end ())) {

			// Take intersection 
			if (!ret_set)
				ret = vector<Range*> (get<1>(resource_hull[stmt_num]));
			else {
				vector<Range*>::iterator b_dom = (get<1>(resource_hull[stmt_num])).begin();
				int pos = 0;
				for (vector<Range*>::iterator a_dom=ret.begin(); a_dom!=ret.end(); a_dom++,b_dom++,pos++) {
					// Intersection of lo_a and lo_b;
					exprNode *lo_a = (*a_dom)->get_lo_range ();
					exprNode *lo_b = (*b_dom)->get_lo_range ();
					string lo_a_id = "", lo_b_id = "";
					int lo_a_val = 0, lo_b_val = 0;
					lo_a->decompose_access_fsm (lo_a_id, lo_a_val);
					lo_b->decompose_access_fsm (lo_b_id, lo_b_val);
					assert (lo_a_id.compare (lo_b_id) == 0);
					exprNode *lo = (lo_a_val >= lo_b_val) ? lo_a : lo_b;
					// Intersection of hi_a and hi_b;
					exprNode *hi_a = (*a_dom)->get_hi_range ();
					exprNode *hi_b = (*b_dom)->get_hi_range ();
					string hi_a_id = "", hi_b_id = "";  
					int hi_a_val = 0, hi_b_val = 0;
					hi_a->decompose_access_fsm (hi_a_id, hi_a_val);
					hi_b->decompose_access_fsm (hi_b_id, hi_b_val);
					assert (hi_a_id.compare (hi_b_id) == 0);
					exprNode *hi = (hi_a_val <= hi_b_val) ? hi_a : hi_b;
					ret[pos] = new Range (lo, hi); 
				}
			}
			ret_set = true;
		}
	}
	// If ret is empty, create a full domain and return it
	if (ret.empty ()) {
		for (auto p : parameters) {
			// lo expr is 0
			exprNode *lo = new datatypeNode<int>(0, INT);
			exprNode *hi = new binaryNode (T_MINUS, new idNode (p), new datatypeNode<int>(1, INT), false);
			ret.push_back (new Range (lo, hi));
		}
	}


	cout << indent << "ret hull : [";
	for (vector<Range *>::iterator d=ret.begin(); d!=ret.end(); d++) {
		exprNode *lo = (*d)->get_lo_range ();
		exprNode *hi = (*d)->get_hi_range ();
		string lo_id = "", hi_id = "";
		int lo_val = 0, hi_val = 0;
		lo->decompose_access_fsm (lo_id, lo_val);
		hi->decompose_access_fsm (hi_id, hi_val);
		stringstream lo_out, hi_out;
		if (!lo_id.empty ()) { 
			cout << lo_id;
			if (lo_val > 0)
				cout << "+";
		}
		if (lo_val != 0 || lo_id.empty ())
			cout << lo_val;
		cout << ":";
		if (!hi_id.empty()) {
			cout << hi_id;
			if (hi_val > 0)
				cout << "+";
		}
		if (hi_val != 0 || hi_id.empty ())
			cout << hi_val;
		if (d != prev(ret.end())) 
			cout << " ,";
	}
	cout << "]\n";

	return ret;
}

void stencilDefn::compute_resource_hull (vector<Range*> &initial_domain, vector<int> pointwise_occurrences) {
	// Map from array name to its hull
	map<string, vector<Range*>> hull_map;
	// Modify the initial domain to go from 0 to MAX. For halo_dims, account for the halo
	vector<Range*> full_domain;
	int pos = 0;
	for (vector<Range*>::iterator it=initial_domain.begin(); it!=initial_domain.end(); it++, pos++) {
		string iter = iterators[pos];
		int init_lo = 0, init_hi = -1;
		if (halo_dims.find (iter) != halo_dims.end ()) {
			tuple<int,int> halo = halo_dims[iter];	
			if (get<0>(halo) < 0)
				init_lo += get<0>(halo);
			if (get<1>(halo) > 0)
				init_hi += get<1>(halo);
		}	
		exprNode *lo = new datatypeNode<int> (init_lo, INT);
		exprNode *hi = new datatypeNode<int> (init_hi, INT);
		full_domain.push_back (new Range (lo, hi));
	}
	// Initialize the final compute hull
	recompute_hull = vector<Range*> (full_domain);
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	// Go through the statements and compute hull for each
        for (auto st : stmts) {
                int stmt_num = st->get_stmt_num ();
		vector<Range*> lhs_hull = vector<Range*> (full_domain);
		vector<Range*> rhs_hull = vector<Range*> (full_domain);
		string lhs_name = (st->get_lhs_expr())->get_name (); 
		// Get the offset if the accesses along stream dimension need amendment
		int offset = (stream && (plane_offset.find(stmt_num) != plane_offset.end ())) ? plane_offset[stmt_num] : 0;

		for (vector<stmtNode*>::iterator it=decomposed_stmts[stmt_num].begin(); it!=decomposed_stmts[stmt_num].end(); it++) {
			exprNode *lhs = (*it)->get_lhs_expr ();
			exprNode *rhs = (*it)->get_rhs_expr ();
			int stmt_num = (*it)->get_stmt_num ();
			rhs->compute_hull (hull_map, rhs_hull, full_domain, stmt_resource_map[stmt_num], stream, stream_dim, offset);
			if (!retiming_feasible && (*it)->get_op_type () != ST_EQ)
				lhs->compute_hull (hull_map, rhs_hull, full_domain, stmt_resource_map[stmt_num], stream, stream_dim, offset);

			// Intersect lhs_hull with the statement domain if non empty, and all accesses are not global memory 
			vector<Range*> initial_stmt_domain = (*it)->get_stmt_domain ();
			if (!initial_stmt_domain.empty () && use_shmem) {
				int pos = 0;
				vector<Range*>::iterator b_dom = initial_stmt_domain.begin();
				for (vector<Range*>::iterator a_dom=lhs_hull.begin(); a_dom!=lhs_hull.end(); a_dom++,b_dom++,pos++) {
					string iter = iterators[pos];
					int halo_lo = (halo_dims.find (iter) != halo_dims.end()) ? get<0>(halo_dims[iter]) : 0;
					int halo_hi = (halo_dims.find (iter) != halo_dims.end()) ? get<1>(halo_dims[iter]) : 0;
					// Intersection of lo_a and lo_b;
					exprNode *lo_a = (*a_dom)->get_lo_range ();
					exprNode *lo_b = (*b_dom)->get_lo_range ();
					string lo_a_id = "", lo_b_id = "";
					int lo_a_val = 0, lo_b_val = 0;
					lo_a->decompose_access_fsm (lo_a_id, lo_a_val);
					lo_b->decompose_access_fsm (lo_b_id, lo_b_val);
					// Modify lo_b_val if the halo has been specified
					lo_b_val += halo_lo;
					exprNode *temp_lo_b = new datatypeNode<int> (lo_b_val, INT);
					exprNode *lo = (lo_a_val >= lo_b_val) ? lo_a : temp_lo_b;
					// Intersection of hi_a and hi_b;
					exprNode *hi_a = (*a_dom)->get_hi_range ();
					exprNode *hi_b = (*b_dom)->get_hi_range ();
					string hi_a_id = "", hi_b_id = "";
					int hi_a_val = 0, hi_b_val = 0;
					hi_a->decompose_access_fsm (hi_a_id, hi_a_val);
					hi_b->decompose_access_fsm (hi_b_id, hi_b_val);
					// Modify hi_b_val if the halo has been specified
					hi_b_val += halo_hi;
					exprNode *temp_hi_b = new datatypeNode<int> (hi_b_val, INT);
					exprNode *hi = (hi_a_val <= hi_b_val) ? hi_a : temp_hi_b;
					lhs_hull[pos] = new Range (lo, hi);
				}
			}

			map<string, vector<Range*>> empty_map;
			if (retiming_feasible) {
				empty_map[lhs_name] = vector<Range*> (rhs_hull);
			}
			lhs->compute_hull (empty_map, lhs_hull, full_domain, stmt_resource_map[stmt_num], stream, stream_dim, offset);

			// Intersect lhs_hull with rhs_hull
			int pos = 0;
			vector<Range*>::iterator b_dom = rhs_hull.begin();
			for (vector<Range*>::iterator a_dom=lhs_hull.begin(); a_dom!=lhs_hull.end(); a_dom++,b_dom++,pos++) {
				// Intersection of lo_a and lo_b;
				exprNode *lo_a = (*a_dom)->get_lo_range ();
				exprNode *lo_b = (*b_dom)->get_lo_range ();
				string lo_a_id = "", lo_b_id = "";
				int lo_a_val = 0, lo_b_val = 0;
				lo_a->decompose_access_fsm (lo_a_id, lo_a_val);
				lo_b->decompose_access_fsm (lo_b_id, lo_b_val);
				assert (lo_a_id.compare (lo_b_id) == 0);
				exprNode *lo = (lo_a_val >= lo_b_val) ? lo_a : lo_b; 
				// Intersection of hi_a and hi_b;
				exprNode *hi_a = (*a_dom)->get_hi_range ();
				exprNode *hi_b = (*b_dom)->get_hi_range ();
				string hi_a_id = "", hi_b_id = "";	
				int hi_a_val = 0, hi_b_val = 0;
				hi_a->decompose_access_fsm (hi_a_id, hi_a_val);
				hi_b->decompose_access_fsm (hi_b_id, hi_b_val);
				assert (hi_a_id.compare (hi_b_id) == 0);
				exprNode *hi = (hi_a_val <= hi_b_val) ? hi_a : hi_b; 	
				lhs_hull[pos] = new Range (lo, hi);
			}
		}
		// Put the lhs hull into hull_map
		hull_map[lhs_name] = vector<Range*> (lhs_hull);
		// Populate the resurce_hull map
		resource_hull[stmt_num] = make_tuple (lhs_hull, rhs_hull);
		// Intersect lhs_hull with recompute_hull
		int pos = 0;
		vector<Range*>::iterator d_dom = lhs_hull.begin();
		for (vector<Range*>::iterator c_dom=recompute_hull.begin(); c_dom!=recompute_hull.end(); c_dom++,d_dom++,pos++) {
			// Intersection of lo_c and lo_d;
			exprNode *lo_c = (*c_dom)->get_lo_range ();
			exprNode *lo_d = (*d_dom)->get_lo_range ();
			string lo_c_id = "", lo_d_id = "";
			int lo_c_val = 0, lo_d_val = 0;
			lo_c->decompose_access_fsm (lo_c_id, lo_c_val);
			lo_d->decompose_access_fsm (lo_d_id, lo_d_val);
			assert (lo_c_id.compare (lo_d_id) == 0);
			exprNode *lo = (lo_c_val >= lo_d_val) ? lo_c : lo_d; 
			// Intersection of hi_c and hi_d;
			exprNode *hi_c = (*c_dom)->get_hi_range ();
			exprNode *hi_d = (*d_dom)->get_hi_range ();
			string hi_c_id = "", hi_d_id = "";	
			int hi_c_val = 0, hi_d_val = 0;
			hi_c->decompose_access_fsm (hi_c_id, hi_c_val);
			hi_d->decompose_access_fsm (hi_d_id, hi_d_val);
			assert (hi_c_id.compare (hi_d_id) == 0);
			exprNode *hi = (hi_c_val <= hi_d_val) ? hi_c : hi_d; 	
			recompute_hull[pos] = new Range (lo, hi);
		}
	}

	//TODO: Check correcteness
	// If we are writing an array out, then we must restrict its domain to recompute_hull
	set<string> rbw_reads, rbw_writes;
	map<string,exprNode*> last_writes;
	for (auto st : stmts) {
		for (auto stmt : decomposed_stmts[st->get_stmt_num()]) {
			exprNode *lhs = stmt->get_lhs_expr ();
			exprNode *rhs = stmt->get_rhs_expr ();
			// Compute reads before writes
			rhs->compute_rbw (rbw_reads, rbw_writes, true);
			if (stmt->get_op_type () != ST_EQ)
				lhs->compute_rbw (rbw_reads, rbw_writes, true);
			lhs->compute_rbw (rbw_reads, rbw_writes, false);
			lhs->compute_last_writes (last_writes, stream, stream_dim);
		}
	}

	for (vector<stmtNode*>::iterator it=stmts.begin(); it!=stmts.end(); it++) {
		exprNode *lhs = (*it)->get_lhs_expr ();
		// Only proceed if lhs is copyout, and read before write
		bool lhs_rbw_copyout = lhs->is_shiftvec_node () && lhs->is_last_write (last_writes) && 
			(is_copyout_array (lhs->get_name()) || is_inout_array (lhs->get_name())) && 
			(rbw_reads.find (lhs->get_name()) != rbw_reads.end ());
		if (!lhs_rbw_copyout)
			continue;
		int stmt_num = (*it)->get_stmt_num ();
		vector<Range*> lhs_hull = vector<Range*> (full_domain);
		pos = 0;
		vector<Range*>::iterator d_dom = recompute_hull.begin();
		vector<Range*> &mod_hull = get<0>(resource_hull[stmt_num]);
		for (vector<Range*>::iterator c_dom=mod_hull.begin(); c_dom!=mod_hull.end(); c_dom++,d_dom++,pos++) {
			// Intersection of lo_c and lo_d;
			exprNode *lo_c = (*c_dom)->get_lo_range ();
			exprNode *lo_d = (*d_dom)->get_lo_range ();
			string lo_c_id = "", lo_d_id = "";
			int lo_c_val = 0, lo_d_val = 0;
			lo_c->decompose_access_fsm (lo_c_id, lo_c_val);
			lo_d->decompose_access_fsm (lo_d_id, lo_d_val);
			assert (lo_c_id.compare (lo_d_id) == 0);
			exprNode *lo = (lo_c_val >= lo_d_val) ? lo_c : lo_d; 
			// Intersection of hi_c and hi_d;
			exprNode *hi_c = (*c_dom)->get_hi_range ();
			exprNode *hi_d = (*d_dom)->get_hi_range ();
			string hi_c_id = "", hi_d_id = "";	
			int hi_c_val = 0, hi_d_val = 0;
			hi_c->decompose_access_fsm (hi_c_id, hi_c_val);
			hi_d->decompose_access_fsm (hi_d_id, hi_d_val);
			assert (hi_c_id.compare (hi_d_id) == 0);
			exprNode *hi = (hi_c_val <= hi_d_val) ? hi_c : hi_d; 	
			mod_hull[pos] = new Range (lo, hi);
		}
	}


	// Adjust hull for pointwise occurrences
	for (int it=stmts.size()-2; it>=0; it--) {
		// Is the lhs of this statement a pointwise occurrence?
		int src_num = stmts[it]->get_stmt_num ();
		if (find (pointwise_occurrences.begin(), pointwise_occurrences.end(), src_num) == pointwise_occurrences.end()) 
			continue;
		// Check all the dependences, find the first consumer, and take its intersection with the lhs of this statement
		vector<Range*> &src_hull = get<0>(resource_hull[src_num]); 
		for (int jt=it+1; jt<(int)stmts.size(); jt++) {
			int dest_num = stmts[jt]->get_stmt_num ();
			if (raw_dependence_exists (src_num, dest_num) || waw_dependence_exists (src_num, dest_num) || accumulation_dependence_exists (src_num, dest_num) || war_dependence_exists (src_num, dest_num)) {
				vector<Range*> dest_hull = get<0>(resource_hull[dest_num]);
				// Take intersection of src_hull and dest_hull
				vector<Range*>::iterator b_dom = dest_hull.begin ();
				int pos = 0;
				for (vector<Range*>::iterator a_dom=src_hull.begin(); a_dom!=src_hull.end(); a_dom++,b_dom++,pos++) {
					// Intersection of lo_a and lo_b;
					exprNode *lo_a = (*a_dom)->get_lo_range ();
					exprNode *lo_b = (*b_dom)->get_lo_range ();
					string lo_a_id = "", lo_b_id = "";
					int lo_a_val = 0, lo_b_val = 0;
					lo_a->decompose_access_fsm (lo_a_id, lo_a_val);
					lo_b->decompose_access_fsm (lo_b_id, lo_b_val);
					assert (lo_a_id.compare (lo_b_id) == 0);
					exprNode *lo = (lo_a_val >= lo_b_val) ? lo_a : lo_b;
					// Intersection of hi_a and hi_b;
					exprNode *hi_a = (*a_dom)->get_hi_range ();
					exprNode *hi_b = (*b_dom)->get_hi_range ();
					string hi_a_id = "", hi_b_id = "";  
					int hi_a_val = 0, hi_b_val = 0;
					hi_a->decompose_access_fsm (hi_a_id, hi_a_val);
					hi_b->decompose_access_fsm (hi_b_id, hi_b_val);
					assert (hi_a_id.compare (hi_b_id) == 0);
					exprNode *hi = (hi_a_val <= hi_b_val) ? hi_a : hi_b;
					src_hull[pos] = new Range (lo, hi); 
				}
				break;
			}
		}
	}

	// Print the hull after adjustments
	if (DEBUG) {
		for (auto hull : hull_map) {
			cout << indent << "resource hull for array " << hull.first << " : [";
			for (vector<Range *>::iterator d=(hull.second).begin(); d!=(hull.second).end(); d++) {
				exprNode *lo = (*d)->get_lo_range ();
				exprNode *hi = (*d)->get_hi_range ();
				string lo_id = "", hi_id = "";
				int lo_val = 0, hi_val = 0;
				lo->decompose_access_fsm (lo_id, lo_val);
				hi->decompose_access_fsm (hi_id, hi_val);
				stringstream lo_out, hi_out;
				if (!lo_id.empty ()) { 
					cout << lo_id;
					if (lo_val > 0)
						cout << "+";
				}
				if (lo_val != 0 || lo_id.empty ())
					cout << lo_val;
				cout << ":";
				if (!hi_id.empty()) {
					cout << hi_id;
					if (hi_val > 0)
						cout << "+";
				}
				if (hi_val != 0 || hi_id.empty ())
					cout << hi_val;
				if (d != prev((hull.second).end())) 
					cout << " ,";
			}
			cout << "]\n";
		}
		cout << indent << "recompute hull : [";
		for (vector<Range *>::iterator d=recompute_hull.begin(); d!=recompute_hull.end(); d++) {
			exprNode *lo = (*d)->get_lo_range ();
			exprNode *hi = (*d)->get_hi_range ();
			string lo_id = "", hi_id = "";
			int lo_val = 0, hi_val = 0;
			lo->decompose_access_fsm (lo_id, lo_val);
			hi->decompose_access_fsm (hi_id, hi_val);
			stringstream lo_out, hi_out;
			if (!lo_id.empty ()) { 
				cout << lo_id;
				if (lo_val > 0)
					cout << "+";
			}
			if (lo_val != 0 || lo_id.empty ())
				cout << lo_val;
			cout << ":";
			if (!hi_id.empty()) {
				cout << hi_id;
				if (hi_val > 0)
					cout << "+";
			}
			if (hi_val != 0 || hi_id.empty ())
				cout << hi_val;
			if (d != prev(recompute_hull.end())) 
				cout << " ,";
		}
		cout << "]\n";
	}

}

bool stencilDefn::print_shm_initializations (stringstream &out) {
	bool retflag = true, halo_dim = false;
	if (!use_shmem && !stream)
		return retflag;

	stringstream loop_begin, initializations, loop_end;

	loop_begin << indent << "//Initial loads\n";
	// Find the initial reads. 
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	set<string> rbw_reads, rbw_writes, wbr_reads, wbr_writes;
	for (auto st : stmts) {
		for (auto stmt : decomposed_stmts[st->get_stmt_num()]) {
			exprNode *lhs = stmt->get_lhs_expr ();
			exprNode *rhs = stmt->get_rhs_expr ();
			// Compute reads before writes
			rhs->compute_rbw (rbw_reads, rbw_writes, true);
			if (stmt->get_op_type () != ST_EQ)
				lhs->compute_rbw (rbw_reads, rbw_writes, true);
			lhs->compute_rbw (rbw_reads, rbw_writes, false);
			// Compute writes before reads for retiming initialization
			rhs->compute_wbr (wbr_writes, wbr_reads, false);
			if (stmt->get_op_type () != ST_EQ)
				lhs->compute_wbr (wbr_writes, wbr_reads, false);
			lhs->compute_wbr (wbr_writes, wbr_reads, true);
		}
	}

	// First generate the guard using stencil_hull
	map<string,int> udecls = get_unroll_decls ();
	vector<string> blockdims = (iterators.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iterators.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));
	int pos = 0;
	stringstream for_out, if_out;
	int if_count = 0, for_count = 0;
	if_out << indent << "if (";
	vector<Range*>::iterator dom=initial_hull.begin();
	for (vector<string>::iterator it=iterators.begin(); it!=iterators.end(); it++,dom++,pos++) {
		if (stream && stream_dim.compare (*it) == 0)
			continue;
		int ufactor = (udecls.find (*it) != udecls.end()) ? udecls[*it] : 1;
		int halo_lo = (halo_dims.find (*it) != halo_dims.end()) ? get<0>(halo_dims[*it]) : 0;
		int halo_hi = (halo_dims.find (*it) != halo_dims.end()) ? get<1>(halo_dims[*it]) : 0;
		int halo_size = halo_hi - halo_lo;
		int block_sz = (get_block_dims())[blockdims[pos]];
		int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);

		if (ufactor == 1 && halo_lo == 0 && halo_hi == 0) {
			if (if_count != 0) 
				if_out << " && ";
			if_out << *it << " <= min(" << *it << "0+";
			if_out << "blockDim." << blockdims[pos];
			//Account for equality
			int val = abs(halo_lo) - 1;
			if (val > 0)
				if_out << "+";
			if (val != 0)
				if_out << val;
			if_out << ", ";
			exprNode *hi = (*dom)->get_hi_range ();
			string hi_id = "";
			int hi_val = 0;
			hi->decompose_access_fsm (hi_id, hi_val);
			if_out << hi_id; 
			if (!hi_id.empty() && hi_val > 0)
				if_out << "+";
			if (hi_val != 0) 
				if_out << hi_val;
			if_out << ")";
			if_count++;
		}
		else if (halo_lo == 0 && halo_hi == 0) {
			if (blocked_loads) {
				string for_indent = "";
				for (int idt=0; idt<=for_count; idt++) 
					for_indent = for_indent + indent;
				// Print unroll pragma
				for_out << for_indent << "#pragma unroll " << ufactor << "\n";
				// Print initialization 
				for_out << for_indent << "for (int ";
				for_out << "l1_" << *it << ufactor << " = 0, ";
				for_out << *it << ufactor << " = " << *it << "; ";
				// Print condition
				for_out << "l1_" << *it << ufactor << " < " << ufactor << "; ";
				// Print increment
				for_out << "l1_" << *it << ufactor << "++, "; 
				for_out << *it << ufactor << "++";
				for_out << ") {\n";
				for_count++;

				// Print if condition
				if (if_count != 0) 
					if_out << " && "; 
				//if_out << *it << ufactor << " <= min3(";
				//if_out << *it << "+" << ufactor-1 << ", ";
				if_out << *it << ufactor << " <= min(";
				if_out << *it << "0+";
				if_out << ufactor << "*";
				if_out << "blockDim." << blockdims[pos];
				//Account for equality
				int val = abs(halo_lo) - 1;
				if (val > 0)
					if_out << "+";
				if (val != 0)
					if_out << val;
				if_out << ", ";
				exprNode *hi = (*dom)->get_hi_range ();
				string hi_id = "";
				int hi_val = 0;
				hi->decompose_access_fsm (hi_id, hi_val);
				if_out << hi_id; 
				if (!hi_id.empty() && hi_val > 0)
					if_out << "+";
				if (hi_val != 0) 
					if_out << hi_val;
				if_out << ")";
				if_count++;
			}
			else {
				string for_indent = "";
				for (int idt=0; idt<=for_count; idt++) 
					for_indent = for_indent + indent;
				// Print unroll pragma
				for_out << for_indent << "#pragma unroll " << ufactor << "\n";
				// Print initialization 
				for_out << for_indent << "for (int ";
				for_out << "l1_" << *it << ufactor << " = 0, ";
				for_out << *it << ufactor << " = " << *it << "; ";
				// Print condition
				for_out << "l1_" << *it << ufactor << " < " << ufactor << "; ";
				// Print increment
				for_out << "l1_" << *it << ufactor << "++, ";
				for_out << *it << ufactor << " += blockDim." << blockdims[pos];
				for_out << ") {\n";
				for_count++;

				// Print if condition
				if (if_count != 0) 
					if_out << " && "; 
				if_out << *it << ufactor << " <= min(" << *it << "0+";
				if_out << ufactor << "*";
				if_out << "blockDim." << blockdims[pos];
				//Account for equality
				int val = abs(halo_lo) - 1;
				if (val > 0)
					if_out << "+";
				if (val != 0)
					if_out << val;
				if_out << ", ";
				exprNode *hi = (*dom)->get_hi_range ();
				string hi_id = "";
				int hi_val = 0;
				hi->decompose_access_fsm (hi_id, hi_val);
				if_out << hi_id; 
				if (!hi_id.empty() && hi_val > 0)
					if_out << "+";
				if (hi_val != 0) 
					if_out << hi_val;
				if_out << ")";
				if_count++;
			}
		}
		else {
			//assert (!blocked_loads);
			halo_dim = true;
			string for_indent = "";
			for (int idt=0; idt<=for_count; idt++) 
				for_indent = for_indent + indent;
			// Print unroll pragma
			for_out << for_indent << "#pragma unroll " << halo_ufactor << "\n";
			// Print initialization 
			for_out << for_indent << "for (int ";
			for_out << *it << halo_ufactor << " = ";
			for_out << *it << "0+(int)threadIdx." << blockdims[pos];
			//if (halo_lo < 0)
			//	for_out << halo_lo;
			for_out << "; ";
			// Print the lower bound as if condition
			if (if_count != 0)
				if_out << " && ";
			if_out << *it << halo_ufactor << " >= ";
			exprNode *lo = (*dom)->get_lo_range ();
			string lo_id = "";
			int lo_val = 0;
			lo->decompose_access_fsm (lo_id, lo_val);
			if (!lo_id.empty () && lo_val != 0)
				if_out << "(";
			if_out << lo_id; 
			if (!lo_id.empty() && lo_val >= 0)
				if_out << "+";
			if_out << lo_val;
			if (!lo_id.empty () && lo_val != 0)
				if_out << ")";
			if_count++;
			// Print the higher bound for the for loop
			for_out << *it << halo_ufactor << " <= min(" << *it << "0+";
			if (ufactor > 1)
				for_out << ufactor << "*";
			for_out << "(int)blockDim." << blockdims[pos];
			// Account for the equality+1
			int ub = halo_hi  - halo_lo  - 1;
			if (ub > 0)
				for_out << "+" << ub;
			for_out<< ", ";
			exprNode *hi = (*dom)->get_hi_range ();
			string hi_id = "";
			int hi_val = 0;
			hi->decompose_access_fsm (hi_id, hi_val);
			for_out << hi_id; 
			if (!hi_id.empty() && hi_val > 0)
				for_out << "+";
			if (hi_val != 0) 
				for_out << hi_val;
			for_out << "); ";
			// Print increment
			for_out << *it << halo_ufactor << " += blockDim." << blockdims[pos] << ") {\n";
			for_count++;
		}
	}
	if_out << ") {\n";
	// Print the for loops
	if (for_count != 0) { 
		loop_begin << for_out.str ();
	}
	// Print if condition in the innermost loop
	if (if_count != 0) {
		if (for_count != 0) {
			for (int idt=0; idt<for_count; idt++)
				loop_begin << indent;
		}
		loop_begin << if_out.str ();
	}

	string stmt_indent = indent;
	if (if_count != 0) 
		stmt_indent = stmt_indent + indent;
	for (int cl=0; cl<for_count; cl++) 
		stmt_indent = stmt_indent + indent;

	// Print the initializations now. Begin by computing the extent map, and the minimum extent
	map<tuple<string,bool>, tuple<int,int>> resource_extent;
	int min_extent = 0;
	for (auto srmap : stmt_resource_map) {
		int stmt_num = srmap.first;
		if ((plane_offset.find (stmt_num) != plane_offset.end ()) && (plane_offset[stmt_num] != 0))
			continue;
		for (auto rmap : srmap.second) {
			tuple<string,int,bool> first = rmap.first;
			string arr_name = trim_string (get<0>(first), "@");
			if (rbw_reads.find (arr_name) == rbw_reads.end())
				continue;
			int extent = get<1>(first);
			min_extent = min (min_extent, extent);
		}
	}
	min_extent = abs (min_extent);

	for (auto rmap : resource_map) {
		tuple<string,int,bool> first = rmap.first;
		string arr_name = trim_string (get<0>(first), "@");
		/*
		   when not to continue: 1. it is global memory
		   2. it is not read before write for normal cases
		   3. retiming is true, and it is not a write before read
		 */
		if (rmap.second == GLOBAL_MEM ||
				is_temp_array (arr_name) || 
				((rbw_reads.find (arr_name) == rbw_reads.end()) && 
				 (!retiming_feasible || (retiming_feasible && wbr_reads.find (arr_name) == wbr_reads.end ()))))
			continue;
		int extent = get<1>(first);
		tuple<string,bool> key = make_tuple (get<0>(first), get<2>(first));
		int lo_extent=extent, hi_extent=extent;
		if (resource_extent.find (key) != resource_extent.end ()) {
			tuple<int,int> value = resource_extent[key];
			lo_extent = min (get<0>(value), extent);
			hi_extent = max (get<1>(value), extent);
		}
		resource_extent[key] = make_tuple (lo_extent, hi_extent);
	}

	cout << "RBW arrays : ";
	for (auto a : rbw_reads) {
		cout << a << " ";
	}
	// Start printing resource, centered at min_extent
	for (auto emap : resource_extent) {
		tuple<string,bool> first = emap.first;
		tuple<int,int> second = emap.second;
		string arr_name = trim_string (get<0>(first), "@");
		// If the resource is not a read-only value, continue
		if (((rbw_reads.find (arr_name) == rbw_reads.end()) && 
					(!retiming_feasible || (retiming_feasible && wbr_reads.find (arr_name) == wbr_reads.end ()))))
			continue;
		for (int extent=get<0>(second); extent<=get<1>(second); extent++) {
			string res_name = get<0>(first);
			bool resource_found = false;
			RESOURCE res = GLOBAL_MEM;
			tuple<string,int,bool> key = make_tuple (get<0>(first), extent, get<1>(first));
			if (get<1>(first)) {
				res = (resource_map.find (key) != resource_map.end ()) ? resource_map[key] : REGISTER;
				resource_found = true;
			}
			else if (resource_map.find (key) != resource_map.end ()) { 
				res = resource_map[key];
				resource_found = true;	
			}
			// If resource is found, and it is an initial read, print initialization
			if (resource_found && (res==SHARED_MEM) && (rbw_reads.find (arr_name) != rbw_reads.end ())) {
				assert (access_stats.find (key) != access_stats.end ());
				map<int,vector<string>> index_accesses = get<2>(access_stats[key]);
				// Print LHS only if it is shared memory
				if (res == SHARED_MEM) {
					initializations << stmt_indent << print_trimmed_string (get<0>(key), '@');
					initializations << "_shm";
					if (get<2>(key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						initializations << append << abs(extent);
					}
					// Here comes the complicated part 
					for (auto ia : index_accesses) {
						vector<string> index_vec = ia.second;
						assert (index_vec.size() == 1);
						if (get<2>(key) && stream_dim.compare (index_vec.front()) == 0)
							continue;
						string idx = index_vec.front ();
						initializations << "[" << idx;
						int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
						int halo_lo = (halo_dims.find (idx) != halo_dims.end()) ? get<0>(halo_dims[idx]) : 0;
						int halo_hi = (halo_dims.find (idx) != halo_dims.end()) ? get<1>(halo_dims[idx]) : 0;
						int halo_size = halo_hi - halo_lo;
						vector<string>::iterator iter = find (iterators.begin(), iterators.end(), idx);
						int block_sz = (get_block_dims())[blockdims[iter-iterators.begin()]];
						int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
						if (halo_ufactor > 1)
							initializations << halo_ufactor;	
						initializations << "-" << idx << "0" << "]";
					}

					initializations << " = ";
					// Print RHS
					initializations << arr_name;
					// Print the domain of the array
					vector<Range*> range = get_array_range (arr_name);
					int pos = 0;
					for (vector<Range*>::iterator it=range.begin(); it!=range.end(); it++,pos++) {
						exprNode *lo = (*it)->get_lo_range ();
						exprNode *hi = (*it)->get_hi_range ();
						string lo_id = "", hi_id = "";
						int lo_val = 0, hi_val = 0;
						lo->decompose_access_fsm (lo_id, lo_val);
						hi->decompose_access_fsm (hi_id, hi_val);
						vector<string> index_vec = index_accesses[pos];
						assert (index_vec.size() <= 1);
						if (lo_id.empty () && hi_id.empty ()) {
							// Find the first number from res_name surrounded by @@
							// Find the first @
							cout << "arr_name = " << arr_name << ", res_name = " << res_name; 
							size_t p1 = res_name.find ("@");
							res_name = res_name.substr (p1+1);
							size_t p2 = res_name.find ("@");
							string val = res_name.substr(0,p2);
							res_name = res_name.substr(p2+1);
							cout << ", val = " << val << endl;
							initializations << "[" << stoi (val) << "]";
						}
						else {
							if (full_stream && (stream_dim.compare (index_vec.front()) == 0)) {
								// Get the lo range of the initial_hull along the streaming dimension
								vector<string>::iterator piv = find (iterators.begin(), iterators.end(), index_vec.front());
								assert (piv != iterators.end());
								exprNode *init_lo = (initial_hull[piv-iterators.begin()])->get_lo_range ();
								string init_lo_id = "";
								int init_lo_val = 0;
								init_lo->decompose_access_fsm (init_lo_id, init_lo_val);
								initializations << "[";
								if(!code_diff) {
							          initializations << "min(";
								  if (!init_lo_id.empty ()) 
								  	initializations << init_lo_id;
								  int plane = max (0, min_extent + extent + init_lo_val);
								  if (!init_lo_id.empty() && plane > 0)
								  	initializations << "+";	
								  initializations << plane << ", ";
								  exprNode *init_hi = (initial_hull[piv-iterators.begin()])->get_hi_range ();
								  string init_hi_id = "";
								  int init_hi_val = 0;
								  init_hi->decompose_access_fsm (init_hi_id, init_hi_val);
								  initializations << init_hi_id;
								  if (!init_hi_id.empty() && init_hi_val>0)
								  	initializations << "+";
								  if (init_hi_val != 0)
								  	initializations << init_hi_val;
								  initializations << ")";
								}
								else
									initializations << "0";
								initializations << "]"; 
							}
							else if (stream && (stream_dim.compare (index_vec.front()) == 0)) {
								string idx = index_vec.front ();
								initializations << "[";
								if (!code_diff) {
									initializations << "min(" << idx << "0";
									int plane = max (0, min_extent + extent);
									if (plane > 0) {
										initializations << "+" << plane;
									}
									initializations << ", ";
								    	vector<string>::iterator piv = find (iterators.begin(), iterators.end(), index_vec.front());
								    	exprNode *init_hi = (initial_hull[piv-iterators.begin()])->get_hi_range ();
								    	string init_hi_id = "";
								    	int init_hi_val = 0;
								    	init_hi->decompose_access_fsm (init_hi_id, init_hi_val);
								    	initializations << init_hi_id;
								    	if (!init_hi_id.empty() && init_hi_val>0)
								    		initializations << "+";
								    	if (init_hi_val != 0)
								    		initializations << init_hi_val;
								    	initializations << ")";
								}
								else
									initializations << "0";		
								initializations << "]"; 
							}
							else {
								string idx = index_vec.front ();
								initializations << "[" << idx;
          			if (code_diff) 
									initializations << "-" << idx << "0";
								int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
								int halo_lo = (halo_dims.find (idx) != halo_dims.end()) ? get<0>(halo_dims[idx]) : 0;
								int halo_hi = (halo_dims.find (idx) != halo_dims.end()) ? get<1>(halo_dims[idx]) : 0;
								int halo_size = halo_hi - halo_lo;
								vector<string>::iterator iter = find (iterators.begin(), iterators.end(), idx);
								int block_sz = (get_block_dims())[blockdims[iter-iterators.begin()]];
								int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
								if (halo_ufactor > 1)
									initializations << halo_ufactor;
								initializations << "]";
							}
						}
					}
					initializations << ";\n";
				}
			}
			if (retiming_feasible && resource_found && (res==SHARED_MEM) && (wbr_reads.find (arr_name) != wbr_reads.end ())) {
				assert (access_stats.find (key) != access_stats.end ());
				map<int,vector<string>> index_accesses = get<2>(access_stats[key]);
				initializations << stmt_indent << print_trimmed_string (get<0>(key), '@');
				initializations << "_shm";
				if (get<2>(key)) {
					string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
					initializations << append << abs(extent);
				}
				// Here comes the complicated part 
				for (auto ia : index_accesses) {
					vector<string> index_vec = ia.second;
					assert (index_vec.size() == 1);
					if (get<2>(key) && stream_dim.compare (index_vec.front()) == 0)
						continue;
					string idx = index_vec.front ();
					initializations << "[" << idx;
					int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
					int halo_lo = (halo_dims.find (idx) != halo_dims.end()) ? get<0>(halo_dims[idx]) : 0;
					int halo_hi = (halo_dims.find (idx) != halo_dims.end()) ? get<1>(halo_dims[idx]) : 0;
					int halo_size = halo_hi - halo_lo;
					vector<string>::iterator iter = find (iterators.begin(), iterators.end(), idx);
					int block_sz = (get_block_dims())[blockdims[iter-iterators.begin()]];
					int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
					if (halo_ufactor > 1)
						initializations << halo_ufactor;
					initializations << "-" << idx << "0" << "]";
				}

				initializations << " = ";
				// Print RHS
				initializations << "0;\n";
			}
		}
	}
	// Close the if condition
	if (if_count != 0) {
		for (int cl=0; cl<=for_count; cl++) 
			loop_end << indent;	 
		loop_end << "}\n";
	}
	// Print closing braces for the for loops
	if (for_count != 0) {
		for (int cl=for_count; cl>0; cl--) {
			for (int idt=0; idt<cl; idt++)
				loop_end << indent;
			loop_end << "}\n";
		}
	}
	if (!(initializations.str()).empty()) {
		out << loop_begin.str() << initializations.str();
		if (halo_dim) { 
			out << loop_end.str() << endl;
			retflag = true;
		}
		else {
			retflag = false;
		}
	}
	return retflag;
}

void stencilDefn::print_reg_initializations (stringstream &out, bool print_condition) {
	stringstream loop_begin, initializations, loop_end;

	loop_begin << indent << "//Initial loads\n";
	// Find the initial reads. 
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	set<string> reads, writes;
	for (auto st : stmts) {
		for (auto stmt : decomposed_stmts[st->get_stmt_num()]) {
			exprNode *lhs = stmt->get_lhs_expr ();
			exprNode *rhs = stmt->get_rhs_expr ();
			rhs->compute_rbw (reads, writes, true);
			if (stmt->get_op_type () != ST_EQ)
				lhs->compute_rbw (reads, writes, true);
			lhs->compute_rbw (reads, writes, false);
		}
	}

	// First generate the guard using stencil_hull
	map<string,int> udecls = get_unroll_decls ();
	vector<string> blockdims = (iterators.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iterators.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));
	int pos = 0;
	stringstream for_out, if_out;
	int if_count = 0, for_count = 0;
	if_out << indent << "if (";
	vector<Range*>::iterator dom=initial_hull.begin();
	for (vector<string>::iterator it=iterators.begin(); it!=iterators.end(); it++,dom++,pos++) {
		if (stream && stream_dim.compare (*it) == 0)
			continue;
		int ufactor = (udecls.find (*it) != udecls.end()) ? udecls[*it] : 1;
		int halo_lo = (halo_dims.find (*it) != halo_dims.end()) ? get<0>(halo_dims[*it]) : 0;
		if (ufactor == 1) {
			if (if_count != 0) 
				if_out << " && ";
			if_out << *it << " <= min(" << *it << "0+";
			if_out << "blockDim." << blockdims[pos];
			//Account for equality
			int val = abs(halo_lo) - 1;
			if (val > 0)
				if_out << "+";
			if (val != 0)
				if_out << val;
			if_out << ", ";
			exprNode *hi = (*dom)->get_hi_range ();
			string hi_id = "";
			int hi_val = 0;
			hi->decompose_access_fsm (hi_id, hi_val);
			if_out << hi_id; 
			if (!hi_id.empty() && hi_val > 0)
				if_out << "+";
			if (hi_val != 0) 
				if_out << hi_val;
			if_out << ")";
			if_count++;
		}
		else {
			if (blocked_loads) {
				string for_indent = "";
				for (int idt=0; idt<=for_count; idt++) 
					for_indent = for_indent + indent;
				// Print unroll pragma
				for_out << for_indent << "#pragma unroll " << ufactor << "\n";
				// Print initialization 
				for_out << for_indent << "for (int ";
				for_out << "l1_" << *it << ufactor << " = 0, ";
				for_out << *it << ufactor << " = " << *it << "; ";
				// Print condition
				for_out << "l1_" << *it << ufactor << " < " << ufactor << "; ";
				// Print increment
				for_out << "l1_" << *it << ufactor << "++, "; 
				for_out << *it << ufactor << "++";
				for_out << ") {\n";
				for_count++;

				// Print if condition
				if (if_count != 0) 
					if_out << " && "; 
				//if_out << *it << ufactor << " <= min3(";
				//if_out << *it << "+" << ufactor-1 << ", ";
				if_out << *it << ufactor << " <= min(";
				if_out << *it << "0+";
				if_out << ufactor << "*";
				if_out << "blockDim." << blockdims[pos];
				//Account for equality
				int val = abs(halo_lo) - 1;
				if (val > 0)
					if_out << "+";
				if (val != 0)
					if_out << val;
				if_out << ", ";
				exprNode *hi = (*dom)->get_hi_range ();
				string hi_id = "";
				int hi_val = 0;
				hi->decompose_access_fsm (hi_id, hi_val);
				if_out << hi_id; 
				if (!hi_id.empty() && hi_val > 0)
					if_out << "+";
				if (hi_val != 0) 
					if_out << hi_val;
				if_out << ")";
				if_count++;
			}
			else {
				string for_indent = "";
				for (int idt=0; idt<=for_count; idt++) 
					for_indent = for_indent + indent;
				// Print unroll pragma
				for_out << for_indent << "#pragma unroll " << ufactor << "\n";
				// Print initialization 
				for_out << for_indent << "for (int ";
				for_out << "l1_" << *it << ufactor << " = 0, ";
				for_out << *it << ufactor << " = " << *it << "; ";
				// Print condition
				for_out << "l1_" << *it << ufactor << " < " << ufactor << "; ";
				// Print increment
				for_out << "l1_" << *it << ufactor << "++, ";
				for_out << *it << ufactor << " += blockDim." << blockdims[pos];
				for_out << ") {\n";
				for_count++;

				// Print if condition
				if (if_count != 0) 
					if_out << " && "; 
				if_out << *it << ufactor << " <= min(" << *it << "0+";
				if_out << ufactor << "*";
				if_out << "blockDim." << blockdims[pos];
				//Account for equality
				int val = abs(halo_lo) - 1;
				if (val > 0)
					if_out << "+";
				if (val != 0)
					if_out << val;
				if_out << ", ";
				exprNode *hi = (*dom)->get_hi_range ();
				string hi_id = "";
				int hi_val = 0;
				hi->decompose_access_fsm (hi_id, hi_val);
				if_out << hi_id; 
				if (!hi_id.empty() && hi_val > 0)
					if_out << "+";
				if (hi_val != 0) 
					if_out << hi_val;
				if_out << ")";
				if_count++;
			}
		}
	}
	if_out << ") {\n";
	// Print the for loops
	if (for_count != 0) { 
		loop_begin << for_out.str ();
	}
	// Print if condition in the innermost loop
	if (if_count != 0) {
		if (for_count != 0) {
			for (int idt=0; idt<for_count; idt++)
				loop_begin << indent;
		}
		loop_begin << if_out.str ();
	}

	string stmt_indent = indent;
	if (if_count != 0)
		stmt_indent = stmt_indent + indent;
	for (int cl=0; cl<for_count; cl++) 
		stmt_indent = stmt_indent + indent;

	// Print the initializations now. Begin by computing the extent map, and the minimum extent
	map<tuple<string,bool>, tuple<int,int>> resource_extent;
	int min_extent = 0;
	for (auto srmap : stmt_resource_map) {
		int stmt_num = srmap.first;
		if ((plane_offset.find (stmt_num) != plane_offset.end ()) && (plane_offset[stmt_num] != 0))
			continue;
		for (auto rmap : srmap.second) {
			tuple<string,int,bool> first = rmap.first;
			string arr_name = trim_string (get<0>(first), "@");
			if (reads.find (arr_name) == reads.end ())
				continue;
			int extent = get<1>(first);
			min_extent = min (min_extent, extent);
		}
	}
	min_extent = abs (min_extent);

	for (auto rmap : resource_map) {
		tuple<string,int,bool> first = rmap.first;
		string arr_name = trim_string (get<0>(first), "@");
		if (rmap.second == GLOBAL_MEM || reads.find (arr_name) == reads.end ())
			continue;
		int extent = get<1>(first);
		tuple<string,bool> key = make_tuple (get<0>(first), get<2>(first));
		int lo_extent=extent, hi_extent=extent;
		if (resource_extent.find (key) != resource_extent.end ()) {
			tuple<int,int> value = resource_extent[key];
			lo_extent = min (get<0>(value), extent);
			hi_extent = max (get<1>(value), extent);
		}
		resource_extent[key] = make_tuple (lo_extent, hi_extent);
	}

	// Start printing resource, centered at min_extent
	for (auto emap : resource_extent) {
		tuple<string,bool> first = emap.first;
		tuple<int,int> second = emap.second;
		string arr_name = trim_string (get<0>(first), "@");
		// If the resource is not a read-only value, continue
		if (is_temp_array (arr_name) || 
				(reads.find (arr_name) == reads.end ())) 
			continue;
		for (int extent=get<0>(second); extent<=get<1>(second); extent++) {
			string res_name = get<0>(first);
			bool resource_found = false;
			RESOURCE res;
			tuple<string,int,bool> key = make_tuple (get<0>(first), extent, get<1>(first));
			if (get<1>(first)) {
				res = (resource_map.find (key) != resource_map.end ()) ? resource_map[key] : REGISTER;
				resource_found = true;
			}
			else if (resource_map.find (key) != resource_map.end ()) { 
				res = resource_map[key];
				resource_found = true;	
			}
			// If resource is found, and it is an initial read, print initialization
			if (resource_found && (res==REGISTER) && (reads.find (arr_name) != reads.end ())) {
				map<int,vector<string>> index_accesses;
				if (access_stats.find (key) != access_stats.end ())
					index_accesses = get<2>(access_stats[key]);
				else {
					for (int i=0; i<(int)iterators.size(); i++) {
						vector<string> indices = {iterators[i]};
						index_accesses[i] = indices;
					}
				}
				// Print LHS only if it is a register
				if (res == REGISTER) {
					initializations << stmt_indent << print_trimmed_string (get<0>(key), '@');
					initializations << "_reg";
					if (get<2>(key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						initializations << append << abs(extent);
					}
					for (auto iter : iterators) {
						if ((udecls.find (iter) != udecls.end()) && (udecls[iter] > 1))
							initializations << "[l1_" << iter << udecls[iter] << "]";
					}
					initializations << " = ";
					// Print RHS
					initializations << arr_name;
					// Print the domain of the array
					vector<Range*> range = get_array_range (arr_name);
					int pos = 0;
					for (vector<Range*>::iterator it=range.begin(); it!=range.end(); it++,pos++) {
						exprNode *lo = (*it)->get_lo_range ();
						exprNode *hi = (*it)->get_hi_range ();
						string lo_id = "", hi_id = "";
						int lo_val = 0, hi_val = 0;
						lo->decompose_access_fsm (lo_id, lo_val);
						hi->decompose_access_fsm (hi_id, hi_val);
						vector<string> index_vec = index_accesses[pos];
						assert (index_vec.size() <= 1);
						if (lo_id.empty () && hi_id.empty ()) {
							// Find the first number from res_name surrounded by @@
							// Find the first @
							cout << "arr_name = " << arr_name << ", res_name = " << res_name; 
							size_t p1 = res_name.find ("@");
							res_name = res_name.substr (p1+1);
							size_t p2 = res_name.find ("@");
							string val = res_name.substr(0,p2);
							res_name = res_name.substr(p2+1);
							cout << ", val = " << val << endl;
							initializations << "[" << stoi (val) << "]";
						}
						else {
							if (full_stream && (stream_dim.compare (index_vec.front()) == 0)) {
								// Get the lo range of the initial_hull along the streaming dimension
								vector<string>::iterator piv = find (iterators.begin(), iterators.end(), index_vec.front());
								assert (piv != iterators.end());
								exprNode *init_lo = (initial_hull[piv-iterators.begin()])->get_lo_range ();
								string init_lo_id = "";
								int init_lo_val = 0;
								init_lo->decompose_access_fsm (init_lo_id, init_lo_val);
								initializations << "[min(";
								if (!init_lo_id.empty ()) 
									initializations << init_lo_id;
								int plane = max (0, min_extent + extent + init_lo_val);
								if (!init_lo_id.empty() && plane > 0)
									initializations << "+";
								initializations << plane << ", ";
 								exprNode *init_hi = (initial_hull[piv-iterators.begin()])->get_hi_range ();
								string init_hi_id = "";
								int init_hi_val = 0;
								init_hi->decompose_access_fsm (init_hi_id, init_hi_val);
								initializations << init_hi_id;
								if (!init_hi_id.empty() && init_hi_val>0)
									initializations << "+";
								if (init_hi_val != 0)
									initializations << init_hi_val;
								initializations << ")]"; 
							}
							else if (stream && (stream_dim.compare (index_vec.front()) == 0)) {
								string idx = index_vec.front ();
								initializations << "[min(";
								initializations << idx << "0";
								int plane = max (0, min_extent + extent);
								if (plane > 0) {
									initializations << "+" << plane;
								}
								initializations << ", ";
								vector<string>::iterator piv = find (iterators.begin(), iterators.end(), index_vec.front());
								exprNode *init_hi = (initial_hull[piv-iterators.begin()])->get_hi_range ();
								string init_hi_id = "";
								int init_hi_val = 0;
								init_hi->decompose_access_fsm (init_hi_id, init_hi_val);
								initializations << init_hi_id;
								if (!init_hi_id.empty() && init_hi_val>0)
									initializations << "+";
								if (init_hi_val != 0)
									initializations << init_hi_val;
								initializations << ")]"; 
							}
							else {
								string idx = index_vec.front ();
								initializations << "[" << idx;
								int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
								if (ufactor > 1)
									initializations << ufactor;
								initializations << "]";
							}
						}
					}
					initializations << ";\n";
				}
			}
		}
	}
	// Close the if condition
	if (if_count > 0) {
		for (int cl=0; cl<=for_count; cl++)
			loop_end << indent;
		loop_end << "}\n";
	}
	// Print closing braces for the for loops
	if (for_count != 0) {
		for (int cl=for_count; cl>0; cl--) {
			for (int idt=0; idt<cl; idt++)
				loop_end << indent;
			loop_end << "}\n";
		}
	}
	if (!(initializations.str()).empty()) {
		if (print_condition)
			out << loop_begin.str();
		out << initializations.str() << loop_end.str() << endl;
	}
	else {
		// Print a closing if the shared memory opened paranthesis
		if (!print_condition)
			out << loop_end.str() << endl;
	}
}

// Print the register or shmem prefetch loads without halo
void stencilDefn::print_prefetch_loads_without_halo (stringstream &out) {
	stringstream loop_begin, prefetch_loads, loop_end;
	string loop_indent = stream ? indent : "";

	// Begin by computing the extent map, and the minimum extent
	map<tuple<string,bool>, tuple<int,int>> resource_extent;
	for (auto rmap : resource_map) {
		tuple<string,int,bool> first = rmap.first;
		string arr_name = trim_string (get<0>(first), "@");
		if (rmap.second == GLOBAL_MEM && !is_temp_array (arr_name))
			continue;
		int extent = get<1>(first);
		tuple<string,bool> key = make_tuple (get<0>(first), get<2>(first));
		int lo_extent=extent, hi_extent=extent;
		if (resource_extent.find (key) != resource_extent.end ()) {
			tuple<int,int> value = resource_extent[key];
			lo_extent = min (get<0>(value), extent);
			hi_extent = max (get<1>(value), extent);
		}
		resource_extent[key] = make_tuple (lo_extent, hi_extent);
	}

	loop_begin << dindent << "//Prefetch loads without halo\n";

	// Find the initial reads. 
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	set<string> reads, writes;
	for (auto st : stmts) {
		for (auto stmt : decomposed_stmts[st->get_stmt_num()]) {
			exprNode *lhs = stmt->get_lhs_expr ();
			exprNode *rhs = stmt->get_rhs_expr ();
			rhs->compute_rbw (reads, writes, true);
			if (stmt->get_op_type () != ST_EQ)
				lhs->compute_rbw (reads, writes, true);
			lhs->compute_rbw (reads, writes, false);
		}	
	}

	// First generate the guard using stencil_hull
	map<string,int> udecls = get_unroll_decls ();
	vector<string> blockdims = (iterators.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iterators.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));

	// Loop setup for prefetches
	stringstream for_out, if_out;
	int if_count = 0, for_count = 0;
	if_out << indent << "if (";
	vector<Range*>::iterator dom=initial_hull.begin();
	int pos = 0;
	for (vector<string>::iterator it=iterators.begin(); it!=iterators.end(); it++,dom++,pos++) {
		if (stream && stream_dim.compare (*it) == 0)
			continue;
		int ufactor = (udecls.find (*it) != udecls.end()) ? udecls[*it] : 1;
		int halo_lo = (halo_dims.find (*it) != halo_dims.end()) ? get<0>(halo_dims[*it]) : 0;
		if (ufactor == 1) {
			if (if_count != 0) 
				if_out << " && ";
			if_out << *it << " <= min(" << *it << "0+";
			if_out << "blockDim." << blockdims[pos];
			//Account for equality
			int val = abs(halo_lo) - 1;
			if (val > 0)
				if_out << "+";
			if (val != 0)
				if_out << val;
			if_out << ", ";
			exprNode *hi = (*dom)->get_hi_range ();
			string hi_id = "";
			int hi_val = 0;
			hi->decompose_access_fsm (hi_id, hi_val);
			if_out << hi_id; 
			if (!hi_id.empty() && hi_val > 0)
				if_out << "+";
			if (hi_val != 0) 
				if_out << hi_val;
			if_out << ")";
			if_count++;
		}
		else {
			if (blocked_loads) {
				string for_indent = loop_indent;
				for (int idt=0; idt<=for_count; idt++) 
					for_indent = for_indent + indent;
				// Print unroll pragma
				for_out << for_indent << "#pragma unroll " << ufactor << "\n";
				// Print initialization 
				for_out << for_indent << "for (int ";
				for_out << "l1_" << *it << ufactor << " = 0, ";
				for_out << *it << ufactor << " = " << *it << "; ";
				// Print condition
				for_out << "l1_" << *it << ufactor << " < " << ufactor << "; ";
				// Print increment
				for_out << "l1_" << *it << ufactor << "++, "; 
				for_out << *it << ufactor << "++";
				for_out << ") {\n";
				for_count++;

				// Print if condition
				if (if_count != 0) 
					if_out << " && "; 
				//if_out << *it << ufactor << " <= min3(";
				//if_out << *it << "+" << ufactor-1 << ", ";
				if_out << *it << ufactor << " <= min(";
				if_out << *it << "0+";
				if_out << ufactor << "*";
				if_out << "blockDim." << blockdims[pos];
				//Account for equality
				int val = abs(halo_lo) - 1;
				if (val > 0)
					if_out << "+";
				if (val != 0)
					if_out << val;
				if_out << ", ";
				exprNode *hi = (*dom)->get_hi_range ();
				string hi_id = "";
				int hi_val = 0;
				hi->decompose_access_fsm (hi_id, hi_val);
				if_out << hi_id; 
				if (!hi_id.empty() && hi_val > 0)
					if_out << "+";
				if (hi_val != 0) 
					if_out << hi_val;
				if_out << ")";
				if_count++;
			}
			else {
				string for_indent = loop_indent;
				for (int idt=0; idt<=for_count; idt++) 
					for_indent = for_indent + indent;
				// Print unroll pragma
				for_out << for_indent << "#pragma unroll " << ufactor << "\n";
				// Print initialization 
				for_out << for_indent << "for (int ";
				for_out << "l1_" << *it << ufactor << " = 0, ";
				for_out << *it << ufactor << " = " << *it << "; ";
				// Print condition
				for_out << "l1_" << *it << ufactor << " < " << ufactor << "; ";
				// Print increment
				for_out << "l1_" << *it << ufactor << "++, ";
				for_out << *it << ufactor << " += blockDim." << blockdims[pos];
				for_out << ") {\n";
				for_count++;

				// Print if condition
				if (if_count != 0) 
					if_out << " && "; 
				if_out << *it << ufactor << " <= min(" << *it << "0+";
				if_out << ufactor << "*";
				if_out << "blockDim." << blockdims[pos];
				//Account for equality
				int val = abs(halo_lo) - 1;
				if (val > 0)
					if_out << "+";
				if (val != 0)
					if_out << val;
				if_out << ", ";
				exprNode *hi = (*dom)->get_hi_range ();
				string hi_id = "";
				int hi_val = 0;
				hi->decompose_access_fsm (hi_id, hi_val);
				if_out << hi_id; 
				if (!hi_id.empty() && hi_val > 0)
					if_out << "+";
				if (hi_val != 0) 
					if_out << hi_val;
				if_out << ")";
				if_count++;
			}
		}
	}
	if_out << ") {\n";
	// Print the for loops
	if (for_count != 0) { 
		loop_begin << for_out.str ();
	}
	// Print if condition in the innermost loop
	if (if_count != 0) {
		for (int idt=0; idt<=for_count; idt++)
			loop_begin << indent;
		loop_begin << if_out.str ();
	}

	string stmt_indent = dindent;
	if (if_count != 0) 
		stmt_indent = stmt_indent + indent;
	for (int cl=0; cl<for_count; cl++) 
		stmt_indent = stmt_indent + indent;

	// Start printing the resource
	for (auto emap : resource_extent) {
		tuple<string,bool> first = emap.first;
		tuple<int,int> second = emap.second;
		string arr_name = trim_string (get<0>(first), "@");
		// If the resource is not a read-only value, continue
		if (reads.find (arr_name) == reads.end ()) 
			continue;
		for (int extent=get<1>(second); extent<=get<1>(second); extent++) {
			string res_name = get<0>(first);
			bool resource_found = false;
			RESOURCE res;
			tuple<string,int,bool> key = make_tuple (get<0>(first), extent, get<1>(first));
			if (get<1>(first)) {
				res = (resource_map.find (key) != resource_map.end ()) ? resource_map[key] : REGISTER;
				resource_found = true;
			}
			else if (resource_map.find (key) != resource_map.end ()) { 
				res = resource_map[key];
				resource_found = true;	
			}
			// If resource is found, and it is an initial read, print initialization
			if (resource_found && (res==REGISTER || (res==SHARED_MEM && !halo_exists())) && (reads.find (arr_name) != reads.end ())) {
				assert (access_stats.find (key) != access_stats.end ());
				map<int,vector<string>> index_accesses = get<2>(access_stats[key]);
				// Print LHS
				prefetch_loads << stmt_indent << print_trimmed_string (get<0>(key), '@');
				prefetch_loads << "_prefetch";
				if (get<2>(key)) {
					string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
					prefetch_loads << append << abs(extent);
				}
				int pos = 0;
				for (vector<string>::iterator iter=iterators.begin(); iter!=iterators.end(); iter++,pos++) {
					int ufactor = (udecls.find (*iter) != udecls.end()) ? udecls[*iter] : 1;
					if (ufactor > 1) 
						prefetch_loads << "[l1_" << *iter << ufactor << "]";
				}

				prefetch_loads << " = ";
				// Print RHS
				prefetch_loads << arr_name;
				// Print the domain of the array
				vector<Range*> range = get_array_range (arr_name);
				pos = 0;
				for (vector<Range*>::iterator it=range.begin(); it!=range.end(); it++,pos++) {
					exprNode *lo = (*it)->get_lo_range ();
					exprNode *hi = (*it)->get_hi_range ();
					string lo_id = "", hi_id = "";
					int lo_val = 0, hi_val = 0;
					lo->decompose_access_fsm (lo_id, lo_val);
					hi->decompose_access_fsm (hi_id, hi_val);
					// Adjust hi_val to get the domain from [0 .. N-1]
					hi_val -= 1;
					vector<string> index_vec = index_accesses[pos];
					assert (index_vec.size() <= 1);
					if (lo_id.empty () && hi_id.empty ()) {
						// Find the first number from res_name surrounded by @@
						// Find the first @
						size_t p1 = res_name.find ("@");
						res_name = res_name.substr (p1+1);
						size_t p2 = res_name.find ("@");
						string val = res_name.substr(0,p2);
						res_name = res_name.substr(p2+1);
						cout << ", val = " << val << endl;
						prefetch_loads << "[" << stoi (val) << "]";
					}
					else {
						if (stream && (stream_dim.compare (index_vec.front()) == 0)) {
							int extent_p1 = extent+1;
							string idx = index_vec.front ();
							vector<string>::iterator iter_pos = find (iterators.begin(), iterators.end(), idx);
							assert (iter_pos != iterators.end ());
							int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
							prefetch_loads << "[min(" << idx;
							if (ufactor > 1)
								prefetch_loads << ufactor;
							if (extent_p1 > 0) 
								prefetch_loads << "+";
							if (extent_p1 != 0)
								prefetch_loads << extent_p1;
							// Print the hi range of array
							prefetch_loads << ", ";
							if (!hi_id.empty())
								prefetch_loads << hi_id;
							if (!hi_id.empty() && (hi_val >= 0))
								prefetch_loads << "+";
							prefetch_loads << hi_val;
							prefetch_loads << ")]"; 
						}
						else {
							string idx = index_vec.front ();
							prefetch_loads << "[" << idx;
							vector<string>::iterator iter_pos = find (iterators.begin(), iterators.end(), idx);
							assert (iter_pos != iterators.end ());
							int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
							if (ufactor > 1)
								prefetch_loads << ufactor;
							prefetch_loads << "]";
						}
					}
				}
				prefetch_loads << ";\n";
			}
		}
	}

	// Close the if condition
	if (if_count != 0) {
		for (int cl=0; cl<for_count; cl++) 
			loop_end << indent;	 
		loop_end << dindent << "}\n";
	}
	// Print closing braces for the for loops
	if (for_count != 0) {
		for (int cl=for_count; cl>0; cl--) {
			for (int idt=0; idt<cl; idt++)
				loop_end << indent;
			loop_end << indent << "}\n";
		}
	}

	// Append to out
	if (!(prefetch_loads.str()).empty()) {
		out << loop_begin.str() << prefetch_loads.str() << loop_end.str();
	}
}


// Print the prefetch loads for shared memory with halo
void stencilDefn::print_prefetch_loads_with_halo (stringstream &out) {
	stringstream shm_loop_begin, shm_prefetch_loads, shm_loop_end;
	string loop_indent = stream ? indent : "";

	// Begin by computing the extent map, and the minimum extent
	map<tuple<string,bool>, tuple<int,int>> resource_extent;
	for (auto rmap : resource_map) {
		tuple<string,int,bool> first = rmap.first;
		string arr_name = trim_string (get<0>(first), "@");
		if (rmap.second == GLOBAL_MEM && !is_temp_array (arr_name))
			continue;
		int extent = get<1>(first);
		tuple<string,bool> key = make_tuple (get<0>(first), get<2>(first));
		int lo_extent=extent, hi_extent=extent;
		if (resource_extent.find (key) != resource_extent.end ()) {
			tuple<int,int> value = resource_extent[key];
			lo_extent = min (get<0>(value), extent);
			hi_extent = max (get<1>(value), extent);
		}
		resource_extent[key] = make_tuple (lo_extent, hi_extent);
	}

	shm_loop_begin << dindent << "//Prefetch shared memory loads with halo\n";

	// Find the initial reads. 
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	set<string> reads, writes;
	for (auto st : stmts) {
		for (auto stmt : decomposed_stmts[st->get_stmt_num()]) {
			exprNode *lhs = stmt->get_lhs_expr ();
			exprNode *rhs = stmt->get_rhs_expr ();
			rhs->compute_rbw (reads, writes, true);
			if (stmt->get_op_type () != ST_EQ)
				lhs->compute_rbw (reads, writes, true);
			lhs->compute_rbw (reads, writes, false);
		}
	}

	// First generate the guard using stencil_hull
	map<string,int> udecls = get_unroll_decls ();
	vector<string> blockdims = (iterators.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iterators.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));

	// Loop setup for shared memory prefetches
	stringstream shm_for_out, shm_if_out;
	int shm_if_count = 0, shm_for_count = 0;
	shm_if_out << indent << "if (";
	vector<Range*>::iterator dom=initial_hull.begin();
	int pos = 0;
	for (vector<string>::iterator it=iterators.begin(); it!=iterators.end(); it++,dom++,pos++) {
		if (stream && stream_dim.compare (*it) == 0)
			continue;
		int ufactor = (udecls.find (*it) != udecls.end()) ? udecls[*it] : 1;
		int halo_lo = (halo_dims.find (*it) != halo_dims.end()) ? get<0>(halo_dims[*it]) : 0;
		int halo_hi = (halo_dims.find (*it) != halo_dims.end()) ? get<1>(halo_dims[*it]) : 0;
		int halo_size = halo_hi - halo_lo;
		int block_sz = (get_block_dims())[blockdims[pos]];
		int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);

		if (ufactor == 1 && halo_lo == 0 && halo_hi == 0) {
			if (shm_if_count != 0) 
				shm_if_out << " && ";
			shm_if_out << *it << " <= min(" << *it << "0+";
			shm_if_out << "blockDim." << blockdims[pos];
			//Account for equality
			int val = abs(halo_lo) - 1;
			if (val > 0)
				shm_if_out << "+";
			if (val != 0)
				shm_if_out << val;
			shm_if_out << ", ";
			exprNode *hi = (*dom)->get_hi_range ();
			string hi_id = "";
			int hi_val = 0;
			hi->decompose_access_fsm (hi_id, hi_val);
			shm_if_out << hi_id; 
			if (!hi_id.empty() && hi_val > 0)
				shm_if_out << "+";
			if (hi_val != 0) 
				shm_if_out << hi_val;
			shm_if_out << ")";
			shm_if_count++;
		}
		else if (halo_lo == 0 && halo_hi == 0) {
			if (blocked_loads) {
				string for_indent = loop_indent; 
				for (int idt=0; idt<=shm_for_count; idt++) 
					for_indent = for_indent + indent;
				// Print unroll pragma
				shm_for_out << for_indent << "#pragma unroll " << ufactor << "\n";
				// Print initialization 
				shm_for_out << for_indent << "for (int ";
				shm_for_out << "l1_" << *it << ufactor << " = 0, ";
				shm_for_out << *it << ufactor << " = " << *it << "; ";
				// Print condition
				shm_for_out << "l1_" << *it << ufactor << " < " << ufactor << "; ";
				// Print increment
				shm_for_out << "l1_" << *it << ufactor << "++, "; 
				shm_for_out << *it << ufactor << "++";
				shm_for_out << ") {\n";
				shm_for_count++;

				// Print if condition
				if (shm_if_count != 0) 
					shm_if_out << " && "; 
				//shm_if_out << *it << ufactor << " <= min3(";
				//shm_if_out << *it << "+" << ufactor-1 << ", ";
				shm_if_out << *it << ufactor << " <= min(";
				shm_if_out << *it << "0+";
				shm_if_out << ufactor << "*";
				shm_if_out << "blockDim." << blockdims[pos];
				//Account for equality
				int val = abs(halo_lo) - 1;
				if (val > 0)
					shm_if_out << "+";
				if (val != 0)
					shm_if_out << val;
				shm_if_out << ", ";
				exprNode *hi = (*dom)->get_hi_range ();
				string hi_id = "";
				int hi_val = 0;
				hi->decompose_access_fsm (hi_id, hi_val);
				shm_if_out << hi_id; 
				if (!hi_id.empty() && hi_val > 0)
					shm_if_out << "+";
				if (hi_val != 0) 
					shm_if_out << hi_val;
				shm_if_out << ")";
				shm_if_count++;
			}
			else {
				string for_indent = loop_indent; 
				for (int idt=0; idt<=shm_for_count; idt++) 
					for_indent = for_indent + indent;
				// Print unroll pragma
				shm_for_out << for_indent << "#pragma unroll " << ufactor << "\n";
				// Print initialization 
				shm_for_out << for_indent << "for (int ";
				shm_for_out << "l1_" << *it << ufactor << " = 0, ";
				shm_for_out << *it << ufactor << " = " << *it << "; ";
				// Print condition
				shm_for_out << "l1_" << *it << ufactor << " < " << ufactor << "; ";
				// Print increment
				shm_for_out << "l1_" << *it << ufactor << "++, ";
				shm_for_out << *it << ufactor << " += blockDim." << blockdims[pos];
				shm_for_out << ") {\n";
				shm_for_count++;

				// Print if condition
				if (shm_if_count != 0) 
					shm_if_out << " && "; 
				shm_if_out << *it << ufactor << " <= min(" << *it << "0+";
				shm_if_out << ufactor << "*";
				shm_if_out << "blockDim." << blockdims[pos];
				//Account for equality
				int val = abs(halo_lo) - 1;
				if (val > 0)
					shm_if_out << "+";
				if (val != 0)
					shm_if_out << val;
				shm_if_out << ", ";
				exprNode *hi = (*dom)->get_hi_range ();
				string hi_id = "";
				int hi_val = 0;
				hi->decompose_access_fsm (hi_id, hi_val);
				shm_if_out << hi_id; 
				if (!hi_id.empty() && hi_val > 0)
					shm_if_out << "+";
				if (hi_val != 0) 
					shm_if_out << hi_val;
				shm_if_out << ")";
				shm_if_count++;
			}
		}
		else {
			//assert (!blocked_loads);
			string for_indent = loop_indent;
			for (int idt=0; idt<=shm_for_count; idt++) 
				for_indent = for_indent + indent;
			// Print unroll pragma
			shm_for_out << for_indent << "#pragma unroll " << halo_ufactor << "\n";
			// Print initialization 
			shm_for_out << for_indent << "for (int ";
			shm_for_out << "l1_" << *it << halo_ufactor << " = 0, ";
			shm_for_out << *it << halo_ufactor << " = ";
			shm_for_out << *it << "0+(int)threadIdx." << blockdims[pos];
			//if (halo_lo < 0)
			//	shm_for_out << halo_lo;
			shm_for_out << "; ";
			// Print the lower bound as if condition
			if (shm_if_count != 0)
				shm_if_out << " && ";
			shm_if_out << *it << halo_ufactor << " >= ";
			exprNode *lo = (*dom)->get_lo_range ();
			string lo_id = "";
			int lo_val = 0;
			lo->decompose_access_fsm (lo_id, lo_val);
			if (!lo_id.empty () && lo_val != 0)
				shm_if_out << "(";
			shm_if_out << lo_id; 
			if (!lo_id.empty() && lo_val >= 0)
				shm_if_out << "+";
			shm_if_out << lo_val;
			if (!lo_id.empty () && lo_val != 0)
				shm_if_out << ")";
			shm_if_count++;
			// Print the higher bound for the for loop
			shm_for_out << *it << halo_ufactor << " <= min(" << *it << "0+";
			if (ufactor > 1)
				shm_for_out << ufactor << "*";
			shm_for_out << "(int)blockDim." << blockdims[pos];
			// Account for the equality+1
			int ub = halo_hi  - halo_lo  - 1;
			if (ub > 0)
				shm_for_out << "+" << ub;
			shm_for_out<< ", ";
			exprNode *hi = (*dom)->get_hi_range ();
			string hi_id = "";
			int hi_val = 0;
			hi->decompose_access_fsm (hi_id, hi_val);
			shm_for_out << hi_id; 
			if (!hi_id.empty() && hi_val > 0)
				shm_for_out << "+";
			if (hi_val != 0) 
				shm_for_out << hi_val;
			shm_for_out << "); ";
			// Print increment
			shm_for_out << "l1_" << *it << halo_ufactor << "++, ";
			shm_for_out << *it << halo_ufactor << " += blockDim." << blockdims[pos] << ") {\n";
			shm_for_count++;
		}
	}
	shm_if_out << ") {\n";
	// Print the for loops
	if (shm_for_count != 0) { 
		shm_loop_begin << shm_for_out.str ();
	}
	// Print if condition in the innermost loop
	if (shm_if_count != 0) {
		for (int idt=0; idt<=shm_for_count; idt++)
			shm_loop_begin << indent;
		shm_loop_begin << shm_if_out.str ();
	}

	string shm_stmt_indent = dindent;
	if (shm_if_count != 0) 
		shm_stmt_indent = shm_stmt_indent + indent;
	for (int cl=0; cl<shm_for_count; cl++) 
		shm_stmt_indent = shm_stmt_indent + indent;


	// Start printing shared memory resource
	for (auto emap : resource_extent) {
		tuple<string,bool> first = emap.first;
		tuple<int,int> second = emap.second;
		string arr_name = trim_string (get<0>(first), "@");
		// If the resource is not a read-only value, continue
		if (reads.find (arr_name) == reads.end ()) 
			continue;
		for (int extent=get<1>(second); extent<=get<1>(second); extent++) {
			string res_name = get<0>(first);
			bool resource_found = false;
			RESOURCE res;
			tuple<string,int,bool> key = make_tuple (get<0>(first), extent, get<1>(first));
			if (get<1>(first)) {
				res = (resource_map.find (key) != resource_map.end ()) ? resource_map[key] : REGISTER;
				resource_found = true;
			}
			else if (resource_map.find (key) != resource_map.end ()) { 
				res = resource_map[key];
				resource_found = true;	
			}
			// If resource is found, and it is an initial read, print initialization
			if (resource_found && (res==SHARED_MEM) && (reads.find (arr_name) != reads.end ())) {
				assert (access_stats.find (key) != access_stats.end ());
				map<int,vector<string>> index_accesses = get<2>(access_stats[key]);
				// Print LHS
				shm_prefetch_loads << shm_stmt_indent << print_trimmed_string (get<0>(key), '@');
				shm_prefetch_loads << "_prefetch";
				if (get<2>(key)) {
					string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
					shm_prefetch_loads << append << abs(extent);
				}
				int pos = 0;
				for (vector<string>::iterator iter=iterators.begin(); iter!=iterators.end(); iter++,pos++) {
					int ufactor = (udecls.find (*iter) != udecls.end()) ? udecls[*iter] : 1;
					int halo_lo = (halo_dims.find (*iter) != halo_dims.end()) ? get<0>(halo_dims[*iter]) : 0;
					int halo_hi = (halo_dims.find (*iter) != halo_dims.end()) ? get<1>(halo_dims[*iter]) : 0;
					int halo_size = halo_hi - halo_lo;
					int block_sz = (get_block_dims())[blockdims[pos]];
					int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
					if (halo_ufactor > 1) 
						shm_prefetch_loads << "[l1_" << *iter << halo_ufactor << "]";
				}

				shm_prefetch_loads << " = ";
				// Print RHS
				shm_prefetch_loads << arr_name;
				// Print the domain of the array
				vector<Range*> range = get_array_range (arr_name);
				pos = 0;
				for (vector<Range*>::iterator it=range.begin(); it!=range.end(); it++,pos++) {
					exprNode *lo = (*it)->get_lo_range ();
					exprNode *hi = (*it)->get_hi_range ();
					string lo_id = "", hi_id = "";
					int lo_val = 0, hi_val = 0;
					lo->decompose_access_fsm (lo_id, lo_val);
					hi->decompose_access_fsm (hi_id, hi_val);
					// Adjust hi_val to get the domain from [0 .. N-1]
					hi_val -= 1;
					vector<string> index_vec = index_accesses[pos];
					assert (index_vec.size() <= 1);
					if (lo_id.empty () && hi_id.empty ()) {
						// Find the first number from res_name surrounded by @@
						// Find the first @
						size_t p1 = res_name.find ("@");
						res_name = res_name.substr (p1+1);
						size_t p2 = res_name.find ("@");
						string val = res_name.substr(0,p2);
						res_name = res_name.substr(p2+1);
						cout << ", val = " << val << endl;
						shm_prefetch_loads << "[" << stoi (val) << "]";
					}
					else {
						if (stream && (stream_dim.compare (index_vec.front()) == 0)) {
							int extent_p1 = extent+1;
							string idx = index_vec.front ();
							vector<string>::iterator iter_pos = find (iterators.begin(), iterators.end(), idx);
							assert (iter_pos != iterators.end ());
							int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
							int halo_lo = (halo_dims.find (idx) != halo_dims.end()) ? get<0>(halo_dims[idx]) : 0;
							int halo_hi = (halo_dims.find (idx) != halo_dims.end()) ? get<1>(halo_dims[idx]) : 0;
							int halo_size = halo_hi - halo_lo;
							int block_sz = (get_block_dims())[blockdims[iter_pos-iterators.begin()]];
							int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
							shm_prefetch_loads << "[min(" << idx;
							if (halo_ufactor > 1)
								shm_prefetch_loads << halo_ufactor;
							if (extent_p1 > 0) 
								shm_prefetch_loads << "+";
							if (extent_p1 != 0)
								shm_prefetch_loads << extent_p1;
							// Print the hi range of array
							shm_prefetch_loads << ", ";
							if (!hi_id.empty())
								shm_prefetch_loads << hi_id;
							if (!hi_id.empty() && (hi_val >= 0))
								shm_prefetch_loads << "+";
							shm_prefetch_loads << hi_val;
							shm_prefetch_loads << ")]"; 
						}
						else {
							string idx = index_vec.front ();
							shm_prefetch_loads << "[" << idx;
							vector<string>::iterator iter_pos = find (iterators.begin(), iterators.end(), idx);
							assert (iter_pos != iterators.end ());
							int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
							int halo_lo = (halo_dims.find (idx) != halo_dims.end()) ? get<0>(halo_dims[idx]) : 0;
							int halo_hi = (halo_dims.find (idx) != halo_dims.end()) ? get<1>(halo_dims[idx]) : 0;
							int halo_size = halo_hi - halo_lo;
							int block_sz = (get_block_dims())[blockdims[iter_pos-iterators.begin()]];
							int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
							if (halo_ufactor > 1)
								shm_prefetch_loads << halo_ufactor;
							shm_prefetch_loads << "]";
						}
					}
				}
				shm_prefetch_loads << ";\n";
			}
		}
	}

	// Close the if condition
	if (shm_if_count != 0) {
		for (int cl=0; cl<shm_for_count; cl++) 
			shm_loop_end << indent;	 
		shm_loop_end << dindent << "}\n";
	}
	// Print closing braces for the for loops
	if (shm_for_count != 0) {
		for (int cl=shm_for_count; cl>0; cl--) {
			for (int idt=0; idt<cl; idt++)
				shm_loop_end << indent;
			shm_loop_end << indent << "}\n";
		}
	}

	// Append to out
	if (!(shm_prefetch_loads.str()).empty()) {
		out << shm_loop_begin.str() << shm_prefetch_loads.str() << shm_loop_end.str();
	}
}


string stencilDefn::print_loading_planes_with_nonseparable_halo (stringstream &out, map<tuple<string,bool>, tuple<int,int>> resource_extent, string stmt_indent) {
	stringstream loading_halo_planes, loop_begin, loop_end;
	// Find the initial reads. 
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	set<string> reads, writes;
	for (auto st : stmts) {
		for (auto stmt : decomposed_stmts[st->get_stmt_num()]) {
			exprNode *lhs = stmt->get_lhs_expr ();
			exprNode *rhs = stmt->get_rhs_expr ();
			rhs->compute_rbw (reads, writes, true);
			if (stmt->get_op_type () != ST_EQ)
				lhs->compute_rbw (reads, writes, true);
			lhs->compute_rbw (reads, writes, false);
		}
	}

	// First generate the guard using stencil_hull
	map<string,int> udecls = get_unroll_decls ();
	vector<string> blockdims = (iterators.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iterators.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));

	stringstream for_out, if_out;
	int if_count = 0, for_count = 0;

	// Compute the loop initialization if halo exists
	if (halo_exists ()) {
		if_out << dindent << "if (";
		vector<Range*>::iterator dom=initial_hull.begin();
		int pos = 0;
		for (vector<string>::iterator it=iterators.begin(); it!=iterators.end(); it++,dom++,pos++) {
			if (stream && stream_dim.compare (*it) == 0)
				continue;
			int ufactor = (udecls.find (*it) != udecls.end()) ? udecls[*it] : 1;
			int halo_lo = (halo_dims.find (*it) != halo_dims.end()) ? get<0>(halo_dims[*it]) : 0;
			int halo_hi = (halo_dims.find (*it) != halo_dims.end()) ? get<1>(halo_dims[*it]) : 0;
			int halo_size = halo_hi - halo_lo;
			int block_sz = (get_block_dims())[blockdims[pos]];
			int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);

			if (ufactor == 1 && halo_lo == 0 && halo_hi == 0) {
				if (if_count != 0) 
					if_out << " && ";
				if_out << *it << " <= min(" << *it << "0+";
				if_out << "blockDim." << blockdims[pos];
				//Account for equality
				int val = abs(halo_lo) - 1;
				if (val > 0)
					if_out << "+";
				if (val != 0)
					if_out << val;
				if_out << ", ";
				exprNode *hi = (*dom)->get_hi_range ();
				string hi_id = "";
				int hi_val = 0;
				hi->decompose_access_fsm (hi_id, hi_val);
				if_out << hi_id; 
				if (!hi_id.empty() && hi_val > 0)
					if_out << "+";
				if (hi_val != 0) 
					if_out << hi_val;
				if_out << ")";
				if_count++;
			}
			else if (halo_lo == 0 && halo_hi == 0) {
				if (blocked_loads) {
					string for_indent = indent; 
					for (int idt=0; idt<=for_count; idt++) 
						for_indent = for_indent + indent;
					// Print unroll pragma
					for_out << for_indent << "#pragma unroll " << ufactor << "\n";
					// Print initialization 
					for_out << for_indent << "for (int ";
					for_out << "l1_" << *it << ufactor << " = 0, ";
					for_out << *it << ufactor << " = " << *it << "; ";
					// Print condition
					for_out << "l1_" << *it << ufactor << " < " << ufactor << "; ";
					// Print increment
					for_out << "l1_" << *it << ufactor << "++, "; 
					for_out << *it << ufactor << "++";
					for_out << ") {\n";
					for_count++;

					// Print if condition
					if (if_count != 0) 
						if_out << " && "; 
					//if_out << *it << ufactor << " <= min3(";
					//if_out << *it << "+" << ufactor-1 << ", ";
					if_out << *it << ufactor << " <= min(";
					if_out << *it << "0+";
					if_out << ufactor << "*";
					if_out << "blockDim." << blockdims[pos];
					//Account for equality
					int val = abs(halo_lo) - 1;
					if (val > 0)
						if_out << "+";
					if (val != 0)
						if_out << val;
					if_out << ", ";
					exprNode *hi = (*dom)->get_hi_range ();
					string hi_id = "";
					int hi_val = 0;
					hi->decompose_access_fsm (hi_id, hi_val);
					if_out << hi_id; 
					if (!hi_id.empty() && hi_val > 0)
						if_out << "+";
					if (hi_val != 0) 
						if_out << hi_val;
					if_out << ")";
					if_count++;
				}
				else {
					string for_indent = indent; 
					for (int idt=0; idt<=for_count; idt++) 
						for_indent = for_indent + indent;
					// Print unroll pragma
					for_out << for_indent << "#pragma unroll " << ufactor << "\n";
					// Print initialization 
					for_out << for_indent << "for (int ";
					for_out << "l1_" << *it << ufactor << " = 0, ";
					for_out << *it << ufactor << " = " << *it << "; ";
					// Print condition
					for_out << "l1_" << *it << ufactor << " < " << ufactor << "; ";
					// Print increment
					for_out << "l1_" << *it << ufactor << "++, ";
					for_out << *it << ufactor << " += blockDim." << blockdims[pos];
					for_out << ") {\n";
					for_count++;

					// Print if condition
					if (if_count != 0) 
						if_out << " && "; 
					if_out << *it << ufactor << " <= min(" << *it << "0+";
					if_out << ufactor << "*";
					if_out << "blockDim." << blockdims[pos];
					//Account for equality
					int val = abs(halo_lo) - 1;
					if (val > 0)
						if_out << "+";
					if (val != 0)
						if_out << val;
					if_out << ", ";
					exprNode *hi = (*dom)->get_hi_range ();
					string hi_id = "";
					int hi_val = 0;
					hi->decompose_access_fsm (hi_id, hi_val);
					if_out << hi_id; 
					if (!hi_id.empty() && hi_val > 0)
						if_out << "+";
					if (hi_val != 0) 
						if_out << hi_val;
					if_out << ")";
					if_count++;
				}
			}
			else {
				string for_indent = indent;
				for (int idt=0; idt<=for_count; idt++) 
					for_indent = for_indent + indent;
				// Print unroll pragma
				for_out << for_indent << "#pragma unroll " << halo_ufactor << "\n";
				// Print initialization 
				for_out << for_indent << "for (int ";
				for_out << *it << halo_ufactor << " = ";
				for_out << *it << "0+(int)threadIdx." << blockdims[pos];
				//if (halo_lo < 0)
				//	for_out << halo_lo;
				for_out << "; ";
				// Print the lower bound as if condition
				if (if_count != 0)
					if_out << " && ";
				if_out << *it << halo_ufactor << " >= ";
				exprNode *lo = (*dom)->get_lo_range ();
				string lo_id = "";
				int lo_val = 0;
				lo->decompose_access_fsm (lo_id, lo_val);
				if (!lo_id.empty () && lo_val != 0)
					if_out << "(";
				if_out << lo_id; 
				if (!lo_id.empty() && lo_val >= 0)
					if_out << "+";
				if_out << lo_val;
				if (!lo_id.empty () && lo_val != 0)
					if_out << ")";
				if_count++;
				// Print the higher bound for the for loop
				for_out << *it << halo_ufactor << " <= min(" << *it << "0+";
				if (ufactor > 1)
					for_out << ufactor << "*";
				for_out << "(int)blockDim." << blockdims[pos];
				// Account for the equality+1
				int ub = halo_hi - halo_lo - 1;
				if (ub > 0)
					for_out << "+" << ub;
				for_out<< ", ";
				exprNode *hi = (*dom)->get_hi_range ();
				string hi_id = "";
				int hi_val = 0;
				hi->decompose_access_fsm (hi_id, hi_val);
				for_out << hi_id; 
				if (!hi_id.empty() && hi_val > 0)
					for_out << "+";
				if (hi_val != 0) 
					for_out << hi_val;
				for_out << "); ";
				// Print increment
				for_out << *it << halo_ufactor << " += blockDim." << blockdims[pos] << ") {\n";
				for_count++;
			}
		}
		if_out << ") {\n";
		// Print the for loops
		if (for_count != 0) { 
			loop_begin << for_out.str ();
		}
		// Print if condition in the innermost loop
		if (if_count != 0) {
			if (for_count != 0) {
				for (int idt=0; idt<for_count; idt++)
					loop_begin << indent;
			}
			loop_begin << if_out.str ();
		}
	}

	// Start printing resource
	for (auto emap : resource_extent) {
		tuple<string,bool> first = emap.first;
		tuple<int,int> second = emap.second;
		string arr_name = trim_string (get<0>(first), "@");
		// If the resource is not a read-only value, continue
		if (reads.find (arr_name) == reads.end ()) 
			continue;
		for (int extent=get<1>(second); extent<=get<1>(second); extent++) {
			string res_name = get<0>(first);
			bool resource_found = false;
			RESOURCE res = GLOBAL_MEM;
			tuple<string,int,bool> key = make_tuple (get<0>(first), extent, get<1>(first));
			if (get<1>(first)) {
				res = (resource_map.find (key) != resource_map.end ()) ? resource_map[key] : REGISTER;
				resource_found = true;
			}
			else if (resource_map.find (key) != resource_map.end ()) { 
				res = resource_map[key];
				resource_found = true;	
			}
			// If resource is found, and it is an initial read, print initialization
			if (resource_found && (res==SHARED_MEM) && (reads.find (arr_name) != reads.end ()) && halo_exists ()) {
				// correct statement indent
				string stmt_indent = dindent;
				if (if_count != 0)
					stmt_indent = stmt_indent + indent;
				for (int cl=0; cl<for_count; cl++)
					stmt_indent = stmt_indent + indent;

				assert (access_stats.find (key) != access_stats.end ());
				map<int,vector<string>> index_accesses = get<2>(access_stats[key]);
				// Check if the index accesses actually contain the streaming dimension
				bool found_stream_dim = false;
				for (auto ia : index_accesses) {
					vector<string> index_vec = ia.second;
					assert (index_vec.size() == 1);
					if (get<2>(key) && (stream_dim.compare (index_vec.front()) == 0))
						found_stream_dim = true;
				}
				if (!found_stream_dim)
					continue;

				// Print LHS
				loading_halo_planes << stmt_indent << print_trimmed_string (get<0>(key), '@');
				if (res == SHARED_MEM) {
					loading_halo_planes << "_shm";
					if (get<2>(key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						loading_halo_planes << append << abs(extent);
					}
					// Here comes the complicated part 
					for (auto ia : index_accesses) {
						vector<string> index_vec = ia.second;
						assert (index_vec.size() == 1);
						if (get<2>(key) && stream_dim.compare (index_vec.front()) == 0)
							continue;
						string idx = index_vec.front ();
						loading_halo_planes << "[" << idx;
						int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
						int halo_lo = (halo_dims.find (idx) != halo_dims.end()) ? get<0>(halo_dims[idx]) : 0;
						int halo_hi = (halo_dims.find (idx) != halo_dims.end()) ? get<1>(halo_dims[idx]) : 0;
						int halo_size = halo_hi - halo_lo;
						vector<string>::iterator iter_pos = find (iterators.begin(), iterators.end(), idx);
						int block_sz = (get_block_dims())[blockdims[iter_pos-iterators.begin()]];
						int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
						if (halo_ufactor > 1)
							loading_halo_planes << halo_ufactor;
						loading_halo_planes << "-" << idx << "0" << "]";
					}
				}
				loading_halo_planes << " = ";
				// Print RHS
				loading_halo_planes << arr_name;
				if (prefetch) {
					loading_halo_planes << "_prefetch";
					if (get<2>(key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						loading_halo_planes << append << abs(extent);
					}
					int pos = 0;
					for (vector<string>::iterator iter=iterators.begin(); iter!=iterators.end(); iter++,pos++) {
						int ufactor = (udecls.find (*iter) != udecls.end()) ? udecls[*iter] : 1;
						int halo_lo = (halo_dims.find (*iter) != halo_dims.end()) ? get<0>(halo_dims[*iter]) : 0;
						int halo_hi = (halo_dims.find (*iter) != halo_dims.end()) ? get<1>(halo_dims[*iter]) : 0;
						int halo_size = halo_hi - halo_lo;
						int block_sz = (get_block_dims())[blockdims[pos]];
						int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
						if (halo_ufactor > 1)
							loading_halo_planes << "[l2_" << *iter << halo_ufactor << "]";
					}
				}
				else {
					// Print the domain of the array
					vector<Range*> range = get_array_range (arr_name);
					int pos = 0;
					for (vector<Range*>::iterator it=range.begin(); it!=range.end(); it++,pos++) {
						exprNode *lo = (*it)->get_lo_range ();
						exprNode *hi = (*it)->get_hi_range ();
						string lo_id = "", hi_id = "";
						int lo_val = 0, hi_val = 0;
						lo->decompose_access_fsm (lo_id, lo_val);
						hi->decompose_access_fsm (hi_id, hi_val);
						// Adjust hi_val to get the domain from [0 .. N-1]
						hi_val -= 1;
						vector<string> index_vec = index_accesses[pos];
						assert (index_vec.size() <= 1);
						if (lo_id.empty () && hi_id.empty ()) {
							// Find the first number from res_name surrounded by @@
							// Find the first @
							cout << "arr_name = " << arr_name << ", res_name = " << res_name; 
							size_t p1 = res_name.find ("@");
							res_name = res_name.substr (p1+1);
							size_t p2 = res_name.find ("@");
							string val = res_name.substr(0,p2);
							res_name = res_name.substr(p2+1);
							cout << ", val = " << val << endl;
							loading_halo_planes << "[" << stoi (val) << "]";
						}
						else {
							if (stream && (stream_dim.compare (index_vec.front()) == 0)) {
								int extent_p1 = extent+1;
								string idx = index_vec.front ();
								if (!code_diff) {
									loading_halo_planes << "[min(" << idx;
									int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
									int halo_lo = (halo_dims.find (idx) != halo_dims.end()) ? get<0>(halo_dims[idx]) : 0;
									int halo_hi = (halo_dims.find (idx) != halo_dims.end()) ? get<1>(halo_dims[idx]) : 0;
									int halo_size = halo_hi - halo_lo;
									vector<string>::iterator iter_pos = find (iterators.begin(), iterators.end(), idx);
									int block_sz = (get_block_dims())[blockdims[iter_pos-iterators.begin()]];
									int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
									if (halo_ufactor > 1)
										loading_halo_planes << halo_ufactor;
									if (extent_p1 > 0) 
										loading_halo_planes << "+";
									if (extent_p1 != 0)
										loading_halo_planes << extent_p1;
									// Print the hi range of array
									loading_halo_planes << ", ";
									if (!hi_id.empty())
										loading_halo_planes << hi_id;
									if (!hi_id.empty() && (hi_val >= 0))
										loading_halo_planes << "+";
									loading_halo_planes << hi_val;
									loading_halo_planes << ")";
								}
								else
									loading_halo_planes << idx << "-" << idx << "0";
								loading_halo_planes << "]"; 
							}
							else {
								string idx = index_vec.front ();
								loading_halo_planes << "[";
								if (!code_diff) {
									loading_halo_planes << idx;
									int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
									int halo_lo = (halo_dims.find (idx) != halo_dims.end()) ? get<0>(halo_dims[idx]) : 0;
									int halo_hi = (halo_dims.find (idx) != halo_dims.end()) ? get<1>(halo_dims[idx]) : 0;
									int halo_size = halo_hi - halo_lo;
									vector<string>::iterator iter_pos = find (iterators.begin(), iterators.end(), idx);
									int block_sz = (get_block_dims())[blockdims[iter_pos-iterators.begin()]];
									int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
									if (halo_ufactor > 1)
										loading_halo_planes << halo_ufactor;
								}
								else
									loading_halo_planes << idx << "-" << idx << "0";
								loading_halo_planes << "]";
							}
						}
					}
				}
				loading_halo_planes << ";\n";
			}
			// If resource is found, and it is an initial read, print initialization
			else if (resource_found && (res!=GLOBAL_MEM) && (reads.find (arr_name) != reads.end ())) {
				assert (access_stats.find (key) != access_stats.end ());
				map<int,vector<string>> index_accesses = get<2>(access_stats[key]);
				// Check if the index accesses actually contain the streaming dimension
				bool found_stream_dim = false;
				for (auto ia : index_accesses) {
					vector<string> index_vec = ia.second;
					assert (index_vec.size() == 1);
					if (get<2>(key) && (stream_dim.compare (index_vec.front()) == 0))
						found_stream_dim = true;
				}
				if (!found_stream_dim)
					continue;

				// Print LHS
				out << stmt_indent << print_trimmed_string (get<0>(key), '@');
				if (res == SHARED_MEM) {
					out << "_shm";
					if (get<2>(key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						out << append << abs(extent);
					}
					// Here comes the complicated part 
					for (auto ia : index_accesses) {
						vector<string> index_vec = ia.second;
						assert (index_vec.size() == 1);
						if (get<2>(key) && stream_dim.compare (index_vec.front()) == 0)
							continue;
						string idx = index_vec.front ();
						out << "[" << idx;
						int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
						if (ufactor > 1)
							out << ufactor;
						out << "-" << idx << "0" << "]";
					}
				}
				else if (res == REGISTER) {
					out << "_reg";
					if (get<2>(key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						out << append << abs(extent);
					}
					for (auto iter : iterators) {
						if ((udecls.find (iter) != udecls.end()) && (udecls[iter] > 1))
							out << "[l2_" << iter << udecls[iter] << "]";
					}
				}
				out << " = ";
				// Print RHS
				out << arr_name;
				if (prefetch) {
					out << "_prefetch";
					if (get<2>(key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						out << append << abs(extent);
					}
					for (auto iter : iterators) {
						if ((udecls.find (iter) != udecls.end()) && (udecls[iter] > 1))
							out << "[l2_" << iter << udecls[iter] << "]";
					}
				}
				else {
					// Print the domain of the array
					vector<Range*> range = get_array_range (arr_name);
					int pos = 0;
					for (vector<Range*>::iterator it=range.begin(); it!=range.end(); it++,pos++) {
						exprNode *lo = (*it)->get_lo_range ();
						exprNode *hi = (*it)->get_hi_range ();
						string lo_id = "", hi_id = "";
						int lo_val = 0, hi_val = 0;
						lo->decompose_access_fsm (lo_id, lo_val);
						hi->decompose_access_fsm (hi_id, hi_val);
						// Adjust hi_val to get the domain from [0 .. N-1]
						hi_val -= 1;
						vector<string> index_vec = index_accesses[pos];
						assert (index_vec.size() <= 1);
						if (lo_id.empty () && hi_id.empty ()) {
							// Find the first number from res_name surrounded by @@
							// Find the first @
							cout << "arr_name = " << arr_name << ", res_name = " << res_name; 
							size_t p1 = res_name.find ("@");
							res_name = res_name.substr (p1+1);
							size_t p2 = res_name.find ("@");
							string val = res_name.substr(0,p2);
							res_name = res_name.substr(p2+1);
							cout << ", val = " << val << endl;
							out << "[" << stoi (val) << "]";
						}
						else {
							if (stream && (stream_dim.compare (index_vec.front()) == 0)) {
								int extent_p1 = extent+1;
								string idx = index_vec.front ();
								out << "[";
								if (!code_diff) {
									out << "min(" << idx;
									int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
									if (ufactor > 1)
										out << ufactor;
									if (extent_p1 > 0) 
										out << "+";
									if (extent_p1 != 0)
										out << extent_p1;
									// Print the hi range of array
									out << ", ";
									if (!hi_id.empty())
										out << hi_id;
									if (!hi_id.empty() && (hi_val >= 0))
										out << "+";
									out << hi_val;
									out << ")";
								}
								else 
									out << idx << "-" << idx << "0";
								out << "]"; 
							}
							else {
								string idx = index_vec.front ();
								out << "[";
								if (!code_diff) {
									out << idx;
									int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
									if (ufactor > 1)
										out << ufactor;
							 	}
								else
									out << idx << "-" << idx << "0";	
								out << "]";
							}
						}
					}
				}
				out << ";\n";
			}
		}
	}

	// Close the if condition
	if (if_count != 0) {
		for (int cl=0; cl<for_count; cl++)
			loop_end << indent;
		loop_end << dindent << "}\n";
	}
	// Print closing braces for the for loops
	if (for_count != 0) {
		for (int cl=for_count; cl>0; cl--) {
			for (int idt=0; idt<cl; idt++)
				loop_end << indent;
			loop_end << indent << "}\n";
		}
	}
	stringstream ret_stream;
	if (!(loading_halo_planes.str()).empty ()) {
		if (use_shmem)
			ret_stream << dindent << "__syncthreads ();\n";	
		ret_stream << loop_begin.str() << loading_halo_planes.str() << loop_end.str ();
	}
	return ret_stream.str ();	
}

// Print the loading planes without halo
void stencilDefn::print_loading_planes_without_halo (stringstream &out, map<tuple<string,bool>, tuple<int,int>> resource_extent, string stmt_indent) {
	// Find the initial reads. 
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	set<string> reads, writes;
	for (auto st : stmts) {
		for (auto stmt : decomposed_stmts[st->get_stmt_num()]) {
			exprNode *lhs = stmt->get_lhs_expr ();
			exprNode *rhs = stmt->get_rhs_expr ();
			rhs->compute_rbw (reads, writes, true);
			if (stmt->get_op_type () != ST_EQ)
				lhs->compute_rbw (reads, writes, true);
			lhs->compute_rbw (reads, writes, false);
		}
	}

	// First generate the guard using stencil_hull
	map<string,int> udecls = get_unroll_decls ();
	vector<string> blockdims = (iterators.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iterators.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));

	// Start printing resource
	for (auto emap : resource_extent) {
		tuple<string,bool> first = emap.first;
		tuple<int,int> second = emap.second;
		string arr_name = trim_string (get<0>(first), "@");
		// If the resource is not a read-only value, continue
		if (reads.find (arr_name) == reads.end ()) 
			continue;
		for (int extent=get<1>(second); extent<=get<1>(second); extent++) {
			string res_name = get<0>(first);
			bool resource_found = false;
			RESOURCE res;
			tuple<string,int,bool> key = make_tuple (get<0>(first), extent, get<1>(first));
			if (get<1>(first)) {
				res = (resource_map.find (key) != resource_map.end ()) ? resource_map[key] : REGISTER;
				resource_found = true;
			}
			else if (resource_map.find (key) != resource_map.end ()) { 
				res = resource_map[key];
				resource_found = true;	
			}
			// If resource is found, and it is an initial read, print initialization
			if (resource_found && (res!=GLOBAL_MEM) && (reads.find (arr_name) != reads.end ())) {
				assert (access_stats.find (key) != access_stats.end ());
				map<int,vector<string>> index_accesses = get<2>(access_stats[key]);
				// Print LHS
				out << stmt_indent << print_trimmed_string (get<0>(key), '@');
				if (res == SHARED_MEM) {
					out << "_shm";
					if (get<2>(key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						out << append << abs(extent);
					}
					// Here comes the complicated part 
					for (auto ia : index_accesses) {
						vector<string> index_vec = ia.second;
						assert (index_vec.size() == 1);
						if (get<2>(key) && stream_dim.compare (index_vec.front()) == 0)
							continue;
						string idx = index_vec.front ();
						out << "[" << idx;
						int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
						if (ufactor > 1)
							out << ufactor;
						out << "-" << idx << "0" << "]";
					}
				}
				else if (res == REGISTER) {
					out << "_reg";
					if (get<2>(key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						out << append << abs(extent);
					}
					for (auto iter : iterators) {
						if ((udecls.find (iter) != udecls.end()) && (udecls[iter] > 1))
							out << "[l2_" << iter << udecls[iter] << "]";
					}
				}
				out << " = ";
				// Print RHS
				out << arr_name;
				if (prefetch) {
					out << "_prefetch";
					if (get<2>(key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						out << append << abs(extent);
					}
					for (auto iter : iterators) {
						if ((udecls.find (iter) != udecls.end()) && (udecls[iter] > 1))
							out << "[l2_" << iter << udecls[iter] << "]";
					}
				}
				else {
					// Print the domain of the array
					vector<Range*> range = get_array_range (arr_name);
					int pos = 0;
					for (vector<Range*>::iterator it=range.begin(); it!=range.end(); it++,pos++) {
						exprNode *lo = (*it)->get_lo_range ();
						exprNode *hi = (*it)->get_hi_range ();
						string lo_id = "", hi_id = "";
						int lo_val = 0, hi_val = 0;
						lo->decompose_access_fsm (lo_id, lo_val);
						hi->decompose_access_fsm (hi_id, hi_val);
						// Adjust hi_val to get the domain from [0 .. N-1]
						hi_val -= 1;
						vector<string> index_vec = index_accesses[pos];
						assert (index_vec.size() <= 1);
						if (lo_id.empty () && hi_id.empty ()) {
							// Find the first number from res_name surrounded by @@
							// Find the first @
							cout << "arr_name = " << arr_name << ", res_name = " << res_name; 
							size_t p1 = res_name.find ("@");
							res_name = res_name.substr (p1+1);
							size_t p2 = res_name.find ("@");
							string val = res_name.substr(0,p2);
							res_name = res_name.substr(p2+1);
							cout << ", val = " << val << endl;
							out << "[" << stoi (val) << "]";
						}
						else {
							if (stream && (stream_dim.compare (index_vec.front()) == 0)) {
								int extent_p1 = extent+1;
								string idx = index_vec.front ();
								out << "[";
								if (!code_diff) {
									out << "min(" << idx;
									int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
									if (ufactor > 1)
										out << ufactor;
									if (extent_p1 > 0) 
										out << "+";
									if (extent_p1 != 0)
										out << extent_p1;
									// Print the hi range of array
									out << ", ";
									if (!hi_id.empty())
										out << hi_id;
									if (!hi_id.empty() && (hi_val >= 0))
										out << "+";
									out << hi_val;
									out << ")";
							  }
								else 
									out << idx << "-" << idx << "0";	
								out << "]"; 
							}
							else {
								string idx = index_vec.front ();
								out << "[";
								if (!code_diff) {
									out << idx;
									int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
									if (ufactor > 1)
										out << ufactor;
								}
								else 
									out << idx << "-" << idx << "0";
								out << "]";
							}
						}
					}
				}
				out << ";\n";
			}
		}
	}
}

// Print the loading planes when halo is non-zero, LHS is shared memory, and RHS is global memory 
void stencilDefn::print_loading_planes_with_halo (stringstream &out, map<tuple<string,bool>, tuple<int,int>> resource_extent, string stmt_indent) {
	// Find the initial reads. 
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	set<string> reads, writes;
	for (auto st : stmts) {
		for (auto stmt : decomposed_stmts[st->get_stmt_num()]) {
			exprNode *lhs = stmt->get_lhs_expr ();
			exprNode *rhs = stmt->get_rhs_expr ();
			rhs->compute_rbw (reads, writes, true);
			if (stmt->get_op_type () != ST_EQ)
				lhs->compute_rbw (reads, writes, true);
			lhs->compute_rbw (reads, writes, false);
		}
	}

	// First generate the guard using stencil_hull
	map<string,int> udecls = get_unroll_decls ();
	vector<string> blockdims = (iterators.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iterators.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));

	// Start printing resource
	for (auto emap : resource_extent) {
		tuple<string,bool> first = emap.first;
		tuple<int,int> second = emap.second;
		string arr_name = trim_string (get<0>(first), "@");
		// If the resource is not a read-only value, continue
		if (reads.find (arr_name) == reads.end ()) 
			continue;
		for (int extent=get<1>(second); extent<=get<1>(second); extent++) {
			string res_name = get<0>(first);
			bool resource_found = false;
			RESOURCE res;
			tuple<string,int,bool> key = make_tuple (get<0>(first), extent, get<1>(first));
			if (get<1>(first)) {
				res = (resource_map.find (key) != resource_map.end ()) ? resource_map[key] : REGISTER;
				resource_found = true;
			}
			else if (resource_map.find (key) != resource_map.end ()) { 
				res = resource_map[key];
				resource_found = true;	
			}
			// If resource is found, and it is an initial read, print initialization
			if (resource_found && (res==SHARED_MEM) && (reads.find (arr_name) != reads.end ())) {
				assert (access_stats.find (key) != access_stats.end ());
				map<int,vector<string>> index_accesses = get<2>(access_stats[key]);
				// Print LHS
				out << stmt_indent << print_trimmed_string (get<0>(key), '@');
				out << "_shm";
				if (get<2>(key)) {
					string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
					out << append << abs(extent);
				}
				// Here comes the complicated part 
				for (auto ia : index_accesses) {
					vector<string> index_vec = ia.second;
					assert (index_vec.size() == 1);
					if (get<2>(key) && stream_dim.compare (index_vec.front()) == 0)
						continue;
					string idx = index_vec.front ();
					vector<string>::iterator iter_pos = find (iterators.begin(), iterators.end(), idx);
					assert (iter_pos != iterators.end ());
					int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
					int halo_lo = (halo_dims.find (idx) != halo_dims.end()) ? get<0>(halo_dims[idx]) : 0;
					int halo_hi = (halo_dims.find (idx) != halo_dims.end()) ? get<1>(halo_dims[idx]) : 0;
					int halo_size = halo_hi - halo_lo;
					int block_sz = (get_block_dims())[blockdims[iter_pos-iterators.begin()]];
					int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
					out << "[" << idx;
					if (halo_ufactor > 1)
						out << halo_ufactor;
					out << "-" << idx << "0" << "]";
				}
				out << " = ";
				// Print RHS
				out << arr_name;
				if (prefetch) {
					out << "_prefetch";
					if (get<2>(key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						out << append << abs(extent);
					}
					int pos = 0;
					for (vector<string>::iterator iter=iterators.begin(); iter!=iterators.end(); iter++,pos++) {
						int ufactor = (udecls.find (*iter) != udecls.end()) ? udecls[*iter] : 1;
						int halo_lo = (halo_dims.find (*iter) != halo_dims.end()) ? get<0>(halo_dims[*iter]) : 0;
						int halo_hi = (halo_dims.find (*iter) != halo_dims.end()) ? get<1>(halo_dims[*iter]) : 0;
						int halo_size = halo_hi - halo_lo;
						int block_sz = (get_block_dims())[blockdims[pos]];
						int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
						if (halo_ufactor > 1) 
							out << "[l2_" << *iter << halo_ufactor << "]";
					}
				}
				else {
					// Print the domain of the array
					vector<Range*> range = get_array_range (arr_name);
					int pos = 0;
					for (vector<Range*>::iterator it=range.begin(); it!=range.end(); it++,pos++) {
						exprNode *lo = (*it)->get_lo_range ();
						exprNode *hi = (*it)->get_hi_range ();
						string lo_id = "", hi_id = "";
						int lo_val = 0, hi_val = 0;
						lo->decompose_access_fsm (lo_id, lo_val);
						hi->decompose_access_fsm (hi_id, hi_val);
						// Adjust hi_val to get the domain from [0 .. N-1]
						hi_val -= 1;
						vector<string> index_vec = index_accesses[pos];
						assert (index_vec.size() <= 1);
						if (lo_id.empty () && hi_id.empty ()) {
							// Find the first number from res_name surrounded by @@
							// Find the first @
							cout << "arr_name = " << arr_name << ", res_name = " << res_name; 
							size_t p1 = res_name.find ("@");
							res_name = res_name.substr (p1+1);
							size_t p2 = res_name.find ("@");
							string val = res_name.substr(0,p2);
							res_name = res_name.substr(p2+1);
							cout << ", val = " << val << endl;
							out << "[" << stoi (val) << "]";
						}
						else {
							if (stream && (stream_dim.compare (index_vec.front()) == 0)) {
								int extent_p1 = extent+1;
								string idx = index_vec.front ();
								vector<string>::iterator iter_pos = find (iterators.begin(), iterators.end(), idx);
								assert (iter_pos != iterators.end ());
								int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
								int halo_lo = (halo_dims.find (idx) != halo_dims.end()) ? get<0>(halo_dims[idx]) : 0;
								int halo_hi = (halo_dims.find (idx) != halo_dims.end()) ? get<1>(halo_dims[idx]) : 0;
								int halo_size = halo_hi - halo_lo;
								int block_sz = (get_block_dims())[blockdims[iter_pos-iterators.begin()]];
								int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
								out << "[";
								if (!code_diff) {
									out << "min(" << idx;
									if (halo_ufactor > 1)
										out << halo_ufactor;
									if (extent_p1 > 0) 
										out << "+";
									if (extent_p1 != 0)
										out << extent_p1;
									// Print the hi range of array
									out << ", ";
									if (!hi_id.empty())
										out << hi_id;
									if (!hi_id.empty() && (hi_val >= 0))
										out << "+";
									out << hi_val;
									out << ")";
								}
								else
									out << idx << "-" << idx << "0";
								out << "]"; 
							}
							else {
								string idx = index_vec.front ();
								out << "[";
								if (!code_diff) {
									out << idx;
									vector<string>::iterator iter_pos = find (iterators.begin(), iterators.end(), idx);
									assert (iter_pos != iterators.end ());
									int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
									int halo_lo = (halo_dims.find (idx) != halo_dims.end()) ? get<0>(halo_dims[idx]) : 0;
									int halo_hi = (halo_dims.find (idx) != halo_dims.end()) ? get<1>(halo_dims[idx]) : 0;
									int halo_size = halo_hi - halo_lo;
									int block_sz = (get_block_dims())[blockdims[iter_pos-iterators.begin()]];
									int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
									if (halo_ufactor > 1)
										out << halo_ufactor;
								}
								else
									out << idx << "-" << idx << "0";
								out << "]";
							}
						}
					}
				}
				out << ";\n";
			}
		}
	}
}

// Print the rotating plane when halo is 0
void stencilDefn::print_rotating_planes_without_halo (stringstream &out, map<tuple<string,bool>, tuple<int,int>> resource_extent) {
	stringstream loop_begin, rotations, loop_end;

	//loop_begin << dindent << "__syncthreads ();\n";
	loop_begin << "\n" << dindent << "//Shift planes without halo \n";

	// First generate the guard using stencil_hull
	map<string,int> udecls = get_unroll_decls ();
	vector<string> blockdims = (iterators.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iterators.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));

	stringstream for_out, if_out;
	int if_count = 0, for_count = 0;
	if_out << indent << "if (";
	vector<Range*>::iterator dom=initial_hull.begin();
	int pos = 0;
	for (vector<string>::iterator it=iterators.begin(); it!=iterators.end(); it++,dom++,pos++) {
		if (stream && stream_dim.compare (*it) == 0)
			continue;
		int ufactor = (udecls.find (*it) != udecls.end()) ? udecls[*it] : 1;
		if (ufactor == 1) {
			if (if_count != 0) 
				if_out << " && ";
			if_out << *it << " <= min(" << *it << "0+";
			if_out << "blockDim." << blockdims[pos] << "-1";
			if_out << ", ";
			exprNode *hi = (*dom)->get_hi_range ();
			string hi_id = "";
			int hi_val = 0;
			hi->decompose_access_fsm (hi_id, hi_val);
			if_out << hi_id; 
			if (!hi_id.empty() && hi_val >= 0)
				if_out << "+";
			if_out << hi_val << ")";
			if_count++;
		}
		else {
			if (blocked_loads) {
				string for_indent = indent;
				for (int idt=0; idt<=for_count; idt++) 
					for_indent = for_indent + indent;
				// Print unroll pragma
				for_out << for_indent << "#pragma unroll " << ufactor << "\n";
				// Print initialization 
				for_out << for_indent << "for (int ";
				for_out << "l2_" << *it << ufactor << " = 0, ";
				for_out << *it << ufactor << " = " << *it << "; ";
				// Print condition
				for_out << "l2_" << *it << ufactor << " < " << ufactor << "; ";
				// Print increment
				for_out << "l2_" << *it << ufactor << "++, ";
				for_out << *it << ufactor << "++";
				for_out << ") {\n";
				for_count++;

				// Print if condition
				if (if_count != 0) 
					if_out << " && "; 
				//if_out << *it << ufactor << " <= min3(";
				//if_out << *it << "+" << ufactor-1 << ", ";
				if_out << *it << ufactor << " <= min(";
				if_out << *it << "0+";
				if_out << ufactor << "*";
				if_out << "blockDim." << blockdims[pos] << "-1";
				if_out << ", ";
				exprNode *hi = (*dom)->get_hi_range ();
				string hi_id = "";
				int hi_val = 0;
				hi->decompose_access_fsm (hi_id, hi_val);
				if_out << hi_id; 
				if (!hi_id.empty() && hi_val >= 0)
					if_out << "+";
				if_out << hi_val;
				if_out << ")";
				if_count++;
			}
			else {
				string for_indent = indent;
				for (int idt=0; idt<=for_count; idt++) 
					for_indent = for_indent + indent;
				// Print unroll pragma
				for_out << for_indent << "#pragma unroll " << ufactor << "\n";
				// Print initialization 
				for_out << for_indent << "for (int ";
				for_out << "l2_" << *it << ufactor << " = 0, ";
				for_out << *it << ufactor << " = " << *it << "; ";
				// Print condition
				for_out << "l2_" << *it << ufactor << " < " << ufactor << "; ";
				// Print increment
				for_out << "l2_" << *it << ufactor << "++, ";
				for_out << *it << ufactor << " += blockDim." << blockdims[pos];
				for_out << ") {\n";
				for_count++;

				// Print if condition
				if (if_count != 0) 
					if_out << " && "; 
				if_out << *it << ufactor << " <= min(" << *it << "0+";
				if_out << ufactor << "*";
				if_out << "blockDim." << blockdims[pos] << "-1";
				if_out << ", ";
				exprNode *hi = (*dom)->get_hi_range ();
				string hi_id = "";
				int hi_val = 0;
				hi->decompose_access_fsm (hi_id, hi_val);
				if_out << hi_id; 
				if (!hi_id.empty() && hi_val >= 0)
					if_out << "+";
				if_out << hi_val;
				if_out << ")";
				if_count++;
			}
		}
	}
	if_out << ") {\n";
	// Print the for loops
	if (for_count != 0) { 
		loop_begin << for_out.str ();
	}
	// Print if condition in the innermost loop
	if (if_count != 0) {
		for (int idt=0; idt<=for_count; idt++)
			loop_begin << indent;
		loop_begin << if_out.str ();
	}

	string stmt_indent = dindent;
	if (if_count != 0) 
		stmt_indent = stmt_indent + indent;
	for (int cl=0; cl<for_count; cl++) 
		stmt_indent = stmt_indent + indent;

	// Start printing rotating resource
	for (auto emap : resource_extent) {
		tuple<string,bool> first = emap.first;
		tuple<int,int> second = emap.second;
		string arr_name = trim_string (get<0>(first), "@");
		bool temp_array = is_temp_array (arr_name);
		for (int extent=get<0>(second); extent<get<1>(second); extent++) {
			string res_name = get<0>(first);
			bool lhs_resource_found = false, rhs_resource_found;
			RESOURCE lhs_res = REGISTER, rhs_res = REGISTER;
			int extent_p1 = extent + 1;
			// First find the resource for LHS
			tuple<string,int,bool> lhs_key = make_tuple (get<0>(first), extent, get<1>(first));
			if (get<1>(first)) {
				lhs_res = (resource_map.find (lhs_key) != resource_map.end ()) ? resource_map[lhs_key] : REGISTER;
				lhs_resource_found = true;
			}
			else if (resource_map.find (lhs_key) != resource_map.end ()) { 
				lhs_res = resource_map[lhs_key];
				lhs_resource_found = true;	
			}
			// Now find the resource for RHS 
			tuple<string,int,bool> rhs_key = make_tuple (get<0>(first), extent_p1, get<1>(first));
			if (get<1>(first)) {
				rhs_res = (resource_map.find (rhs_key) != resource_map.end ()) ? resource_map[rhs_key] : REGISTER;
				rhs_resource_found = true;
			}
			else if (resource_map.find (rhs_key) != resource_map.end ()) { 
				rhs_res = resource_map[rhs_key];
				rhs_resource_found = true;	
			}

			// If resource is found, print rotation
			if (lhs_resource_found && rhs_resource_found) {
				assert (access_stats.find (lhs_key) != access_stats.end ());
				map<int,vector<string>> lhs_index_accesses = get<2>(access_stats[lhs_key]);
				map<int,vector<string>> rhs_index_accesses = get<2>(access_stats[rhs_key]);
				// Check if LHS or RHS actually contain the streaming dimension
				bool found_stream_dim = temp_array;
				for (auto ia : lhs_index_accesses) {
					vector<string> index_vec = ia.second;
					assert (index_vec.size() == 1);
					if (get<2>(lhs_key) && (stream_dim.compare (index_vec.front()) == 0))
						found_stream_dim = true;
				}
				for (auto ia : rhs_index_accesses) {
					vector<string> index_vec = ia.second;
					assert (index_vec.size() == 1);
					if (get<2>(rhs_key) && (stream_dim.compare (index_vec.front()) == 0))
						found_stream_dim = true;
				}
				if (!found_stream_dim) 
					continue;
				// Print LHS
				rotations << stmt_indent << print_trimmed_string (get<0>(lhs_key), '@');
				if (lhs_res == GLOBAL_MEM) {
					// Here comes the complicated part 
					for (auto ia : lhs_index_accesses) {
						vector<string> index_vec = ia.second;
						assert (index_vec.size() == 1);
						if (get<2>(lhs_key) && (stream_dim.compare (index_vec.front()) == 0) && temp_array)
							continue;
						string idx = index_vec.front ();
						rotations << "[" << idx;
						int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
						if (ufactor > 1)
							rotations << ufactor;	
						rotations << "]";
					}
				}
				else if (lhs_res == SHARED_MEM) {
					rotations << "_shm";
					if (get<2>(lhs_key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						rotations << append << abs(extent);
					}
					// Here comes the complicated part 
					for (auto ia : lhs_index_accesses) {
						vector<string> index_vec = ia.second;
						assert (index_vec.size() == 1);
						if (get<2>(lhs_key) && stream_dim.compare (index_vec.front()) == 0)
							continue;
						string idx = index_vec.front ();
						rotations << "[" << idx;
						int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
						if (ufactor > 1)
							rotations << ufactor;
						rotations << "-" << idx << "0" << "]";
					}
				}
				else if (lhs_res == REGISTER) {
					rotations << "_reg";
					if (get<2>(lhs_key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						rotations << append << abs(extent);
						for (auto iter : iterators) {
							if ((udecls.find (iter) != udecls.end()) && (udecls[iter] > 1))
								rotations << "[l2_" << iter << udecls[iter] << "]";
						}
					}
				}
				rotations << " = ";
				// Print RHS
				rotations << print_trimmed_string (get<0>(rhs_key), '@');
				if (rhs_res == GLOBAL_MEM) {
					// Here comes the complicated part 
					for (auto ia : rhs_index_accesses) {
						vector<string> index_vec = ia.second;
						assert (index_vec.size() == 1);
						if (get<2>(rhs_key) && (stream_dim.compare (index_vec.front()) == 0) && temp_array)
							continue;
						string idx = index_vec.front ();
						rotations << "[";
						if (!code_diff) {
							rotations << idx;
							int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
							if (ufactor > 1)
								rotations << ufactor;	
					  }
						else 
							rotations << idx << "-" << idx << "0";	
						rotations << "]";
					}
				}
				else if (rhs_res == SHARED_MEM) {
					rotations << "_shm";
					if (get<2>(rhs_key)) {
						string append = extent_p1 == 0 ? "_c" : (extent_p1 > 0) ? "_p" : "_m";
						rotations << append << abs(extent_p1);
					}
					// Here comes the complicated part 
					for (auto ia : rhs_index_accesses) {
						vector<string> index_vec = ia.second;
						assert (index_vec.size() == 1);
						if (get<2>(rhs_key) && stream_dim.compare (index_vec.front()) == 0)
							continue;
						string idx = index_vec.front ();
						rotations << "[" << idx;
						int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
						if (ufactor > 1)
							rotations << ufactor;	
						rotations << "-" << idx << "0" << "]";
					}
				}
				else if (rhs_res == REGISTER) {
					rotations << "_reg";
					if (get<2>(rhs_key)) {
						string append = extent_p1 == 0 ? "_c" : (extent_p1 > 0) ? "_p" : "_m";
						rotations << append << abs(extent_p1);
						for (auto iter : iterators) {
							if ((udecls.find (iter) != udecls.end()) && (udecls[iter] > 1))
								rotations << "[l2_" << iter << udecls[iter] << "]";
						}
					}
				}
				rotations << ";\n";
			}
		}
	}

	// Print the loads
	print_loading_planes_without_halo (rotations, resource_extent, stmt_indent);

	// Close the if condition
	if (if_count != 0) {
		for (int cl=0; cl<for_count; cl++) 
			loop_end << indent;	 
		loop_end << dindent << "}\n";
	}
	// Print closing braces for the for loops
	if (for_count != 0) {
		for (int cl=for_count; cl>0; cl--) {
			for (int idt=0; idt<cl; idt++)
				loop_end << indent;
			loop_end << indent << "}\n";
		}
	}
	if (!(rotations.str()).empty()) 
		out << loop_begin.str() << rotations.str() << loop_end.str ();
}

// Print rotating planes when halo is non-zero, and LHS is shared memory, and RHS is global or shared memory
void stencilDefn::print_rotating_planes_with_halo (stringstream &out, map<tuple<string,bool>, tuple<int,int>> resource_extent) {
	stringstream loop_begin, rotations, loop_end;

	//loop_begin << dindent << "__syncthreads ();\n";
	loop_begin << "\n" << dindent << "//Shift planes with halo\n";

	// First generate the guard using stencil_hull
	map<string,int> udecls = get_unroll_decls ();
	vector<string> blockdims = (iterators.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iterators.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));

	stringstream for_out, if_out;
	int if_count = 0, for_count = 0;
	if_out << indent << "if (";
	vector<Range*>::iterator dom=initial_hull.begin();
	int pos = 0;
	for (vector<string>::iterator it=iterators.begin(); it!=iterators.end(); it++,dom++,pos++) {
		if (stream && stream_dim.compare (*it) == 0)
			continue;
		int ufactor = (udecls.find (*it) != udecls.end()) ? udecls[*it] : 1;
		int halo_lo = (halo_dims.find (*it) != halo_dims.end()) ? get<0>(halo_dims[*it]) : 0;
		int halo_hi = (halo_dims.find (*it) != halo_dims.end()) ? get<1>(halo_dims[*it]) : 0;

		if (ufactor == 1 && halo_lo == 0 && halo_hi == 0) {
			if (if_count != 0) 
				if_out << " && ";
			if_out << *it << " <= min(" << *it << "0+";
			if_out << "blockDim." << blockdims[pos] << "-1";
			if_out << ", ";
			exprNode *hi = (*dom)->get_hi_range ();
			string hi_id = "";
			int hi_val = 0;
			hi->decompose_access_fsm (hi_id, hi_val);
			if_out << hi_id; 
			if (!hi_id.empty() && hi_val >= 0)
				if_out << "+";
			if_out << hi_val << ")";
			if_count++;
		}
		else {
			int halo_size = halo_hi - halo_lo;
			int block_sz = (get_block_dims())[blockdims[pos]];
			int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
			string for_indent = indent;
			for (int idt=0; idt<=for_count; idt++)
				for_indent = for_indent + indent;
			for_out << for_indent << "#pragma unroll " << halo_ufactor << "\n";
			// Print initialization 
			for_out << for_indent << "for (int ";
			for_out << "l2_" << *it << halo_ufactor << " = 0, ";
			for_out << *it << halo_ufactor << " = ";
			for_out << *it << "0+(int)threadIdx." << blockdims[pos];
			for_out << "; ";
			// Print the higher bound for the for loop
			for_out << *it << halo_ufactor << " <= min(" << *it << "0+";
			if (ufactor > 1)
				for_out << ufactor << "*";
			for_out << "(int)blockDim." << blockdims[pos];
			// Account for the equality+1
			int ub = halo_hi - halo_lo - 1;
			if (ub > 0)
				for_out << "+" << ub;
			for_out<< ", ";
			exprNode *hi = (*dom)->get_hi_range ();
			string hi_id = "";
			int hi_val = 0;
			hi->decompose_access_fsm (hi_id, hi_val);
			for_out << hi_id; 
			if (!hi_id.empty() && hi_val > 0)
				for_out << "+";
			if (hi_val != 0) 
				for_out << hi_val;
			for_out << "); ";
			// Print increment
			for_out << "l2_" << *it << halo_ufactor << "++, ";
			for_out << *it << halo_ufactor << " += blockDim." << blockdims[pos] << ") {\n";
			for_count++;
		}
	}
	if_out << ") {\n";
	// Print the for loops
	if (for_count != 0) { 
		loop_begin << for_out.str ();
	}
	// Print if condition in the innermost loop
	if (if_count != 0) {
		for (int idt=0; idt<=for_count; idt++)
			loop_begin << indent;
		loop_begin << if_out.str ();
	}

	string stmt_indent = dindent;
	if (if_count != 0) 
		stmt_indent = stmt_indent + indent;
	for (int cl=0; cl<for_count; cl++) 
		stmt_indent = stmt_indent + indent;

	// Start printing rotating resource
	for (auto emap : resource_extent) {
		tuple<string,bool> first = emap.first;
		tuple<int,int> second = emap.second;
		string arr_name = trim_string (get<0>(first), "@");
		bool temp_array = is_temp_array (arr_name);
		for (int extent=get<0>(second); extent<get<1>(second); extent++) {
			string res_name = get<0>(first);
			bool lhs_resource_found = false, rhs_resource_found;
			RESOURCE lhs_res = REGISTER, rhs_res = REGISTER;
			int extent_p1 = extent + 1;
			// First find the resource for LHS
			tuple<string,int,bool> lhs_key = make_tuple (get<0>(first), extent, get<1>(first));
			if (get<1>(first)) {
				lhs_res = (resource_map.find (lhs_key) != resource_map.end ()) ? resource_map[lhs_key] : REGISTER;
				lhs_resource_found = true;
			}
			else if (resource_map.find (lhs_key) != resource_map.end ()) { 
				lhs_res = resource_map[lhs_key];
				lhs_resource_found = true;	
			}
			// Now find the resource for RHS 
			tuple<string,int,bool> rhs_key = make_tuple (get<0>(first), extent_p1, get<1>(first));
			if (get<1>(first)) {
				rhs_res = (resource_map.find (rhs_key) != resource_map.end ()) ? resource_map[rhs_key] : REGISTER;
				rhs_resource_found = true;
			}
			else if (resource_map.find (rhs_key) != resource_map.end ()) { 
				rhs_res = resource_map[rhs_key];
				rhs_resource_found = true;	
			}

			// If resource is found, print rotation
			if (lhs_resource_found && rhs_resource_found) {
				assert (access_stats.find (lhs_key) != access_stats.end ());
				map<int,vector<string>> lhs_index_accesses = get<2>(access_stats[lhs_key]);
				map<int,vector<string>> rhs_index_accesses = get<2>(access_stats[rhs_key]);
				// Check if LHS or RHS actually contain the streaming dimension
				bool found_stream_dim = temp_array;
				for (auto ia : lhs_index_accesses) {
					vector<string> index_vec = ia.second;
					assert (index_vec.size() == 1);
					if (get<2>(lhs_key) && (stream_dim.compare (index_vec.front()) == 0))
						found_stream_dim = true;
				}
				for (auto ia : rhs_index_accesses) {
					vector<string> index_vec = ia.second;
					assert (index_vec.size() == 1);
					if (get<2>(rhs_key) && (stream_dim.compare (index_vec.front()) == 0))
						found_stream_dim = true;
				}
				if (!found_stream_dim) 
					continue;
				// Print LHS
				rotations << stmt_indent << print_trimmed_string (get<0>(lhs_key), '@');
				if (lhs_res == GLOBAL_MEM) {
					// Here comes the complicated part 
					for (auto ia : lhs_index_accesses) {
						vector<string> index_vec = ia.second;
						assert (index_vec.size() == 1);
						if (get<2>(lhs_key) && (stream_dim.compare (index_vec.front()) == 0) && temp_array)
							continue;
						string idx = index_vec.front ();
						vector<string>::iterator iter_pos = find (iterators.begin(), iterators.end(), idx);
						assert (iter_pos != iterators.end ());
						int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
						int halo_lo = (halo_dims.find (idx) != halo_dims.end()) ? get<0>(halo_dims[idx]) : 0;
						int halo_hi = (halo_dims.find (idx) != halo_dims.end()) ? get<1>(halo_dims[idx]) : 0;
						int halo_size = halo_hi - halo_lo;
						int block_sz = (get_block_dims())[blockdims[iter_pos-iterators.begin()]];
						int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
						rotations << "[" << idx;
						if (halo_ufactor > 1)
							rotations << halo_ufactor;	
						rotations << "]";
					}
				}
				else if (lhs_res == SHARED_MEM) {
					rotations << "_shm";
					if (get<2>(lhs_key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						rotations << append << abs(extent);
					}
					// Here comes the complicated part 
					for (auto ia : lhs_index_accesses) {
						vector<string> index_vec = ia.second;
						assert (index_vec.size() == 1);
						if (get<2>(lhs_key) && stream_dim.compare (index_vec.front()) == 0)
							continue;
						string idx = index_vec.front ();
						vector<string>::iterator iter_pos = find (iterators.begin(), iterators.end(), idx);
						assert (iter_pos != iterators.end ());
						int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
						int halo_lo = (halo_dims.find (idx) != halo_dims.end()) ? get<0>(halo_dims[idx]) : 0;
						int halo_hi = (halo_dims.find (idx) != halo_dims.end()) ? get<1>(halo_dims[idx]) : 0;
						int halo_size = halo_hi - halo_lo;
						int block_sz = (get_block_dims())[blockdims[iter_pos-iterators.begin()]];
						int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
						rotations << "[" << idx;
						if (halo_ufactor > 1)
							rotations << halo_ufactor;
						rotations << "-" << idx << "0" << "]";
					}
				}
				rotations << " = ";
				// Print RHS
				rotations << print_trimmed_string (get<0>(rhs_key), '@');
				if (rhs_res == GLOBAL_MEM) {
					// Here comes the complicated part 
					for (auto ia : rhs_index_accesses) {
						vector<string> index_vec = ia.second;
						assert (index_vec.size() == 1);
						if (get<2>(rhs_key) && (stream_dim.compare (index_vec.front()) == 0) && temp_array)
							continue;
						string idx = index_vec.front ();
						vector<string>::iterator iter_pos = find (iterators.begin(), iterators.end(), idx);
						assert (iter_pos != iterators.end ());
						int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
						int halo_lo = (halo_dims.find (idx) != halo_dims.end()) ? get<0>(halo_dims[idx]) : 0;
						int halo_hi = (halo_dims.find (idx) != halo_dims.end()) ? get<1>(halo_dims[idx]) : 0;
						int halo_size = halo_hi - halo_lo;
						int block_sz = (get_block_dims())[blockdims[iter_pos-iterators.begin()]];
						int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
						rotations << "[";
						if (!code_diff) {	
							rotations << idx;
							if (halo_ufactor > 1)
								rotations << halo_ufactor;
						}
						else
							rotations << idx << "-" << idx << "0"; 	
						rotations << "]";
					}
				}
				else if (rhs_res == SHARED_MEM) {
					rotations << "_shm";
					if (get<2>(rhs_key)) {
						string append = extent_p1 == 0 ? "_c" : (extent_p1 > 0) ? "_p" : "_m";
						rotations << append << abs(extent_p1);
					}
					// Here comes the complicated part 
					for (auto ia : rhs_index_accesses) {
						vector<string> index_vec = ia.second;
						assert (index_vec.size() == 1);
						if (get<2>(rhs_key) && stream_dim.compare (index_vec.front()) == 0)
							continue;
						string idx = index_vec.front ();
						vector<string>::iterator iter_pos = find (iterators.begin(), iterators.end(), idx);
						assert (iter_pos != iterators.end ());
						int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
						int halo_lo = (halo_dims.find (idx) != halo_dims.end()) ? get<0>(halo_dims[idx]) : 0;
						int halo_hi = (halo_dims.find (idx) != halo_dims.end()) ? get<1>(halo_dims[idx]) : 0;
						int halo_size = halo_hi - halo_lo;
						int block_sz = (get_block_dims())[blockdims[iter_pos-iterators.begin()]];
						int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
						rotations << "[" << idx;
						if (halo_ufactor > 1)
							rotations << halo_ufactor;	
						rotations << "-" << idx << "0" << "]";
					}
				}
				rotations << ";\n";
			}
		}
	}

	// Print the loads
	print_loading_planes_with_halo (rotations, resource_extent, stmt_indent);

	// Close the if condition
	if (if_count != 0) {
		for (int cl=0; cl<for_count; cl++) 
			loop_end << indent;	 
		loop_end << dindent << "}\n";
	}
	// Print closing braces for the for loops
	if (for_count != 0) {
		for (int cl=for_count; cl>0; cl--) {
			for (int idt=0; idt<cl; idt++)
				loop_end << indent;
			loop_end << indent << "}\n";
		}
	}
	if (!(rotations.str()).empty()) 
		out << loop_begin.str() << rotations.str() << loop_end.str ();
}

void stencilDefn::print_rotating_planes_with_nonseparable_halo (stringstream &out, map<tuple<string,bool>, tuple<int,int>> resource_extent) {
	stringstream loop_begin, rotations, loop_end;

	//loop_begin << dindent << "__syncthreads ();\n";
	loop_begin << "\n" << dindent << "//Shift nonseparable planes with halo\n";

	// First generate the guard using stencil_hull
	map<string,int> udecls = get_unroll_decls ();
	vector<string> blockdims = (iterators.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iterators.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));
	map<string,string> addendum_conds;
	stringstream addendum;
	stringstream for_out, if_out;
	int if_count = 0, for_count = 0;
	if_out << indent << "if (";
	vector<Range*>::iterator dom=initial_hull.begin();
	int pos = 0;
	for (vector<string>::iterator it=iterators.begin(); it!=iterators.end(); it++,dom++,pos++) {
		if (stream && stream_dim.compare (*it) == 0)
			continue;
		int ufactor = (udecls.find (*it) != udecls.end()) ? udecls[*it] : 1;
		int halo_lo = (halo_dims.find (*it) != halo_dims.end()) ? get<0>(halo_dims[*it]) : 0;
		int halo_hi = (halo_dims.find (*it) != halo_dims.end()) ? get<1>(halo_dims[*it]) : 0;

		if (ufactor == 1) {
			if (if_count != 0) 
				if_out << " && ";
			if_out << *it << " <= min(" << *it << "0+";
			if_out << "blockDim." << blockdims[pos];
			//Account for equality
			int val = abs(halo_lo) - 1;
			if (val > 0)
				if_out << "+";
			if (val != 0)
				if_out << val;
			if_out << ", ";
			exprNode *hi = (*dom)->get_hi_range ();
			string hi_id = "";
			int hi_val = 0;
			hi->decompose_access_fsm (hi_id, hi_val);
			if_out << hi_id; 
			if (!hi_id.empty() && hi_val >= 0)
				if_out << "+";
			if_out << hi_val << ")";
			if_count++;
		}
		else {
			if (blocked_loads) {
				string for_indent = indent;
				for (int idt=0; idt<=for_count; idt++) 
					for_indent = for_indent + indent;
				// Print unroll pragma
				for_out << for_indent << "#pragma unroll " << ufactor << "\n";
				// Print initialization 
				for_out << for_indent << "for (int ";
				for_out << "l2_" << *it << ufactor << " = 0, ";
				for_out << *it << ufactor << " = " << *it << "; ";
				// Print condition
				for_out << "l2_" << *it << ufactor << " < " << ufactor << "; ";
				// Print increment
				for_out << "l2_" << *it << ufactor << "++, ";
				for_out << *it << ufactor << "++";
				for_out << ") {\n";
				for_count++;

				// Print if condition
				if (if_count != 0) 
					if_out << " && "; 
				//if_out << *it << ufactor << " <= min3(";
				//if_out << *it << "+" << ufactor-1 << ", ";
				if_out << *it << ufactor << " <= min(";
				if_out << *it << "0+";
				if_out << ufactor << "*";
				if_out << "blockDim." << blockdims[pos];
				//Account for equality
				int val = abs(halo_lo) - 1;
				if (val > 0)
					if_out << "+";
				if (val != 0)
					if_out << val;
				if_out << ", ";
				exprNode *hi = (*dom)->get_hi_range ();
				string hi_id = "";
				int hi_val = 0;
				hi->decompose_access_fsm (hi_id, hi_val);
				if_out << hi_id; 
				if (!hi_id.empty() && hi_val >= 0)
					if_out << "+";
				if_out << hi_val;
				if_out << ")";
				if_count++;
			}
			else {
				string for_indent = indent;
				for (int idt=0; idt<=for_count; idt++) 
					for_indent = for_indent + indent;
				// Print unroll pragma
				for_out << for_indent << "#pragma unroll " << ufactor << "\n";
				// Print initialization 
				for_out << for_indent << "for (int ";
				for_out << "l2_" << *it << ufactor << " = 0, ";
				for_out << *it << ufactor << " = " << *it << "; ";
				// Print condition
				for_out << "l2_" << *it << ufactor << " < " << ufactor << "; ";
				// Print increment
				for_out << "l2_" << *it << ufactor << "++, ";
				for_out << *it << ufactor << " += blockDim." << blockdims[pos];
				for_out << ") {\n";
				for_count++;

				// Print if condition
				if (if_count != 0) 
					if_out << " && "; 
				if_out << *it << ufactor << " <= min(" << *it << "0+";
				if_out << ufactor << "*";
				if_out << "blockDim." << blockdims[pos];
				//Account for equality
				int val = abs(halo_lo) - 1;
				if (val > 0)
					if_out << "+";
				if (val != 0)
					if_out << val;
				if_out << ", ";
				exprNode *hi = (*dom)->get_hi_range ();
				string hi_id = "";
				int hi_val = 0;
				hi->decompose_access_fsm (hi_id, hi_val);
				if_out << hi_id; 
				if (!hi_id.empty() && hi_val >= 0)
					if_out << "+";
				if_out << hi_val;
				if_out << ")";
				if_count++;
			}
		}
		// Create addendum conditions
		stringstream add_cond;
		if (ufactor == 1 && halo_hi == 0 && halo_lo == 0) {
			add_cond << *it << " <= min(" << *it << "0+";
			add_cond << "blockDim." << blockdims[pos];
			//Account for equality
			int val = abs(halo_lo) - 1;
			if (val > 0)
				add_cond << "+";
			if (val != 0)
				add_cond << val;
			add_cond << ", ";
			exprNode *hi = (*dom)->get_hi_range ();
			string hi_id = "";
			int hi_val = 0;
			hi->decompose_access_fsm (hi_id, hi_val);
			add_cond << hi_id; 
			if (!hi_id.empty() && hi_val >= 0)
				add_cond << "+";
			add_cond << hi_val << ")";
		}
		else {
			int halo_size = halo_hi - halo_lo;
			int block_sz = (get_block_dims())[blockdims[pos]];
			int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
			add_cond << indent << dindent << "#pragma unroll " << halo_ufactor << "\n";
			// Print initialization 
			add_cond << indent << dindent << "for (int ";
			add_cond << *it << halo_ufactor << " = ";
			add_cond << *it << "0+(int)threadIdx." << blockdims[pos];
			add_cond << "; ";
			// Print the higher bound for the for loop
			add_cond << *it << halo_ufactor << " <= min(" << *it << "0+";
			if (ufactor > 1)
				add_cond << ufactor << "*";
			add_cond << "(int)blockDim." << blockdims[pos];
			int ub = halo_hi - halo_lo - 1;
			// Account for the equality+1
			if (ub > 0) 
				add_cond << "+" << ub;
			add_cond<< ", ";
			exprNode *hi = (*dom)->get_hi_range ();
			string hi_id = "";
			int hi_val = 0;
			hi->decompose_access_fsm (hi_id, hi_val);
			add_cond << hi_id; 
			if (!hi_id.empty() && hi_val > 0)
				add_cond << "+";
			if (hi_val != 0) 
				add_cond << hi_val;
			add_cond << "); ";
			// Print increment
			add_cond << *it << halo_ufactor << " += blockDim." << blockdims[pos] << ")";
		}
		addendum_conds[*it] = add_cond.str ();
	}
	if_out << ") {\n";
	// Print the for loops
	if (for_count != 0) { 
		loop_begin << for_out.str ();
	}
	// Print if condition in the innermost loop
	if (if_count != 0) {
		for (int idt=0; idt<=for_count; idt++)
			loop_begin << indent;
		loop_begin << if_out.str ();
	}

	string stmt_indent = dindent;
	if (if_count != 0) 
		stmt_indent = stmt_indent + indent;
	for (int cl=0; cl<for_count; cl++) 
		stmt_indent = stmt_indent + indent;

	// Print the initializations now. 
	map<string,stringstream> addendum_if_lo, addendum_if_hi, addendum_for_lo, addendum_for_hi;
	map<string,vector<string>> addendum_load_lo, addendum_load_hi;
	map<string,bool> addendum_if_lo_computed, addendum_if_hi_computed;
	map<string,int> addendum_if_lo_count, addendum_if_hi_count, addendum_for_lo_count, addendum_for_hi_count;

	// Initialize the data structures	
	for (auto hd : halo_dims) {
		addendum_if_lo_computed[hd.first] = false;
		addendum_if_hi_computed[hd.first] = false;
		addendum_if_lo_count[hd.first] = 0;
		addendum_if_hi_count[hd.first] = 0;
		addendum_for_lo_count[hd.first] = 0;
		addendum_for_hi_count[hd.first] = 0;
	}

	// Start printing rotating resource
	for (auto emap : resource_extent) {
		tuple<string,bool> first = emap.first;
		tuple<int,int> second = emap.second;
		string arr_name = trim_string (get<0>(first), "@");
		bool temp_array = is_temp_array (arr_name);
		for (int extent=get<0>(second); extent<get<1>(second); extent++) {
			string res_name = get<0>(first);
			bool lhs_resource_found = false, rhs_resource_found;
			RESOURCE lhs_res = REGISTER, rhs_res = REGISTER;
			int extent_p1 = extent + 1;
			// First find the resource for LHS
			tuple<string,int,bool> lhs_key = make_tuple (get<0>(first), extent, get<1>(first));
			if (get<1>(first)) {
				lhs_res = (resource_map.find (lhs_key) != resource_map.end ()) ? resource_map[lhs_key] : REGISTER;
				lhs_resource_found = true;
			}
			else if (resource_map.find (lhs_key) != resource_map.end ()) { 
				lhs_res = resource_map[lhs_key];
				lhs_resource_found = true;	
			}
			// Now find the resource for RHS 
			tuple<string,int,bool> rhs_key = make_tuple (get<0>(first), extent_p1, get<1>(first));
			if (get<1>(first)) {
				rhs_res = (resource_map.find (rhs_key) != resource_map.end ()) ? resource_map[rhs_key] : REGISTER;
				rhs_resource_found = true;
			}
			else if (resource_map.find (rhs_key) != resource_map.end ()) { 
				rhs_res = resource_map[rhs_key];
				rhs_resource_found = true;	
			}

			// If resource is found, print rotation
			if (lhs_resource_found && rhs_resource_found) {
				assert (access_stats.find (lhs_key) != access_stats.end ());
				map<int,vector<string>> lhs_index_accesses = get<2>(access_stats[lhs_key]);
				map<int,vector<string>> rhs_index_accesses = get<2>(access_stats[rhs_key]);
				// Check if LHS or RHS actually contain the streaming dimension
				bool found_stream_dim = temp_array;
				for (auto ia : lhs_index_accesses) {
					vector<string> index_vec = ia.second;
					assert (index_vec.size() == 1);
					if (get<2>(lhs_key) && (stream_dim.compare (index_vec.front()) == 0))
						found_stream_dim = true;
				}
				for (auto ia : rhs_index_accesses) {
					vector<string> index_vec = ia.second;
					assert (index_vec.size() == 1);
					if (get<2>(rhs_key) && (stream_dim.compare (index_vec.front()) == 0))
						found_stream_dim = true;
				}
				if (!found_stream_dim) 
					continue;
				// Print LHS
				rotations << stmt_indent << print_trimmed_string (get<0>(lhs_key), '@');
				if (lhs_res == GLOBAL_MEM) {
					// Here comes the complicated part 
					for (auto ia : lhs_index_accesses) {
						vector<string> index_vec = ia.second;
						assert (index_vec.size() == 1);
						if (get<2>(lhs_key) && (stream_dim.compare (index_vec.front()) == 0) && temp_array)
							continue;
						string idx = index_vec.front ();
						rotations << "[" << idx;
						int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
						if (ufactor > 1)
							rotations << ufactor;	
						rotations << "]";
					}
				}
				else if (lhs_res == SHARED_MEM) {
					rotations << "_shm";
					if (get<2>(lhs_key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						rotations << append << abs(extent);
					}
					// Here comes the complicated part 
					for (auto ia : lhs_index_accesses) {
						vector<string> index_vec = ia.second;
						assert (index_vec.size() == 1);
						if (get<2>(lhs_key) && stream_dim.compare (index_vec.front()) == 0)
							continue;
						string idx = index_vec.front ();
						rotations << "[" << idx;
						int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
						if (ufactor > 1)
							rotations << ufactor;
						rotations << "-" << idx << "0" << "]";
					}
				}
				else if (lhs_res == REGISTER) {
					rotations << "_reg";
					if (get<2>(lhs_key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						rotations << append << abs(extent);
						for (auto iter : iterators) {
							if ((udecls.find (iter) != udecls.end()) && (udecls[iter] > 1))
								rotations << "[l2_" << iter << udecls[iter] << "]";
						}
					}
				}
				rotations << " = ";
				// Print RHS
				rotations << print_trimmed_string (get<0>(rhs_key), '@');
				if (rhs_res == GLOBAL_MEM) {
					// Here comes the complicated part 
					for (auto ia : rhs_index_accesses) {
						vector<string> index_vec = ia.second;
						assert (index_vec.size() == 1);
						if (get<2>(rhs_key) && (stream_dim.compare (index_vec.front()) == 0) && temp_array)
							continue;
						string idx = index_vec.front ();
						rotations << "[";
						if (!code_diff) {
							rotations << idx;
							int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
							if (ufactor > 1)
								rotations << ufactor;	
						}
						else
							rotations << idx << "-" << idx << "0";
						rotations << "]";
					}
				}
				else if (rhs_res == SHARED_MEM) {
					rotations << "_shm";
					if (get<2>(rhs_key)) {
						string append = extent_p1 == 0 ? "_c" : (extent_p1 > 0) ? "_p" : "_m";
						rotations << append << abs(extent_p1);
					}
					// Here comes the complicated part 
					for (auto ia : rhs_index_accesses) {
						vector<string> index_vec = ia.second;
						assert (index_vec.size() == 1);
						if (get<2>(rhs_key) && stream_dim.compare (index_vec.front()) == 0)
							continue;
						string idx = index_vec.front ();
						rotations << "[" << idx;
						int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
						if (ufactor > 1)
							rotations << ufactor;	
						rotations << "-" << idx << "0" << "]";
					}
				}
				else if (rhs_res == REGISTER) {
					rotations << "_reg";
					if (get<2>(rhs_key)) {
						string append = extent_p1 == 0 ? "_c" : (extent_p1 > 0) ? "_p" : "_m";
						rotations << append << abs(extent_p1);
						for (auto iter : iterators) {
							if ((udecls.find (iter) != udecls.end()) && (udecls[iter] > 1))
								rotations << "[l2_" << iter << udecls[iter] << "]";
						}
					}
				}
				rotations << ";\n";
			}

			// Compute the addendum loads at boundaries
			if (lhs_resource_found && rhs_resource_found && lhs_res == SHARED_MEM && halo_exists () && !is_temp_array (arr_name)) {
				stringstream load_lo, load_hi;
				assert (access_stats.find (lhs_key) != access_stats.end ());
				map<int,vector<string>> lhs_index_accesses = get<2>(access_stats[lhs_key]);
				map<int,vector<string>> rhs_index_accesses = get<2>(access_stats[rhs_key]);
				// Iterate over all the halos, and print the boundary loads for them
				for (auto hd : halo_dims) {
					// Continue only if hd appears in lhs_index_accesses
					bool found = false;
					for (auto ia : lhs_index_accesses) {
						vector<string> index_vec = ia.second;
						assert (index_vec.size() == 1);
						string idx = index_vec.front ();
						if (idx.compare (hd.first) == 0)
							found = true; 
					}
					if (!found || (get<0>(hd.second) == 0 && get<1>(hd.second) == 0)) 
						continue;
					// Print LHS
					load_lo << print_trimmed_string (get<0>(lhs_key), '@');
					load_hi << print_trimmed_string (get<0>(lhs_key), '@');
					load_lo << "_shm";
					load_hi << "_shm";
					if (get<2>(lhs_key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						load_lo << append << abs(extent);
						load_hi << append << abs(extent);
					}
					// Here comes the complicated part 
					for (auto ia : lhs_index_accesses) {
						vector<string> index_vec = ia.second;
						assert (index_vec.size() == 1);
						if (get<2>(lhs_key) && stream_dim.compare (index_vec.front()) == 0)
							continue;
						string idx = index_vec.front ();
						vector<string>::iterator iter_pos = find (iterators.begin(), iterators.end(), idx);
						int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
						int halo_lo = (halo_dims.find (idx) != halo_dims.end()) ? get<0>(halo_dims[idx]) : 0;
						int halo_hi = (halo_dims.find (idx) != halo_dims.end()) ? get<1>(halo_dims[idx]) : 0;
						assert (iter_pos != iterators.end ());
						if (!(halo_lo == 0 && halo_hi == 0) && (idx.compare(hd.first) == 0)) {
							// Print the lo condition
							if (!addendum_if_lo_computed[hd.first]) {
								if (addendum_if_lo_count[hd.first] != 0)
									addendum_if_lo[hd.first] << " && ";
								addendum_if_lo_count[hd.first]++;
								addendum_if_lo[hd.first] << "threadIdx." << blockdims[iter_pos-iterators.begin()];
								addendum_if_lo[hd.first] << " < " << abs (halo_lo);
							}
							// Print the hi condition
							if (!addendum_if_hi_computed[hd.first]) {
								if (addendum_if_hi_count[hd.first] != 0)
									addendum_if_hi[hd.first] << " && ";
								addendum_if_hi_count[hd.first]++;
								addendum_if_hi[hd.first] << "threadIdx." << blockdims[iter_pos-iterators.begin()];
								addendum_if_hi[hd.first] << " < " << abs (halo_hi);
							}
							// Print the hi and lo addendum loads
							load_lo << "[threadIdx." << blockdims[iter_pos-iterators.begin()] << "]";
							load_hi << "[";
							if (ufactor > 1) 
								load_hi << ufactor << "*";
							load_hi << "blockDim." << blockdims[iter_pos-iterators.begin()];
							if (halo_lo != 0)
								load_hi << "+" << abs(halo_lo);
							load_hi << "+threadIdx." << blockdims[iter_pos-iterators.begin()] << "]";
						}
						else {
							int halo_size = halo_hi - halo_lo;
							int block_sz = (get_block_dims())[blockdims[iter_pos-iterators.begin()]];
							int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);

							// Print the addendum lo and hi guard
							if (halo_ufactor == 1) {
								if (addendum_conds.find (idx) != addendum_conds.end ()) {
									if (!addendum_if_lo_computed[hd.first]) {
										if (addendum_if_lo_count[hd.first] != 0)
											addendum_if_lo[hd.first] << " && ";
										addendum_if_lo[hd.first] << addendum_conds[idx];
										addendum_if_lo_count[hd.first]++;
									}
									if (!addendum_if_hi_computed[hd.first]) {
										if (addendum_if_hi_count[hd.first] != 0)
											addendum_if_hi[hd.first] << " && ";
										addendum_if_hi[hd.first] << addendum_conds[idx];
										addendum_if_hi_count[hd.first]++;
									}
								}
							}
							else {
								if (addendum_conds.find (idx) != addendum_conds.end ()) {
									if (!addendum_if_lo_computed[hd.first]) {
										addendum_for_lo[hd.first] << addendum_conds[idx];
										addendum_for_lo_count[hd.first]++;
									}
									if (!addendum_if_hi_computed[hd.first]) {
										addendum_for_hi[hd.first] << addendum_conds[idx];
										addendum_for_hi_count[hd.first]++;
									}
								}
							}
							load_lo << "[" << idx;
							load_hi << "[" << idx;
							if (halo_ufactor > 1) {
								load_lo << halo_ufactor;
								load_hi << halo_ufactor;
							}
							load_lo << "-" << idx << "0" << "]";
							load_hi << "-" << idx << "0" << "]";
						}
					}
					load_lo << " = ";
					load_hi << " = ";
					if (rhs_res == SHARED_MEM) {
						load_lo << print_trimmed_string (get<0>(rhs_key), '@');
						load_hi << print_trimmed_string (get<0>(rhs_key), '@');
						load_lo << "_shm";
						load_hi << "_shm";
						if (get<2>(lhs_key)) {
							string append = extent_p1 == 0 ? "_c" : (extent_p1 > 0) ? "_p" : "_m";
							load_lo << append << abs(extent_p1);
							load_hi << append << abs(extent_p1);
						}
						// Here comes the complicated part 
						for (auto ia : lhs_index_accesses) {
							vector<string> index_vec = ia.second;
							assert (index_vec.size() == 1);
							if (get<2>(rhs_key) && stream_dim.compare (index_vec.front()) == 0)
								continue;
							string idx = index_vec.front ();
							vector<string>::iterator iter_pos = find (iterators.begin(), iterators.end(), idx);
							int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
							int halo_lo = (halo_dims.find (idx) != halo_dims.end()) ? get<0>(halo_dims[idx]) : 0;
							int halo_hi = (halo_dims.find (idx) != halo_dims.end()) ? get<1>(halo_dims[idx]) : 0;
							assert (iter_pos != iterators.end ());
							if (!(halo_lo == 0 && halo_hi == 0) && (idx.compare(hd.first) == 0)) {
								// Print the hi and lo addendum loads
								load_lo << "[threadIdx." << blockdims[iter_pos-iterators.begin()] << "]";
								load_hi << "[";
								if (ufactor > 1) 
									load_hi << ufactor << "*";
								load_hi << "blockDim." << blockdims[iter_pos-iterators.begin()];
								if (halo_lo != 0)
									load_hi << "+" << abs(halo_lo);
								load_hi << "+threadIdx." << blockdims[iter_pos-iterators.begin()] << "]";
							}
							else {
								int halo_size = halo_hi - halo_lo;
								int block_sz = (get_block_dims())[blockdims[iter_pos-iterators.begin()]];
								int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
								load_lo << "[" << idx;
								load_hi << "[" << idx;
								if (halo_ufactor > 1) {
									load_lo << halo_ufactor;
									load_hi << halo_ufactor;
								}
								load_lo << "-" << idx << "0" << "]";
								load_hi << "-" << idx << "0" << "]";
							}
						}
					}
					else {
						// Print RHS
						load_lo << arr_name;
						load_hi << arr_name;
						// Here comes the complicated part 
						for (auto ia : rhs_index_accesses) {
							vector<string> index_vec = ia.second;
							assert (index_vec.size() == 1);
							if (get<2>(rhs_key) && (stream_dim.compare (index_vec.front()) == 0) && temp_array)
								continue;
							string idx = index_vec.front ();
							int ufactor = (udecls.find (idx) != udecls.end()) ? udecls[idx] : 1;
							int halo_lo = (halo_dims.find (idx) != halo_dims.end()) ? get<0>(halo_dims[idx]) : 0;
							int halo_hi = (halo_dims.find (idx) != halo_dims.end()) ? get<1>(halo_dims[idx]) : 0;
							vector<string>::iterator iter_pos = find (iterators.begin(), iterators.end(), idx);
							if (!(halo_lo == 0 && halo_hi == 0) && (idx.compare(hd.first)== 0)) {
								// Print the addendum lo and hi loads
								load_lo << "[" << idx << "0+threadIdx." << blockdims[iter_pos-iterators.begin()] << "]"; 
								load_hi << "[" << idx << "0+";
								if (ufactor > 1)
									load_hi << ufactor << "*";
								load_hi << "blockDim." << blockdims[iter_pos-iterators.begin()];
								if (halo_lo != 0)
									load_hi << "+" << abs (halo_lo);
								load_hi << "+threadIdx." << blockdims[iter_pos-iterators.begin()] << "]";	
							}
							else {
								// Print the addendum lo and hi loads
								int halo_size = halo_hi - halo_lo;
								int block_sz = (get_block_dims())[blockdims[iter_pos-iterators.begin()]];
								int halo_ufactor = ufactor + (int) ceil((double)halo_size/(double)block_sz);
								load_lo << "[" << idx;
								load_hi << "[" << idx;
								if (halo_ufactor > 1) {
									load_lo << halo_ufactor;
									load_hi << halo_ufactor;
								}
								if (get<2>(rhs_key) && (stream_dim.compare (index_vec.front()) == 0)) {
									if (extent_p1 > 0) {
										load_lo << "+";
										load_hi << "+";
									}
									if (extent_p1 != 0) {
										load_lo << extent_p1;
										load_hi << extent_p1;
									}
								}
								load_lo << "]";
								load_hi << "]";
							}
						}
					}
					addendum_load_lo[hd.first].push_back (load_lo.str());
					addendum_load_hi[hd.first].push_back (load_hi.str());

					if (!(addendum_if_lo[hd.first].str()).empty ())
						addendum_if_lo_computed[hd.first] = true;
					if (!(addendum_if_hi[hd.first].str()).empty ())
						addendum_if_hi_computed[hd.first] = true;
				}
			}
		}
	}

	// Print the loads
	string loading_halo_planes = print_loading_planes_with_nonseparable_halo (rotations, resource_extent, stmt_indent);

	for (auto hd : halo_dims) {
		if (get<0>(hd.second) == 0 && get<1>(hd.second) == 0)
			continue;
		if (!(addendum_if_lo[hd.first].str()).empty ()) {
			addendum << dindent << "if (" << addendum_if_lo[hd.first].str () << ") {\n";
			if (!(addendum_for_lo[hd.first].str()).empty())
				addendum << addendum_for_lo[hd.first].str () << " {\n";
			for (auto ad_lo : addendum_load_lo[hd.first]) {
				for (int cl=0; cl<=addendum_for_lo_count[hd.first]; cl++)
					addendum << indent;
				addendum << dindent << ad_lo << ";\n";
			}
			if (!(addendum_for_lo[hd.first].str()).empty())
				addendum << dindent << indent << "}\n"; 
			addendum << dindent << "}\n";
		}
		if (!(addendum_if_hi[hd.first].str()).empty()) {
			addendum << dindent << "if (" << addendum_if_hi[hd.first].str () << ") {\n";
			if (!(addendum_for_hi[hd.first].str()).empty())
				addendum << addendum_for_hi[hd.first].str () << " {\n";
			for (auto ad_hi : addendum_load_hi[hd.first]) {
				for (int cl=0; cl<=addendum_for_hi_count[hd.first]; cl++)
					addendum << indent;
				addendum << dindent << ad_hi << ";\n";
			}
			if (!(addendum_for_lo[hd.first].str()).empty())
				addendum << dindent << indent << "}\n"; 
			addendum << dindent << "}\n";
		}
	}

	// Close the if condition
	if (if_count != 0) {
		for (int cl=0; cl<for_count; cl++) 
			loop_end << indent;	 
		loop_end << dindent << "}\n";
	}
	// Print closing braces for the for loops
	if (for_count != 0) {
		for (int cl=for_count; cl>0; cl--) {
			for (int idt=0; idt<cl; idt++)
				loop_end << indent;
			loop_end << indent << "}\n";
		}
	}
	if (!(rotations.str()).empty()) 
		out << loop_begin.str() << rotations.str() << loop_end.str ();

	// Print the load at boundaries
	if (!(addendum.str()).empty ())
		out << addendum.str ();

	// Print the loading of halo planes
	if (!loading_halo_planes.empty())
		out << loading_halo_planes;
}

void stencilDefn::print_declarations (string stencil_name, stringstream &out) {
	out << indent << "//Array and variable declarations\n";
	// List all the arrays that access global memory in the resource map
	vector<string> global_accesses;
	for (auto rmap : resource_map) {
		if (rmap.second == GLOBAL_MEM) {
			string name = get<0>(rmap.first);
			if (find (global_accesses.begin(), global_accesses.end(), name) == global_accesses.end())
				global_accesses.push_back (name);
		}
	}
	// List all the reads and writes. 
	set<string> reads, writes;
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	for (auto st : stmts) {
		for (auto stmt : decomposed_stmts[st->get_stmt_num()]) {
			exprNode *lhs = stmt->get_lhs_expr ();
			exprNode *rhs = stmt->get_rhs_expr ();
			rhs->compute_rbw (reads, writes, true);
			if (stmt->get_op_type () != ST_EQ)
				lhs->compute_rbw (reads, writes, true);
			lhs->compute_rbw (reads, writes, false);
		}
	}

	// First print all the resources that are temporary, and not a part of the arg
	for (auto ad : array_decls) {
		string name = ad->get_array_name ();
		if (ad->is_temp_array () && !is_stencil_arg (name)) {
			if (find (global_accesses.begin(), global_accesses.end(), name) != global_accesses.end()) {
				// Declare an array of the size of the block
				DATA_TYPE type = ad->get_array_type ();
				out << indent << print_data_type (type) << name;
				vector<string> blockdims = (iterators.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iterators.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));
				// Get the index accesses for this array
				map<int,vector<string>> index_accesses;
				for (auto stats: access_stats) {
					tuple<string,int,bool> first = stats.first;
					map<int,vector<string>> second = get<2>(stats.second);
					if (name.compare (trim_string(get<0>(first),"@")) == 0) {
						for (auto s : second) {
							if (index_accesses.find (s.first) == index_accesses.end()) 
								index_accesses[s.first] = s.second;
							else {
								for (auto val : s.second) {
									if (find (index_accesses[s.first].begin(), index_accesses[s.first].end(), val) == index_accesses[s.first].end())
										index_accesses[s.first].push_back (val); 
								}
							}
						}
					}
				}
				vector<Range*> range = ad->get_array_range ();
				map<string,int> udecls = get_unroll_decls ();
				int pos=0;
				for (vector<Range*>::iterator it=range.begin(); it!=range.end(); it++,pos++) {
					exprNode *lo = (*it)->get_lo_range ();
					exprNode *hi = (*it)->get_hi_range ();
					string lo_id = "", hi_id = "";
					int lo_val = 0, hi_val = 0;
					lo->decompose_access_fsm (lo_id, lo_val);
					hi->decompose_access_fsm (hi_id, hi_val);
					if (lo_id.empty () && hi_id.empty ()) {
						int size = hi_val - lo_val;
						if (size != 0) 
							out << "[" << size << "]";
					}
					else {
						int size = 0;
						vector<string> index_vec = index_accesses[pos];
						for (auto iv : index_vec) {
							if (stream && (iv.compare (stream_dim) == 0))
								continue;
							int ufactor = 1;
							if (udecls.find (iv) != udecls.end())
								ufactor = udecls[iv];
							// Factor the halo 
							vector<string>::iterator jt = find (iterators.begin(), iterators.end(), iv);
							int block_sz = (get_block_dims())[blockdims[jt-iterators.begin()]];
							int halo_sz = (halo_dims.find (iv) != halo_dims.end ()) ? (get<1>(halo_dims[iv])-get<0>(halo_dims[iv])) : 0;
							size = max (size, ufactor*block_sz+halo_sz);
						}
						if (size != 0) 
							out << "[" << size << "]";
					}
				}
				out << ";\n";
			}
		}
	}
	for (auto vd : get_var_decls ()) {
		string name = vd.first;
		if (!is_stencil_arg (name)) {
			out << indent << print_data_type (vd.second) << name;
			out << ";\n";
		}
	}
	for (auto vd : var_init_decls) {
		out << indent << print_data_type (get<0>(vd));
		out << get<1>(vd);
		out << " = ";
		(get<2>(vd))->print_node (out);
		out << ";\n";	
	}

	// Now print the resouces from resource map
	map<tuple<string,bool>, tuple<int,int>> resource_extent;
	for (auto rmap : resource_map) {
		if (rmap.second == GLOBAL_MEM) 
			continue;
		tuple<string,int,bool> first = rmap.first;
		int extent = get<1>(first);
		tuple<string,bool> key = make_tuple (get<0>(first), get<2>(first));
		int lo_extent=extent, hi_extent=extent;
		if (resource_extent.find (key) != resource_extent.end ()) {
			tuple<int,int> value = resource_extent[key];
			lo_extent = min (get<0>(value), extent);
			hi_extent = max (get<1>(value), extent);
		}
		resource_extent[key] = make_tuple (lo_extent, hi_extent);
	}
	if (DEBUG) {
		cout << "\nPrinting the resource extent for stencil " << stencil_name << " : \n";
		for (auto emap : resource_extent) {
			cout << indent;
			tuple<string,bool> first = emap.first;
			tuple<int,int> second = emap.second;
			if (get<1>(first)) 
				cout << "stream : ";
			cout << get<0>(first) << " = ";
			cout << get<0>(second) << " : " << get<1>(second) << endl;		
		}
	}
	// Fill all resources from -extent to +extent, missing values will be assigned registers for streaming
	map<string,int> udecls = get_unroll_decls ();
	for (auto emap : resource_extent) {
		tuple<string,bool> first = emap.first;
		tuple<int,int> second = emap.second;
		// Check if there's any shared memory or register to be printed
		bool print_resource = false;
		for (int extent=get<0>(second); extent<=get<1>(second); extent++) {
			RESOURCE res;
			tuple<string,int,bool> key = make_tuple (get<0>(first), extent, get<1>(first));
			if (resource_map.find (key) != resource_map.end ()) {
				res = resource_map[key];
				if (res == SHARED_MEM || res == REGISTER) {
					print_resource = true;
					break;
				}
			}
		}
		if (!print_resource) 
			continue;

		// If there are registers/shared memory accesses, declare them
		string name = trim_string (get<0>(first), "@");
		if (is_array_decl (name)) {
			DATA_TYPE type = get_array_type (name);
			out << indent << print_data_type (type);
		}
		else if (is_var_decl (name)) {
			DATA_TYPE type = get_var_type (name);
			out << indent << print_data_type (type);
		}
		bool first_decl = true, printed_shmem = false;
		for (int extent=get<0>(second); extent<=get<1>(second); extent++) {
			bool resource_found = false;
			RESOURCE res;
			tuple<string,int,bool> key = make_tuple (get<0>(first), extent, get<1>(first));
			if (get<1>(first)) {
				res = (resource_map.find (key) != resource_map.end ()) ? resource_map[key] : REGISTER;
				resource_found = true;
			}
			else if (resource_map.find (key) != resource_map.end ()) { 
				res = resource_map[key];
				resource_found = true;	
			}
			// Only continue if the resource is found, and its not global memory	
			if (resource_found && (res!=GLOBAL_MEM)) {
				if (printed_shmem && res==REGISTER) {
					out << ";\n";
					string name = trim_string (get<0>(first), "@");
					if (is_array_decl (name)) {
						DATA_TYPE type = get_array_type (name);
						out << indent << print_data_type (type);
					}
					else if (is_var_decl (name)) {
						DATA_TYPE type = get_var_type (name);
						out << indent << print_data_type (type);
					}
					// Reset
					printed_shmem = false;
					first_decl = true;
				}

				if (first_decl)
					first_decl = false;
				else 
					out << ", ";
				if (res == SHARED_MEM)
					out << "__shared__ ";
				out << print_trimmed_string (get<0>(key), '@');
				name = trim_string (get<0>(key), "@");
				vector<string> blockdims = (iterators.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iterators.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));
				// Print declaration for the resource
				if (res == SHARED_MEM) {
					printed_shmem = true;
					out << "_shm";
					if (get<2>(key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						out << append << abs(extent);
					}
					map<int,vector<string>> index_accesses;
					for (auto stats: access_stats) {
						tuple<string,int,bool> first = stats.first;
						map<int,vector<string>> second = get<2>(stats.second);
						if (name.compare (trim_string(get<0>(first),"@")) == 0) {
							for (auto s : second) {
								if (index_accesses.find (s.first) == index_accesses.end()) 
									index_accesses[s.first] = s.second;
								else {
									for (auto val : s.second) {
										if (find (index_accesses[s.first].begin(), index_accesses[s.first].end(), val) == index_accesses[s.first].end())
											index_accesses[s.first].push_back (val); 
									}
								}
							}
						}
					}
					vector<Range*> range = get_array_range (name);
					int pos=0;
					for (vector<Range*>::iterator it=range.begin(); it!=range.end(); it++,pos++) {
						exprNode *lo = (*it)->get_lo_range ();
						exprNode *hi = (*it)->get_hi_range ();
						string lo_id = "", hi_id = "";
						int lo_val = 0, hi_val = 0;
						lo->decompose_access_fsm (lo_id, lo_val);
						hi->decompose_access_fsm (hi_id, hi_val);
						bool print_size = !(lo_id.empty () && hi_id.empty ());
						if (stream) {
							print_size &= !((lo_id.empty () || lo_id.compare (stream_dim) == 0) && 
									(hi_id.empty () || hi_id.compare (stream_dim) == 0));
						}
						if (print_size) {
							int size = 0;
							vector<string> index_vec = index_accesses[pos];
							for (auto iv : index_vec) {
								if (stream && (iv.compare (stream_dim) == 0))
									continue;
								int ufactor = 1;
								if (udecls.find (iv) != udecls.end ())
									ufactor = udecls[iv];
								vector<string>::iterator jt = find (iterators.begin(), iterators.end(), iv);
								int block_sz = (get_block_dims())[blockdims[jt-iterators.begin()]];
								int halo_sz = (halo_dims.find (iv) != halo_dims.end ()) ? (get<1>(halo_dims[iv])-get<0>(halo_dims[iv])) : 0;
								size = max (size, ufactor*block_sz+halo_sz);
							}
							if (size != 0)
								out << "[" << size << "]";
						}
					}
				}
				else if (res == REGISTER) {
					out << "_reg";
					if (get<2>(key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						out << append << abs(extent);
					}
					bool scalar = true;
					for (vector<string>::iterator iter=iterators.begin(); iter!=iterators.end(); iter++) {
						int ufactor = (udecls.find (*iter) != udecls.end()) ? udecls[*iter] : 1;
						if (ufactor > 1)
							out << "[" << ufactor << "]";
						scalar &= (ufactor == 1);
					}
					if (retiming_feasible) {
						if (scalar) 
							out << "=0";
						else 
							out << "={0}";
					}
				}
				// Print a prefetch register if prefetching is turned on, and this is the highest extent plane
				if (prefetch && (extent == get<1>(second)) && (reads.find (name) != reads.end ())) {
					if (printed_shmem) {
						out << ";\n";
						string name = trim_string (get<0>(first), "@");
						if (is_array_decl (name)) {
							DATA_TYPE type = get_array_type (name);
							out << indent << print_data_type (type);
						}
						else if (is_var_decl (name)) {
							DATA_TYPE type = get_var_type (name);
							out << indent << print_data_type (type);
						}
						// Reset
						printed_shmem = false;
						first_decl = true;
					}
					if (!first_decl)
						out << ", ";
					out << print_trimmed_string (get<0>(key), '@');
					out << "_prefetch";
					if (get<2>(key)) {
						string append = extent == 0 ? "_c" : (extent > 0) ? "_p" : "_m";
						out << append << abs(extent);
					}
					int pos = 0;
					for (vector<string>::iterator iter=iterators.begin(); iter!=iterators.end(); iter++,pos++) {
						int ufactor = (udecls.find (*iter) != udecls.end()) ? udecls[*iter] : 1;
						int halo_lo = (halo_dims.find (*iter) != halo_dims.end()) ? get<0>(halo_dims[*iter]) : 0;
						int halo_hi = (halo_dims.find (*iter) != halo_dims.end()) ? get<1>(halo_dims[*iter]) : 0;
						int halo_size = halo_hi - halo_lo;
						int block_sz = (get_block_dims())[blockdims[pos]];
						int halo_ufactor = (res == REGISTER) ? ufactor : ufactor + (int) ceil((double)halo_size/(double)block_sz);
						if (halo_ufactor > 1)
							out << "[" << halo_ufactor << "]";
					}
				}
			}
		}
		out << ";\n";
	}
	out << endl;
}

void funcDefn::print_constant_memory (stringstream &out) {
	// Print the arrays in constant memory
	for (auto cst : get_constants()) {
		if (is_array_decl (cst)) {
			DATA_TYPE t = get_array_type (cst);
			out << "__constant__ " << print_data_type (t);
			out << cst;
			// Print the bounds
			vector<Range*> range = get_array_range (cst);
			for (auto r : range) {
				exprNode *lo = r->get_lo_range ();
				exprNode *hi = r->get_hi_range ();
				binaryNode *temp = new binaryNode (T_MINUS, hi, lo, INT, true);
				string id = "";
				int val = 0;
				temp->decompose_access_fsm (id, val);
				// LHS
				out << "[";
				if (!id.empty ())
					out << id <<id;
				if (!id.empty () && val > 0)
					out << "+";
				if (val != 0)
					out << val;
				out << "]";
			}
			out << ";\n";
		}
	}
}

void stencilDefn::print_stencil_header (string name, stringstream &out) {
	stringstream cast_out;
	out << "\n__global__ void " << name << " (";
	vector<string> args = get_arg_list ();
	// Print the arguments
	bool first_arg = true; 
	for (vector<string>::iterator a=args.begin(); a!=args.end(); a++) {
		if (is_array_decl (*a)) {
			// If the array is in constant memory, skip
			bool is_constant = false;
			for (auto cst : constants) {
				if (cst.compare (*a) == 0)
					is_constant = true;
			}
			if (!is_constant) {
				if (!first_arg) 
					out << ", ";
				else 
					first_arg = false;
				DATA_TYPE type = get_array_type (*a);
				vector<Range*> arr_range = get_array_range (*a);
				out << print_data_type (type);
				out << "* __restrict__ ";
				if (arr_range.size () > 1)
					out << "d_";
				out << *a;
				// Create a cast expression for the array
				if (arr_range.size () > 1) {
					// Print accesses
					stringstream accesses;
					for (vector<Range*>::iterator it=next(arr_range.begin()); it!=arr_range.end(); it++) {
						exprNode *lo = (*it)->get_lo_range ();
						exprNode *hi = (*it)->get_hi_range ();
						binaryNode *temp = new binaryNode (T_MINUS, hi, lo, INT, true);
						string id = "";
						int val = 0;
						temp->decompose_access_fsm (id, val);
						// LHS
						accesses << "[";
						if (!id.empty ())
							accesses << id <<id;
						if (!id.empty () && val > 0)
							accesses << "+";
						if (val != 0)
							accesses << val;
						accesses << "]";
					}
					// Print LHS
					cast_out << indent << print_data_type (type);
					cast_out << "(*" << *a << ")";
					cast_out << accesses.str() << " = ";
					// Print RHS
					cast_out << "(" << print_data_type (type) << "(*)" << accesses.str() << ") ";
					cast_out << "d_" << *a << ";\n"; 
				}
			}
		}
		else if (is_var_decl (*a)) {
			if (!first_arg) 
				out << ", ";
			else 
				first_arg = false;
			DATA_TYPE type = get_var_type (*a);
			out << print_data_type (type);
			out << *a;
		}
	}
	for (auto p : parameters) {
		if (find (args.begin(), args.end(), p) == args.end()) {
			out << ", int " << p;
		}
	}
	out << ") {\n";
	// Print the blockdims
	vector<string> blockdims = (iterators.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iterators.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));
	map<string,int> udecls = get_unroll_decls ();
	out << indent << "//Determining the block's indices\n";
	int pos = dim-1;
	for (vector<string>::reverse_iterator it=iterators.rbegin(); it!=iterators.rend(); it++,pos--) {
		if (full_stream && stream_dim.compare (*it) == 0)
			continue;
		Range *dim_range = recompute_hull[pos];
		exprNode *lo = dim_range->get_lo_range ();
		exprNode *hi = dim_range->get_hi_range ();
		string lo_id = "", hi_id = "";
		int lo_val = 0, hi_val = 0;
		lo->decompose_access_fsm (lo_id, lo_val);
		hi->decompose_access_fsm (hi_id, hi_val);
		int offset = hi_val - lo_val + 1;
		int ufactor = 1;
		if (udecls.find (*it) != udecls.end ())
			ufactor = udecls[*it];
		out << indent << "int " << *it << "0 = (int)(blockIdx." << blockdims[pos] << ")*(";
		if (stream && stream_dim.compare (*it) == 0) {
			out << (get_block_dims())[blockdims[pos]];
		}
		else {
			if (ufactor > 1) 
				out << ufactor << "*";
			out << "(int)blockDim." << blockdims[pos];
			if (offset < 0)
				out << offset;
		}
		out << ")";
		// Shift the start by the lo range of initial_hull
		Range *init_range = initial_hull[pos];
		exprNode *init_lo = init_range->get_lo_range ();	
		string init_lo_id = "";
		int init_lo_val = 0;
		init_lo->decompose_access_fsm (init_lo_id, init_lo_val);
		if (!(init_lo_id.empty () && (init_lo_val == 0))) {
			if (!init_lo_id.empty ()) 
				out << "+" << init_lo_id;
			if (init_lo_val > 0) 
				out << "+";
			if (init_lo_val != 0)
				out << init_lo_val;	
		}
		out << ";\n";
		if (!(stream && stream_dim.compare (*it) == 0)) {
			out << indent << "int " << *it << " = " << *it << "0";
			int halo_lo = (halo_dims.find (*it) != halo_dims.end()) ? get<0>(halo_dims[*it]) : 0;
			if (halo_lo != 0)
				out << "+" << abs(halo_lo);
			out << " + (int)(";
			if (ufactor > 1 && blocked_loads) 
				out << ufactor << "*";
			out << "threadIdx." << blockdims[pos] << ");\n";
		}
	}
	out << "\n" << cast_out.str () << "\n";
}

void stencilDefn::print_rotating_planes (stringstream &out) {
	// Print only if we are streaming
	if (!stream) 
		return;

	// Begin by computing the extent map, and the minimum extent
	map<tuple<string,bool>, tuple<int,int>> resource_extent;
	for (auto rmap : resource_map) {
		tuple<string,int,bool> first = rmap.first;
		string arr_name = trim_string (get<0>(first), "@");
		if (rmap.second == GLOBAL_MEM && !is_temp_array (arr_name))
			continue;
		int extent = get<1>(first);
		tuple<string,bool> key = make_tuple (get<0>(first), get<2>(first));
		int lo_extent=extent, hi_extent=extent;
		if (resource_extent.find (key) != resource_extent.end ()) {
			tuple<int,int> value = resource_extent[key];
			lo_extent = min (get<0>(value), extent);
			hi_extent = max (get<1>(value), extent);
		}
		resource_extent[key] = make_tuple (lo_extent, hi_extent);
	}

	// If halo does not exist, simply print the planes
	if (!halo_exists ()) {
		if (use_shmem)
			out << dindent << "__syncthreads ();\n";
		print_rotating_planes_without_halo (out, resource_extent);
		out << indent << "}\n";
		return;
	}

	// Otherwise find if the planes are separable or non-separable
	bool separable = true;
	for (auto emap : resource_extent) {
		RESOURCE common_res = GLOBAL_MEM;
		bool common_res_set = false, same_res = true;
		tuple<string,bool> first = emap.first;
		tuple<int,int> second = emap.second;
		string arr_name = trim_string (get<0>(first), "@");
		for (int extent=get<0>(second); extent<=get<1>(second); extent++) {
			string res_name = get<0>(first);
			bool lhs_resource_found = false, rhs_resource_found = false;
			RESOURCE lhs_res = REGISTER, rhs_res = REGISTER;
			// First find the resource for LHS
			tuple<string,int,bool> lhs_key = make_tuple (get<0>(first), extent, get<1>(first));
			if (get<1>(first)) {
				lhs_res = (resource_map.find (lhs_key) != resource_map.end ()) ? resource_map[lhs_key] : REGISTER;
				lhs_resource_found = true;
			}
			else if (resource_map.find (lhs_key) != resource_map.end ()) { 
				lhs_res = resource_map[lhs_key];
				lhs_resource_found = true;	
			}
			if (extent < get<1>(second)) {
				// Now find the resource for RHS
				int extent_p1 = extent + 1; 
				tuple<string,int,bool> rhs_key = make_tuple (get<0>(first), extent_p1, get<1>(first));
				if (get<1>(first)) {
					rhs_res = (resource_map.find (rhs_key) != resource_map.end ()) ? resource_map[rhs_key] : REGISTER;
					rhs_resource_found = true;
				}
				else if (resource_map.find (rhs_key) != resource_map.end ()) {
					rhs_res = resource_map[rhs_key];
					rhs_resource_found = true;
				}
				// We cannot separate if LHS is shared memory, and RHS is register
				if (rhs_resource_found) {
					if (lhs_res == SHARED_MEM && rhs_res == REGISTER)
						separable &= false;	
				}
			}
			if (lhs_resource_found) {
				if (!common_res_set) {
					common_res = lhs_res;
					common_res_set = true;
				}
				same_res &= (common_res == lhs_res);
			}
		}
		separable &= same_res;
	}

	if (separable) {
		// Separate the resources into shared memory and registers
		map<tuple<string,bool>, tuple<int,int>> reg_resource_extent, shm_resource_extent;
		for (auto rmap : resource_map) {
			tuple<string,int,bool> first = rmap.first;
			string arr_name = trim_string (get<0>(first), "@");
			if (rmap.second == GLOBAL_MEM && !is_temp_array (arr_name))
				continue;
			int extent = get<1>(first);
			tuple<string,bool> key = make_tuple (get<0>(first), get<2>(first));
			int lo_extent=extent, hi_extent=extent;
			if (rmap.second == SHARED_MEM) {	
				if (shm_resource_extent.find (key) != shm_resource_extent.end ()) {
					tuple<int,int> value = shm_resource_extent[key];
					lo_extent = min (get<0>(value), extent);
					hi_extent = max (get<1>(value), extent);
				}
				shm_resource_extent[key] = make_tuple (lo_extent, hi_extent);
			}
			else if (rmap.second == REGISTER) {	
				if (reg_resource_extent.find (key) != reg_resource_extent.end ()) {
					tuple<int,int> value = reg_resource_extent[key];
					lo_extent = min (get<0>(value), extent);
					hi_extent = max (get<1>(value), extent);
				}
				reg_resource_extent[key] = make_tuple (lo_extent, hi_extent);
			}
		}
		if (use_shmem)
			out << dindent << "__syncthreads ();\n";
		print_rotating_planes_without_halo (out, reg_resource_extent);
		print_rotating_planes_with_halo (out, shm_resource_extent);
		out << indent << "}\n";
	}
	else {
		// Create the minimal non-separable part, and the separable part (shm and reg)
		map<tuple<string,bool>, tuple<int,int>> sep_shm_resource_extent, nonsep_resource_extent;
		for (auto emap : resource_extent) {
			RESOURCE common_res = REGISTER;
			bool common_res_set = false, same_res = true;
			tuple<string,bool> first = emap.first;
			tuple<int,int> second = emap.second;
			string arr_name = trim_string (get<0>(first), "@");
			for (int extent=get<0>(second); extent<=get<1>(second); extent++) {
				string res_name = get<0>(first);
				bool lhs_resource_found = false, rhs_resource_found = false;
				RESOURCE lhs_res = REGISTER, rhs_res = REGISTER;
				// First find the resource for LHS
				tuple<string,int,bool> lhs_key = make_tuple (get<0>(first), extent, get<1>(first));
				if (get<1>(first)) {
					lhs_res = (resource_map.find (lhs_key) != resource_map.end ()) ? resource_map[lhs_key] : REGISTER;
					lhs_resource_found = true;
				}
				else if (resource_map.find (lhs_key) != resource_map.end ()) {
					lhs_res = resource_map[lhs_key];
					lhs_resource_found = true;
				}
				if (extent < get<1>(second)) {
					// Now find the resource for RHS
					int extent_p1 = extent + 1;
					tuple<string,int,bool> rhs_key = make_tuple (get<0>(first), extent_p1, get<1>(first));
					if (get<1>(first)) {
						rhs_res = (resource_map.find (rhs_key) != resource_map.end ()) ? resource_map[rhs_key] : REGISTER;
						rhs_resource_found = true;
					}
					else if (resource_map.find (rhs_key) != resource_map.end ()) {
						rhs_res = resource_map[rhs_key];
						rhs_resource_found = true;
					}
					// We cannot separate if LHS is shared memory, and RHS is register
					if (rhs_resource_found) {
						if (lhs_res == SHARED_MEM && rhs_res == REGISTER)
							separable &= false;
					}
				}
				if (lhs_resource_found) {
					if (!common_res_set) {
						common_res = lhs_res;
						common_res_set = true;
					}
					same_res &= (common_res == lhs_res);
				}
			}
			// Decide which category this extent belongs to
			if (same_res && (common_res == SHARED_MEM)) {
				sep_shm_resource_extent[first] = second;
			}
			else {
				nonsep_resource_extent[first] = second;
			}
		}
		// Now print the resources
		if (use_shmem)
			out << dindent << "__syncthreads ();\n";
		if (!sep_shm_resource_extent.empty())
			print_rotating_planes_with_halo (out, sep_shm_resource_extent);
		if (!nonsep_resource_extent.empty())	
			print_rotating_planes_with_nonseparable_halo (out, nonsep_resource_extent);
		out << indent << "}\n";
	}
}

void stencilDefn::print_mapped_stencil_stmts (stringstream &out, vector<string> copyout) {
	// Find the initial reads.
	string stmt_indent = stream ? dindent : indent;
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	set<string> reads, writes;
	for (auto stmt : stmts) {
		exprNode *lhs = stmt->get_lhs_expr ();
		exprNode *rhs = stmt->get_rhs_expr ();
		rhs->compute_rbw (reads, writes, true);
		if (stmt->get_op_type () != ST_EQ)
			lhs->compute_rbw (reads, writes, true);
		lhs->compute_rbw (reads, writes, false);
	}
	// Compute the temporary arrays
	vector<string> temp_arrays;
	for (auto ad : array_decls) {
		if (ad->is_temp_array ()) {
			temp_arrays.push_back (ad->get_array_name ());
		}
	}
	string recompute_hi_bound = "", recompute_lo_bound = "";
	int recompute_hi_val = 0, recompute_lo_val = 0;

	//First print the streaming iterator
	if (stream) {
		int min_extent = 0, max_extent = 0;
		for (auto srmap : stmt_resource_map) {
			int stmt_num = srmap.first;
			if ((plane_offset.find (stmt_num) != plane_offset.end ()) && (plane_offset[stmt_num] != 0))
				continue;
			for (auto rmap : srmap.second) {
				tuple<string,int,bool> first = rmap.first;
				string arr_name = trim_string (get<0>(first), "@");
				if (reads.find (arr_name) == reads.end ())
					continue;
				int extent = get<1>(first);
				min_extent = min (min_extent, extent);
				max_extent = max (max_extent, extent); 
			}
		}
		min_extent = abs (min_extent);
		max_extent = abs (max_extent);
		// Compute the hi and lo range of streaming dimension
		vector<Range*>::iterator dom = initial_hull.begin();
		string hi_bound = "", lo_bound = "";
		int hi_val = 0, lo_val = 0, pos = 0;
		vector<string> blockdims = (iterators.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iterators.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));
		for (vector<string>::iterator it=iterators.begin(); it!=iterators.end(); it++,dom++,pos++) {
			if (stream_dim.compare (*it) != 0)
				continue;
			exprNode *hi = (*dom)->get_hi_range();
			hi->decompose_access_fsm (hi_bound, hi_val);
			exprNode *lo = (*dom)->get_lo_range();
			lo->decompose_access_fsm (lo_bound, lo_val);
			break;
		}
		vector<Range*>::iterator recompute_dom = recompute_hull.begin();
		for (vector<string>::iterator it=iterators.begin(); it!=iterators.end(); it++,recompute_dom++) {
			if (stream_dim.compare (*it) != 0)
				continue;
			exprNode *hi = (*recompute_dom)->get_hi_range();
			hi->decompose_access_fsm (recompute_hi_bound, recompute_hi_val);
			exprNode *lo = (*recompute_dom)->get_lo_range();
			lo->decompose_access_fsm (recompute_lo_bound, recompute_lo_val);
			break;
		}
		out << indent << "#pragma unroll " << stream_uf << "\n";
		// Initialization of for loop
		if (full_stream)
			out << indent << "for (int " << stream_dim << "=";
		else
			out << indent << "for (int " << stream_dim << "=" << stream_dim << "0+";
		if (!lo_bound.empty ()) 
			out << lo_bound;
		lo_val = full_stream ? lo_val + min_extent : min_extent;
		if (!lo_bound.empty () && lo_val >= 0) 
			out << "+";
		out << lo_val << "; ";
		// Conditional
		hi_val = hi_val - max_extent;
		out << stream_dim << "<=";
		if (full_stream) {
			if (!hi_bound.empty ()) 
				out << hi_bound;
			if (!hi_bound.empty () && hi_val >= 0)
				out << "+";
			out << hi_val << "; ";
		}
		else {
			out << "min (";
			if (!hi_bound.empty ()) 
				out << hi_bound;
			if (!hi_bound.empty () && hi_val >= 0)
				out << "+";
			out << hi_val << ", ";
			// Print the bound;
			out << stream_dim << "0+" << (get_block_dims())[blockdims[pos]] + lo_val + (retiming_feasible ? abs(recompute_lo_val)+abs(recompute_hi_val) : 0) - 1;
			out << "); ";	
		}
		// Increment expression
		out << stream_dim << "++) {\n";
	}
	if (use_shmem)
		out << stmt_indent << "__syncthreads ();\n";

	if (prefetch) { 
		if (halo_exists ()) 
			print_prefetch_loads_with_halo (out);
		print_prefetch_loads_without_halo (out);
	}

	// Compute the last writes
	map<string,exprNode*> last_writes;
	for (auto st : stmts) {
		for (auto s : decomposed_stmts[st->get_stmt_num()]) {
			exprNode *lhs = s->get_lhs_expr ();
			lhs->compute_last_writes (last_writes, stream, stream_dim);
		}
	}
	// Remove the temporary arrays from last writes
	for (auto ad : array_decls) {
		if (ad->is_temp_array () && (last_writes.find (ad->get_array_name()) != last_writes.end()))
			last_writes.erase (ad->get_array_name ()); 
	}
	// If inouts are defined, take the intersection
	vector<string> inouts = get_inout_list ();
	if (!inouts.empty()) {
		for (map<string,exprNode*>::iterator it=last_writes.begin(); it!=last_writes.end();) {
			if (find (inouts.begin(), inouts.end(), it->first) == inouts.end ()) {
				it = last_writes.erase (it);
			}
			else 
				++it;
		}
	}
	//// Remove arrays that are not copy-out.
	//for (map<string,exprNode*>::iterator it=last_writes.begin(); it!=last_writes.end();) {
	//	if (find (copyout.begin(), copyout.end(), it->first) == copyout.end ()) {
	//		it = last_writes.erase (it);
	//	}
	//	else 
	//		++it;
	//}
	// Iterate over all statements, and cluster them based on their domain and dependences
	// Start from backwards. Stop exploring when you find a source of dependence
	vector<int> pointwise_occurrences;
	compute_pointwise_occurrences (pointwise_occurrences);
	vector<tuple<vector<Range*>, vector<Range*>, vector<stmtNode*>>> cluster_tuple;
	for (auto stmt : stmts) {
		int stmt_num = stmt->get_stmt_num ();
		vector<Range*> res_hull = get<0>(resource_hull[stmt_num]);
		vector<Range*> st_hull = get<0>(stmt_hull[stmt_num]);

		bool found = false;
		vector<tuple<vector<Range*>, vector<Range*>, vector<stmtNode*>>>::reverse_iterator ctuple;
		for (ctuple = cluster_tuple.rbegin(); ctuple != cluster_tuple.rend(); ctuple++) {
			bool check_dom_equality = (res_hull.size () == get<0>(*ctuple).size ());
			check_dom_equality &= (st_hull.size () == get<1>(*ctuple).size ());

			bool raw_dep_found = false;
			if (check_dom_equality) {
				// Check that there is no raw dependence between the cluster entries and this statement
				for (auto entries : get<2>(*ctuple)) {
					if (raw_dependence_exists (entries->get_stmt_num (), stmt_num)) {
						raw_dep_found = true;
						break;
					}
				}
				// Now see if the statement can be put in this cluster 
				bool same_dom = true;
				vector<string>::iterator it=iterators.begin();
				vector<Range*>::iterator b_dom = get<0>(*ctuple).begin();
				vector<Range*>::iterator c_dom=st_hull.begin();
				vector<Range*>::iterator d_dom = get<1>(*ctuple).begin();
				for (vector<Range*>::iterator a_dom=res_hull.begin(); a_dom!=res_hull.end(); a_dom++,b_dom++,c_dom++,d_dom++,it++) {
					if (stream && (stream_dim.compare (*it) == 0))
						continue;
					// Check if lo is same
					exprNode *a_lo_expr = (*a_dom)->get_lo_range ();
					exprNode *b_lo_expr = (*b_dom)->get_lo_range ();
					same_dom &= a_lo_expr->same_expr (b_lo_expr);
					// Check if hi is same
					exprNode *a_hi_expr = (*a_dom)->get_hi_range ();
					exprNode *b_hi_expr = (*b_dom)->get_hi_range ();
					same_dom &= a_hi_expr->same_expr (b_hi_expr);
					// Check if lo is same
					exprNode *c_lo_expr = (*c_dom)->get_lo_range ();
					exprNode *d_lo_expr = (*d_dom)->get_lo_range ();
					same_dom &= c_lo_expr->same_expr (d_lo_expr);
					// Check if hi is same
					exprNode *c_hi_expr = (*c_dom)->get_hi_range ();
					exprNode *d_hi_expr = (*d_dom)->get_hi_range ();
					same_dom &= c_hi_expr->same_expr (d_hi_expr);
				}
				if (same_dom) {
					// Make an entry into the cluster entry	
					vector<stmtNode*> &entries = get<2>(*ctuple);
					entries.push_back (stmt);
					get<2>(*ctuple) = entries;
					found = true;
					break;
				}
			}
			// Bail if RAW dependences were found, since moving upwards will violate them
			if (raw_dep_found)
				break;
		}
		// If not found, then make an entry into the cluster_tuple
		if (!found) {
			vector<stmtNode*> entry = {stmt};
			cluster_tuple.push_back (make_tuple (res_hull, st_hull, entry));
		}
	}

	vector<int> stmts_visited;

	if (generate_if) {
		// Now print the statements
		for (vector<tuple<vector<Range*>, vector<Range*>, vector<stmtNode*>>>::iterator ctuple=cluster_tuple.begin(); ctuple!=cluster_tuple.end(); ctuple++) {
			vector<stmtNode*> stmt_list = get<2>(*ctuple);
			string stmt_indent = stream ? indent + dindent : dindent;
			string loop_indent = stream ? dindent : indent;
			// Print the domain
			map<string,int> udecls = get_unroll_decls ();
			vector<string> blockdims = (iterators.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iterators.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));

			// Create all possible unrolling scenarios
			vector<map<string,int>> unroll_perm = create_permutations (udecls, iterators);
			cout << "Printing the unroll instances\n";
			for (auto up_end : unroll_perm) {
				for (auto u : up_end) {
					cout << u.first << "-" << u.second << " ";
				}
				cout << endl;
			}

			// Print syncthreads if this cluster has any dependence to the previously visited statements
			bool print_syncthread = false;
			for (auto v : stmts_visited) {
				for (auto s : stmt_list) {
					int dest_num = s->get_stmt_num ();
					if (raw_dependence_exists (v,dest_num) || war_dependence_exists (v,dest_num) || waw_dependence_exists (v,dest_num)) 
						print_syncthread = true;
				}
			}
			if (print_syncthread) {
				out << loop_indent << "__syncthreads ();\n";
				stmts_visited.clear ();
			}

			// up_end indicates the end limit of the if condition 
			int first_if = true;
			for (auto up_end : unroll_perm) {
				if (DEBUG) {
					cout << " unrolling end limit = " ; 
					for (auto p : up_end) 
						cout << p.first << ":" << p.second << " ";
					cout << endl;
				}
				// Recreate unroll factors by incrementing the extents of up_end
				map<string,int> up_copy;
				for (auto p : up_end) {
					up_copy[p.first] = p.second + 1;
				}
				// This will be used to both print the unrolled statements, and as start of the if condition
				vector<map<string,int>> stmt_perm = create_permutations (up_copy, iterators);

				// Explore different start limits with the end limit up_end
				for (vector<map<string,int>>::reverse_iterator up_start=stmt_perm.rbegin(); up_start!=stmt_perm.rend(); up_start++) { 
					stringstream concurrent_stream_shift;
					concurrent_stream_shift.str("");
					if (first_if) {
						out << loop_indent << "if (";
						first_if = false;
					}
					else 
						out << loop_indent << "else if (";
					vector<Range*>::iterator res_dom = get<0>(*ctuple).begin();
					vector<Range*>::iterator st_dom  = get<1>(*ctuple).begin();
					vector<Range*>::iterator init_dom = initial_hull.begin();
					int pos = 0;
					for (vector<string>::iterator it=iterators.begin(); it!=iterators.end(); it++,res_dom++,st_dom++,init_dom++,pos++) {
						if (stream && stream_dim.compare (*it) == 0) {
							if (!full_stream) {
								concurrent_stream_shift << "min (";
								exprNode *init_hi = (*init_dom)->get_hi_range ();
								string init_hi_id = "";
								int init_hi_val = 0;
								init_hi->decompose_access_fsm (init_hi_id, init_hi_val);
								if (!init_hi_id.empty ())
									concurrent_stream_shift << init_hi_id; 
								if (!init_hi_id.empty() && init_hi_val > 0)
									out << "+";
								if (init_hi_val != 0) 
									concurrent_stream_shift << init_hi_val;
								concurrent_stream_shift << ", ";
								concurrent_stream_shift << *it << "0+" << abs(recompute_lo_val) << ")";
								cout << "Concurrent stream = " << concurrent_stream_shift.str() << "\n";	
							}
							continue;
						}
						int uf_start = (*up_start)[*it];
						int uf_end = up_end[*it];
						int ufactor = (udecls.find (*it) != udecls.end()) ? udecls[*it] : 1;
						int halo_lo = (halo_dims.find (*it) != halo_dims.end()) ? get<0>(halo_dims[*it]) : 0;

						// Get the lo and hi bounds of st_dom 
						exprNode *st_lo = (*st_dom)->get_lo_range ();
						string st_lo_id = "";
						int st_lo_val = 0;
						st_lo->decompose_access_fsm (st_lo_id, st_lo_val);
						exprNode *st_hi = (*st_dom)->get_hi_range ();
						string st_hi_id = "";
						int st_hi_val = 0;
						st_hi->decompose_access_fsm (st_hi_id, st_hi_val);
						// Get the lo and hi bounds of res_dom
						exprNode *res_lo = (*res_dom)->get_lo_range ();
						string res_lo_id = "";
						int res_lo_val = 0;
						res_lo->decompose_access_fsm (res_lo_id, res_lo_val);
						exprNode *res_hi = (*res_dom)->get_hi_range ();
						string res_hi_id = "";
						int res_hi_val = 0;
						res_hi->decompose_access_fsm (res_hi_id, res_hi_val);
						// Now print the lo bounds
						out << *it;
						if (blocked_loads) {
							if (uf_start > 0)
								out << "+" << uf_start;
						}
						else {
							if (uf_start > 0) {
								out << "+";
								if (uf_start > 1)
									out << uf_start << "*";
								out << "blockDim." << blockdims[pos];
							}
						}
						out << " >= ";
						if (!st_lo_id.empty() || (st_lo_val != 0)) {
							out << "max(";
							if (!st_lo_id.empty ())
								out << st_lo_id;
							if (!st_lo_id.empty() && st_lo_val > 0)
								out << "+";
							if (st_lo_val != 0)
								out << st_lo_val;
							out << ", ";
						}
						out << *it << "0";
						if (!res_lo_id.empty ())
							out << "+" << res_lo_id;
						//Account for halo_lo;
						res_lo_val += abs(halo_lo);
						if (res_lo_val > 0)
							out << "+";
						if (res_lo_val != 0)
							out << res_lo_val;
						if (!st_lo_id.empty() || (st_lo_val != 0)) 
							out << ")";	
						out << " && ";
						// Now print hi bound
						out << *it;
						if (blocked_loads) {
							if (uf_end > 0)
								out << "+" << uf_end;
						}
						else {
							if (uf_end > 0) {
								out << "+";
								if (uf_end > 1)
									out << uf_end << "*";
								out << "blockDim." << blockdims[pos];
							}
						}
						out << " <= ";
						if (!st_hi_id.empty() || (st_hi_val != 0)) {
							out << "min(";
						}
						out << *it << "0+";
						if (ufactor > 1) 
							out << ufactor << "*";
						out << "blockDim." << blockdims[pos];
						if (!res_hi_id.empty ())
							out << "+" << res_hi_id;
						//Account for halo_lo;
						res_hi_val += abs(halo_lo);
						if (res_hi_val > 0)
							out << "+"; 
						if (res_hi_val != 0)
							out << res_hi_val;
						if (!st_hi_id.empty() || (st_hi_val != 0)) {
							out << ", ";
							if (!st_hi_id.empty ())
								out << st_hi_id; 
							if (!st_hi_id.empty() && st_hi_val > 0)
								out << "+";
							if (st_hi_val != 0) 
								out << st_hi_val;
							out << ")";
						}
						if (it != prev (iterators.end())) 
							out << " && ";	
					}
					out << ") {\n";

					// Print statements with unrolling
					vector<map<string,int>> uf_printed;
					for (auto up_instance : stmt_perm) {
						// Print only if up_instance is lexicographically higher than up_start   
						if (lexicographically_low (up_instance, *up_start, iterators))
							continue;
						if (find (uf_printed.begin(), uf_printed.end(), up_instance) != uf_printed.end()) {
							continue; 
						}
						for (auto full_stmt : stmt_list) {
							stmts_visited.push_back (full_stmt->get_stmt_num());
							for (auto stmt : decomposed_stmts[full_stmt->get_stmt_num()]) {
								stringstream lhs_out, rhs_out;
								exprNode *lhs = stmt->get_lhs_expr ();
								exprNode *rhs = stmt->get_rhs_expr ();
								STMT_OP op_type = stmt->get_op_type ();
								string cstream_shift = concurrent_stream_shift.str();
								rhs->print_node (rhs_out, resource_map, false, full_stream, stream, stream_dim, temp_arrays, up_instance, blocked_loads, udecls, parameters, iterators, linearize_accesses, code_diff, cstream_shift);
								lhs->print_node (lhs_out, resource_map, true,  full_stream, stream, stream_dim, temp_arrays, up_instance, blocked_loads, udecls, parameters, iterators, linearize_accesses, code_diff, cstream_shift);
								out << stmt_indent << lhs_out.str () << print_stmt_op (op_type) << rhs_out.str () << ";\n";
								// If lhs is last write, print it
								stringstream write_out;
								lhs->print_last_write (write_out, resource_map, last_writes, full_stream, stream, stream_dim, temp_arrays, up_instance, blocked_loads, udecls, parameters, iterators, linearize_accesses, code_diff, cstream_shift);
								if (!(write_out.str()).empty())
									out << stmt_indent << write_out.str ();
							}
						}
						uf_printed.push_back (up_instance);
					}
					out << loop_indent << "}\n";
				}
			}
		}
	}
	else {
		int reg_count = 0;
		for (vector<tuple<vector<Range*>, vector<Range*>, vector<stmtNode*>>>::iterator ctuple=cluster_tuple.begin(); ctuple!=cluster_tuple.end(); ctuple++,reg_count++) {
			vector<stmtNode*> stmt_list = get<2>(*ctuple);
			string loop_indent = stream ? dindent : indent;
			string stmt_indent = stream ? dindent : indent;
			// Print the domain
			map<string,int> udecls = get_unroll_decls ();
			vector<string> blockdims = (iterators.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iterators.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));

			// Print syncthreads if this cluster has any dependence to the previously visited statements
			bool print_syncthread = false;
			for (auto v : stmts_visited) {
				for (auto s : stmt_list) {
					int dest_num = s->get_stmt_num ();
					if (raw_dependence_exists (v,dest_num) || war_dependence_exists (v,dest_num) || waw_dependence_exists (v,dest_num)) 
						print_syncthread = true;
				}
			}
			if (print_syncthread) {
				out << loop_indent << "__syncthreads ();\n";
				stmts_visited.clear (); 
			} 

			// Consider unrolling
			int pos = 0;
			stringstream concurrent_stream_shift;
			concurrent_stream_shift.str("");
			vector<Range*>::iterator res_dom = get<0>(*ctuple).begin();
			vector<Range*>::iterator st_dom  = get<1>(*ctuple).begin();
			vector<Range*>::iterator init_dom = initial_hull.begin();
			stringstream for_out, if_out;
			int if_count = 0, for_count = 0;
			if_out << loop_indent << "if (";
			for (vector<string>::iterator it=iterators.begin(); it!=iterators.end(); it++,res_dom++,st_dom++,init_dom++,pos++) {
				if (stream && stream_dim.compare (*it) == 0) {
					if (!full_stream) {
						concurrent_stream_shift << "min (";
						exprNode *init_hi = (*init_dom)->get_hi_range ();
						string init_hi_id = "";
						int init_hi_val = 0;
						init_hi->decompose_access_fsm (init_hi_id, init_hi_val);
						if (!init_hi_id.empty ())
							concurrent_stream_shift << init_hi_id;
						if (!init_hi_id.empty() && init_hi_val > 0)
							out << "+";
						if (init_hi_val != 0)
							concurrent_stream_shift << init_hi_val;
						concurrent_stream_shift << ", ";
						concurrent_stream_shift << *it << "0+" << abs(recompute_lo_val) << ")";
						cout << "Concurrent stream = " << concurrent_stream_shift.str() << "\n";
					}
					continue;
				}
				int ufactor = (udecls.find (*it) != udecls.end()) ? udecls[*it] : 1;
				int halo_lo = (halo_dims.find (*it) != halo_dims.end()) ? get<0>(halo_dims[*it]) : 0;

				// Get the lo and hi bounds of st_dom 
				exprNode *st_lo = (*st_dom)->get_lo_range ();
				string st_lo_id = "";
				int st_lo_val = 0;
				st_lo->decompose_access_fsm (st_lo_id, st_lo_val);
				exprNode *st_hi = (*st_dom)->get_hi_range ();
				string st_hi_id = "";
				int st_hi_val = 0;
				st_hi->decompose_access_fsm (st_hi_id, st_hi_val);
				// Get the lo and hi bounds of res_dom
				exprNode *res_lo = (*res_dom)->get_lo_range ();
				string res_lo_id = "";
				int res_lo_val = 0;
				res_lo->decompose_access_fsm (res_lo_id, res_lo_val);
				exprNode *res_hi = (*res_dom)->get_hi_range ();
				string res_hi_id = "";
				int res_hi_val = 0;
				res_hi->decompose_access_fsm (res_hi_id, res_hi_val);

				if (ufactor == 1) {
					if (if_count != 0) 
						if_out << " && ";
					// Now print the lo bounds
					if_out << *it << " >= ";
					if (!st_lo_id.empty() || (st_lo_val != 0)) {
						if_out << "max(";
						if (!st_lo_id.empty ())
							if_out << st_lo_id;
						if (!st_lo_id.empty() && st_lo_val > 0)
							if_out << "+";
						if (st_lo_val != 0)
							if_out << st_lo_val;
						if_out << ", ";
					}
					if_out << *it << "0";
					if (!res_lo_id.empty ())
						if_out << "+" << res_lo_id;
					//Account for halo_lo;
					res_lo_val += abs(halo_lo);
					if (res_lo_val > 0)
						if_out << "+";
					if (res_lo_val != 0)
						if_out << res_lo_val;
					if (!st_lo_id.empty() || (st_lo_val != 0)) 
						if_out << ")";	
					if_out << " && ";
					// Now print hi bound
					if_out << *it;
					if_out << " <= ";
					if (!st_hi_id.empty() || (st_hi_val != 0)) {
						if_out << "min(";
					}
					if_out << *it << "0+";
					if (ufactor > 1) 
						if_out << ufactor << "*";
					if_out << "blockDim." << blockdims[pos];
					if (!res_hi_id.empty ())
						if_out << "+" << res_hi_id;
					//Account for halo_lo;
					res_hi_val += abs(halo_lo);
					if (res_hi_val > 0)
						if_out << "+"; 
					if (res_hi_val != 0)
						if_out << res_hi_val;
					if (!st_hi_id.empty() || (st_hi_val != 0)) {
						if_out << ", ";
						if (!st_hi_id.empty ())
							if_out << st_hi_id; 
						if (!st_hi_id.empty() && st_hi_val > 0)
							if_out << "+";
						if (st_hi_val != 0) 
							if_out << st_hi_val;
						if_out << ")";
					}
					if_count++;
				}
				else {
					string for_indent = loop_indent; 
					for (int idt=0; idt<for_count; idt++) 
						for_indent = for_indent + indent;
					for_out << for_indent << "#pragma unroll " << ufactor << "\n";
					// Print initialization
					for_out << for_indent << "for (int ";
					for_out << "r" << reg_count << "_" << *it << ufactor << " = 0, ";
					for_out << *it << ufactor << " = " << *it;
					for_out << "; ";
					// Print condition
					for_out << "r" << reg_count << "_" << *it << ufactor << " < " << ufactor;
					for_out << "; ";
					// Print increment
					for_out << "r" << reg_count << "_" << *it << ufactor << "++, "; 
					for_out << *it << ufactor;
					if (blocked_loads) {
						for_out << "++";
					}
					else {
						for_out << "+=" << "blockDim." << blockdims[pos];
					}
					for_out << ") {\n";
					for_count++;

					// Print the lower and upper bounds for if condition
					if (if_count != 0)
						if_out << " && ";
					if_out << *it << ufactor;
					if_out << " >= ";
					// Print the lo bound
					if (!st_lo_id.empty() || (st_lo_val != 0)) {
						if_out << "max(";
						if (!st_lo_id.empty ())
							if_out << st_lo_id;
						if (!st_lo_id.empty() && st_lo_val > 0)
							if_out << "+";
						if (st_lo_val != 0)
							if_out << st_lo_val;
						if_out << ", ";
					}
					if_out << *it << "0";
					if (!res_lo_id.empty ())
						if_out << "+" << res_lo_id;
					//Account for halo_lo;
					res_lo_val += abs(halo_lo);
					if (res_lo_val > 0)
						if_out << "+";
					if (res_lo_val != 0)
						if_out << res_lo_val;
					if (!st_lo_id.empty() || (st_lo_val != 0))
						if_out << ")";
					if_out << " && ";
					// Print the hi bound
					if_out << *it << ufactor;
					if_out << " <= ";
					if (!st_hi_id.empty() || (st_hi_val != 0)) {
						//if (blocked_loads)
						//	if_out << "min3(";
						//else
						//	if_out << "min(";
						if_out << "min(";
					}
					//if (blocked_loads) {
					//	if_out << *it << "+" << ufactor-1 << ", ";
					//}
					if_out << *it << "0+";
					if (ufactor > 1) 
						if_out << ufactor << "*";
					if_out << "blockDim." << blockdims[pos];
					if (!res_hi_id.empty ())
						if_out << "+" << res_hi_id;
					//Account for halo_lo;
					res_hi_val += abs(halo_lo);
					if (res_hi_val > 0)
						if_out << "+"; 
					if (res_hi_val != 0)
						if_out << res_hi_val;
					if (!st_hi_id.empty() || (st_hi_val != 0)) {
						if_out << ", ";
						if (!st_hi_id.empty ())
							if_out << st_hi_id; 
						if (!st_hi_id.empty() && st_hi_val > 0)
							if_out << "+";
						if (st_hi_val != 0) 
							if_out << st_hi_val;
						if_out << ")";
					}
					if_count++;
				}
			}
			if_out << ") {\n";
			// Print the for loops
			if (for_count != 0) { 
				out << for_out.str ();
			}
			// Print if condition in the innermost loop
			if (if_count != 0) {
				if (for_count != 0) {
					for (int idt=0; idt<for_count; idt++)
						out << indent;
				}
				out << if_out.str ();
			}

			if (if_count != 0) 
				stmt_indent = stmt_indent + indent;
			if (for_count != 0) {
				for (int cl=0; cl<for_count; cl++) 
					stmt_indent = stmt_indent + indent;
			}

			for (auto full_stmt : stmt_list) {
				stmts_visited.push_back (full_stmt->get_stmt_num());
				for (auto stmt : decomposed_stmts[full_stmt->get_stmt_num()]) {
					stringstream lhs_out, rhs_out;
					exprNode *lhs = stmt->get_lhs_expr ();
					exprNode *rhs = stmt->get_rhs_expr ();
					STMT_OP op_type = stmt->get_op_type ();
					string cstream_shift = concurrent_stream_shift.str();
					rhs->print_node (rhs_out, resource_map, false, full_stream, stream, stream_dim, temp_arrays, blocked_loads, udecls, parameters, iterators, linearize_accesses, code_diff, halo_dims, reg_count, cstream_shift);
					lhs->print_node (lhs_out, resource_map, true,  full_stream, stream, stream_dim, temp_arrays, blocked_loads, udecls, parameters, iterators, linearize_accesses, code_diff, halo_dims, reg_count, cstream_shift);
					out << stmt_indent << lhs_out.str () << print_stmt_op (op_type) << rhs_out.str () << ";\n";
					// If lhs is last write, print it
					stringstream write_out;
					lhs->print_last_write (write_out, resource_map, last_writes, full_stream, stream, stream_dim, temp_arrays, blocked_loads, udecls, parameters, iterators, linearize_accesses, code_diff, halo_dims, reg_count, cstream_shift);
					if (!(write_out.str()).empty())
						out << stmt_indent << write_out.str();
				}
			}
			// Close the if condition
			if (if_count != 0) {
				for (int cl=0; cl<for_count; cl++)
					out << indent;
				out << loop_indent << "}\n";
			}
			// Print closing braces for the for loops
			if (for_count != 0) {
				for (int cl=for_count; cl>0; cl--) {
					for (int idt=0; idt<cl-1; idt++)
						out << indent;
					out << loop_indent << "}\n";
				}
			}
		}
	}
}

void stencilDefn::compute_dimensionality (void) {
	vector<string> iters = get_iterators ();
	dim = iters.size(); 
	// Iterate over each statement, and compute the dimensionality
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	for (auto s : stmts) {
		exprNode *lhs = s->get_lhs_expr ();
		exprNode *rhs = s->get_rhs_expr ();
		int lhs_dim = lhs->compute_dimensionality (iters);
		int rhs_dim = rhs->compute_dimensionality (iters);
		int stmt_dim = max (lhs_dim, rhs_dim);
		dim = max (stmt_dim, dim);
	}
}

void stencilDefn::determine_stmt_resource_mapping (void) {
	// Iterate over each statement, and determine its resource mapping
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	for (auto st : stmts) {
		map<tuple<string,int,bool>,RESOURCE> resource_map;
		for (auto s : decomposed_stmts[st->get_stmt_num()]) {	
			exprNode *lhs = s->get_lhs_expr ();
			exprNode *rhs = s->get_rhs_expr ();
			lhs->determine_stmt_resource_mapping (resource_map, iterators, stream, stream_dim, use_shmem);
			rhs->determine_stmt_resource_mapping (resource_map, iterators, stream, stream_dim, use_shmem);
		}
		stmt_resource_map[st->get_stmt_num()] = resource_map;
	}
}

void stencilDefn::print_stmt_resource_map (string name) {
	cout << "\nResource map for stencil " << name << " :\n";
	for (auto srmap : stmt_resource_map) {
		cout << "stmt " << srmap.first << " map:\n";
		for (auto rmap : srmap.second) {
			tuple<string,int,bool> first = rmap.first;
			cout << indent;
			if (get<2>(first))
				cout << "stream : "; 
			cout << get<0>(first) << "[" << get<1>(first) << "] = "; 
			cout << print_resource (rmap.second) << endl;
		}
	}
}

bool stencilDefn::waw_dependence_exists (int source, int dest) {
	// Check if dest exists in the waw_dependence map
	bool result = waw_dependence_graph.find (dest) != waw_dependence_graph.end ();
	if (result) {
		bool found = false;
		for (auto waw : waw_dependence_graph[dest]) {
			found |= (waw == source);
		}
		result &= found;
	}
	return result;	
}

bool stencilDefn::war_dependence_exists (int source, int dest) {
	// Check if dest exists in the war_dependence map
	bool result = war_dependence_graph.find (dest) != war_dependence_graph.end ();
	if (result) {
		bool found = false;
		for (auto war : war_dependence_graph[dest]) {
			found |= (war == source);
		}
		result &= found;
	}
	return result;	
}

bool stencilDefn::raw_dependence_exists (int source, int dest) {
	// Check if dest exists in the raw_dependence map
	bool result = raw_dependence_graph.find (dest) != raw_dependence_graph.end ();
	if (result) {
		bool found = false;
		for (auto raw : raw_dependence_graph[dest]) {
			found |= (raw == source);
		}
		result &= found;
	}
	return result;	
}

bool stencilDefn::accumulation_dependence_exists (int source, int dest) {
	// Check if dest exists in the dependence map
	bool result = accumulation_dependence_graph.find (dest) != accumulation_dependence_graph.end ();
	if (result) {
		bool found = false;
		for (auto dep : accumulation_dependence_graph[dest]) {
			found |= (dep == source);
		}
		result &= found;
	}
	return result;	
}

void stencilDefn::offset_dependent_planes (void) {
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	//Compute the read-only arrays
	set<string> reads, writes;
	for (auto st : stmts) {
		for (auto stmt : decomposed_stmts[st->get_stmt_num()]) {
			exprNode *lhs = stmt->get_lhs_expr ();
			exprNode *rhs = stmt->get_rhs_expr ();
			// Compute reads before writes
			rhs->compute_rbw (reads, writes, true);
			if (stmt->get_op_type () != ST_EQ)
				lhs->compute_rbw (reads, writes, true);
			lhs->compute_rbw (reads, writes, false);
		}
	}
	if (DEBUG) {
		for (auto st : stmts) {
			int idx = 0;
			for (auto stmt : decomposed_stmts[st->get_stmt_num()]) {
				cout << "substmt " << idx << " of stmt " << st->get_stmt_num() << "\n";
				stringstream lhs_out, rhs_out;
				exprNode *lhs = stmt->get_lhs_expr ();
				exprNode *rhs = stmt->get_rhs_expr ();
				STMT_OP op_type = stmt->get_op_type ();
				lhs->print_node (lhs_out);
				rhs->print_node (rhs_out);
				cout << "\t" << lhs_out.str () << print_stmt_op (op_type) << rhs_out.str () << ";\n";
				idx++;
			}
		}
	}
	// Compute the offsets based on RAW dependence
	for (vector<stmtNode*>::iterator i=next(stmts.begin()); i!=stmts.end(); i++) {
		int i_stmt_num = (*i)->get_stmt_num ();
		for (vector<stmtNode*>::iterator j=stmts.begin(); j!=i; j++) {
			int j_stmt_num = (*j)->get_stmt_num ();
			// Offset the plane of i with respect to j if there is a RAW dependence from j to i
			if (raw_dependence_exists (j_stmt_num, i_stmt_num)) {
				for (auto d : decomposed_stmts[i_stmt_num]) {
					exprNode *dest_rhs = d->get_rhs_expr ();
					int src_offset = (plane_offset.find (j_stmt_num) == plane_offset.end ()) ? 0 : plane_offset[j_stmt_num];
					for (auto s : decomposed_stmts[j_stmt_num]) {
						exprNode *src_lhs = s->get_lhs_expr ();
						int offset = dest_rhs->maximum_streaming_offset (src_lhs, stream_dim);
						int dest_offset = (plane_offset.find (i_stmt_num) == plane_offset.end ()) ? 0 : plane_offset[i_stmt_num];
						plane_offset[i_stmt_num] = max (dest_offset, offset + src_offset);
					}
				}
			}
		}
	}
	// Change the statements with scalar lhs to nearest following consumer statement 
	vector<int> pointwise_occurrences;
	compute_pointwise_occurrences (pointwise_occurrences);
	if (!pointwise_occurrences.empty ()) {
		for (int i=stmts.size()-2; i>=0; i--) {
			int i_stmt_num = stmts[i]->get_stmt_num ();
			if (find (pointwise_occurrences.begin(), pointwise_occurrences.end(), i_stmt_num) == pointwise_occurrences.end())
				continue;
			// Find the first statement that consumes the data produced by i_stmt_num;
			for (int j=i+1; j<(int)stmts.size(); j++) {
				int j_stmt_num = stmts[j]->get_stmt_num ();
				bool dependence_exists = raw_dependence_exists (i_stmt_num, j_stmt_num) || waw_dependence_exists (i_stmt_num, j_stmt_num);
				if (dependence_exists && (plane_offset.find (j_stmt_num) != plane_offset.end ())) {
					if (plane_offset[j_stmt_num] != 0)
						plane_offset[i_stmt_num] = plane_offset[j_stmt_num]; 
					break;	
				}
			}
		}
	}
	if (DEBUG) {
		cout << "\nPrinting plane offsets\n";
		for (auto p : plane_offset) {
			cout << indent << "statement " << p.first << " - offset " << p.second << endl;
		}
	}
	// Add appropriate shift in the stmt_resource_map
	for (auto srmap : stmt_resource_map) {
		if (plane_offset.find (srmap.first) != plane_offset.end()) {
			if (plane_offset[srmap.first] > 0) {
				map<tuple<string,int,bool>,RESOURCE> res_map;
				for (auto rmap : srmap.second) {
					if (get<2>(rmap.first)) {	
						tuple<string,int,bool> first = make_tuple (get<0>(rmap.first), get<1>(rmap.first)-plane_offset[srmap.first], get<2>(rmap.first));
						res_map[first] = rmap.second;
					}
					else {
						res_map[rmap.first] = rmap.second;
					}
				}
				stmt_resource_map[srmap.first] = res_map; 
			}
		}
	}
	// Add appropriate shifts to the statements
	//for (auto s : stmts) {
	//	int stmt_num = s->get_stmt_num ();
	//	if (plane_offset.find (stmt_num) != plane_offset.end()) {
	//		if (plane_offset[stmt_num] > 0) {
	//			exprNode *lhs = s->get_lhs_expr ();
	//			exprNode *rhs = s->get_rhs_expr ();
	//			lhs->offset_expr (plane_offset[stmt_num], stream_dim);
	//			rhs->offset_expr (plane_offset[stmt_num], stream_dim);
	//		}
	//	}
	//}
	for (auto st : stmts) {
		int stmt_num = st->get_stmt_num ();
		if (plane_offset.find (stmt_num) != plane_offset.end()) {
			if (plane_offset[stmt_num] > 0) {
				for (auto s : decomposed_stmts[stmt_num]) {
					exprNode *lhs = s->get_lhs_expr ();
					exprNode *rhs = s->get_rhs_expr ();
					lhs->offset_expr (plane_offset[stmt_num], stream_dim);
					rhs->offset_expr (plane_offset[stmt_num], stream_dim);
				}
			}
		}
	}
}

bool stencilDefn::is_stencil_arg (string s) {
	// Remove all text following @
	string name = trim_string (s, "@");	
	bool result = false;
	for (auto arg : get_arg_list ()) {
		if (name.compare (arg) == 0) {
			result = true;
			break;
		}	
	}
	return result;
}

bool stencilDefn::is_temp_array (string s) {
	bool result = false;
	string name = trim_string (s, "@");
	for (auto ad : array_decls) {
		if ((name.compare (ad->get_array_name ()) == 0) && (ad->is_temp_array ())) {
			result = true;
			break;
		}
	}
	return result;
}

bool funcDefn::is_array_decl (string s) {
	// Remove all text following @
	string name = trim_string (s, "@");	
	bool result = false;
	for (auto ad : array_decls) {
		if (name.compare (ad->get_array_name ()) == 0) {
			result = true;
			break;
		}	
	}
	return result;
}

bool stencilDefn::is_array_decl (string s) {
	// Remove all text following @
	string name = trim_string (s, "@");	
	bool result = false;
	for (auto ad : array_decls) {
		if (name.compare (ad->get_array_name ()) == 0) {
			result = true;
			break;
		}	
	}
	return result;
}

vector<Range*> funcDefn::get_array_range (string s) {
	// Remove all text following @
	string name = trim_string (s, "@");	
	vector<Range *> result;
	for (auto ad : array_decls) {
		if (name.compare (ad->get_array_name ()) == 0) {
			result = ad->get_array_range ();
			break;
		}
	}
	return result;
}

vector<Range*> stencilDefn::get_array_range (string s) {
	// Remove all text following @
	string name = trim_string (s, "@");	
	vector<Range *> result;
	for (auto ad : array_decls) {
		if (name.compare (ad->get_array_name ()) == 0) {
			result = ad->get_array_range ();
			break;
		}
	}
	return result;
}

DATA_TYPE funcDefn::get_array_type (string s) {
	// Remove all text following @
	string name = trim_string (s, "@");	
	DATA_TYPE result = DOUBLE;
	for (auto ad : array_decls) {
		if (name.compare (ad->get_array_name ()) == 0) {
			result = ad->get_array_type ();
			break;
		}	
	}
	return result;
}

DATA_TYPE stencilDefn::get_array_type (string s) {
	// Remove all text following @
	string name = trim_string (s, "@");	
	DATA_TYPE result = DOUBLE;
	for (auto ad : array_decls) {
		if (name.compare (ad->get_array_name ()) == 0) {
			result = ad->get_array_type ();
			break;
		}	
	}
	return result;
}

bool stencilDefn::is_var_decl (string s) {
	// Remove all text following @
	string name = trim_string (s, "@");	
	bool result = false;
	for (auto vd : var_decls->get_symbol_map ()) {
		if (name.compare (vd.first) == 0) {
			result = true;
			break;
		}	
	}
	return result;
}

DATA_TYPE stencilDefn::get_var_type (string s) {
	// Remove all text following @
	string name = trim_string (s, "@");	
	DATA_TYPE result = DOUBLE;
	for (auto vd : var_decls->get_symbol_map ()) {
		if (name.compare (vd.first) == 0) {
			result = vd.second;
			break;
		}	
	}
	return result;
}

void stencilDefn::amend_resource_map (void) {
	// 1. Create the amended resource map
	for (auto srmap : stmt_resource_map) {
		map<tuple<string,int,bool>,RESOURCE> temp_map = srmap.second;
		for (auto temp : temp_map) {
			if (resource_map.find (temp.first) == resource_map.end ()) 
				resource_map[temp.first] = temp.second;
			else 
				resource_map[temp.first] = max (resource_map[temp.first], temp.second);
		}
	}
	if (DEBUG) {
		cout << "\nAmended resource map after fusion\n";
		for (auto rmap : resource_map) {
			tuple<string,int,bool> first = rmap.first;
			cout << indent;
			if (get<2>(first))
				cout << "stream : ";
			cout << get<0>(first) << "[" << get<1>(first) << "] = ";
			cout << print_resource (rmap.second) << endl;
		}
	}

	// 2. Collect the stats on accesses
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	for (auto st : stmts) {
		for (auto s : decomposed_stmts[st->get_stmt_num()]) {
			exprNode *lhs = s->get_lhs_expr ();
			exprNode *rhs = s->get_rhs_expr ();
			lhs->collect_access_stats (access_stats, false, iterators, stream, stream_dim);
			// If the operator is not =, then lhs is also in rhs
			if (s->get_op_type() != ST_EQ)
				lhs->collect_access_stats (access_stats, true, iterators, stream, stream_dim);
			rhs->collect_access_stats (access_stats, true, iterators, stream, stream_dim);
		}
	}

	// Populate temp_access_stats
	map<tuple<string,int,bool>, tuple<int,int,int>> temp_access_stats;
	vector<string> blockdims = (iterators.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iterators.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));
	for (auto amap : access_stats) {
		tuple<string,int,bool> first = amap.first;
		tuple<int,int,map<int,vector<string>>> second = amap.second;
		// Complex computation now
		map<int,vector<string>> index_map = get<2>(second);
		int domsize = 1;
		// Get the data type
		string name = get<0>(first);
		if (is_array_decl (name)) 
			domsize *= get_data_size (get_array_type (name));
		else if (is_var_decl (name)) 
			domsize *= get_data_size (get_var_type (name));
		for (auto imap : index_map) {
			int cursize = 1;
			for (auto id : imap.second) {
				vector<string>::iterator it = find (iterators.begin(), iterators.end(), id);
				if (it != iterators.end() && !(stream && stream_dim.compare (*it) == 0)) {
					tuple<int,int> halo = (halo_dims.find (id) != halo_dims.end()) ? halo_dims[id] : make_tuple (0,0);
					cursize = max (cursize, (get<1>(halo) - get<0>(halo)) + (get_block_dims())[blockdims[it-iterators.begin()]]);
				}
			}
			domsize *= cursize;
		}
		temp_access_stats[first] = make_tuple (get<0>(second), get<1>(second), domsize);
	}

	if (DEBUG) {
		cout << "\nOriginal Access Stats\n";
		for (auto amap : temp_access_stats) {
			tuple<string,int,bool> first = amap.first;
			tuple<int,int,int> second = amap.second;
			cout << indent;
			if (get<2>(first))
				cout << "stream : ";
			cout << get<0>(first) << "[" << get<1>(first) << "] = ";
			cout << get<0>(second) << " reads, " << get<1>(second) << " writes, ";
			cout << get<2>(second) << " bytes shmem\n"; 
		}
	}

	//3a. Remove trivial planes from shared memory/register that have only one read/write
	bool demoted_storage = false;
	for (auto amap : temp_access_stats) {
		tuple<string,int,bool> first = amap.first;
		tuple<int,int,int> &second = amap.second;
		string arr_name = trim_string (get<0>(first), "@");
		// Remove planes that are only writes
		if (!retiming_feasible && get<0>(second) == 0 && get<1>(second) == 1 && !is_temp_array (arr_name)) {
			assert (resource_map.find (first) != resource_map.end ());
			demoted_storage = true;
			resource_map[first] = GLOBAL_MEM;
		}
		//// Remove the planes that are one read and one write
		//if (get<0>(second) <= 1 && get<1>(second) <= 1 && !is_temp_array (arr_name)) {
		//	assert (resource_map.find (first) != resource_map.end ());
		//	if (resource_map[first] == SHARED_MEM) {
		//		demoted_storage = true;
		//		resource_map[first] = GLOBAL_MEM;
		//	}
		//}
		//// TODO: can be incorrect. Remove the planes that have exactly one read and one write, and that are in copyout
		//if (get<0>(second) == 1 && get<1>(second) == 1 && is_copyout_array (arr_name)) {
		//	assert (resource_map.find (first) != resource_map.end ());
		//	if (resource_map[first] == SHARED_MEM || resource_map[first] == REGISTER) {
		//		demoted_storage = true;
		//		resource_map[first] = GLOBAL_MEM;
		//	}
		//}
	}
	// 3b. Remove planes from shared memory or registers that are declared as gmem and constant 
	for (auto amap : temp_access_stats) {
		tuple<string,int,bool> first = amap.first;
		string arr_name = trim_string (get<0>(first), "@");
		// Check if the array appears in the noshmem_decl
		bool gmem_decl_found = false, constant_decl_found = false;
		for (auto gmem : get_gmem_decl ()) {
			if (arr_name.compare (gmem) == 0) {
				gmem_decl_found = true;
				break;
			}
		}
		for (auto cst : constants) {
			if (arr_name.compare (cst) == 0) {
				constant_decl_found = true;
				break;
			}
		}
		if (gmem_decl_found || constant_decl_found) {
			assert (resource_map.find (first) != resource_map.end ());
			if (resource_map[first] == SHARED_MEM || resource_map[first] == REGISTER) {
				demoted_storage = true;
				resource_map[first] = GLOBAL_MEM;
			}
		}
	}
	// 3c. Remove planes from shared memory that are declared as noshmem
	for (auto amap : temp_access_stats) {
		tuple<string,int,bool> first = amap.first;
		string arr_name = trim_string (get<0>(first), "@");
		// Check if the array appears in the noshmem_decl
		bool noshm_decl_found = false;
		for (auto noshm : get_noshmem_decl ()) {
			if (arr_name.compare (noshm) == 0) {
				noshm_decl_found = true;
				break;
			}
		}
		if (noshm_decl_found) { 
			assert (resource_map.find (first) != resource_map.end ());
			if (resource_map[first] == SHARED_MEM) {
				demoted_storage = true;
				resource_map[first] = GLOBAL_MEM;
			}
		}
	}
	// 3d. Now change the stmt level resource map as well for gmem, constant, and noshmem
	for (auto &srmap : stmt_resource_map) {
		for (auto &rmap : srmap.second) {
			bool gmem_decl_found = false, constant_decl_found = false, noshm_decl_found = false;
			tuple<string,int,bool> first = rmap.first;
			string arr_name = trim_string (get<0>(first), "@");
			for (auto gmem : get_gmem_decl ()) {
				if (arr_name.compare (gmem) == 0) {
					gmem_decl_found = true;
					break;
				}
			}
			for (auto cst : constants) {
				if (arr_name.compare (cst) == 0) {
					constant_decl_found = true;
					break;
				}
			}
			for (auto noshm : get_noshmem_decl ()) {
				if (arr_name.compare (noshm) == 0) {
					noshm_decl_found = true;
					break;
				}
			}
			if (gmem_decl_found || constant_decl_found) {
				if (rmap.second == SHARED_MEM || rmap.second == REGISTER) 
					rmap.second = GLOBAL_MEM;
			}
			if (noshm_decl_found && (rmap.second == SHARED_MEM))
				rmap.second = GLOBAL_MEM;	
		}
	}
	if (DEBUG & demoted_storage) {
		cout << "\nAmended resource map after removing trivial shared memory planes\n";
		for (auto rmap : resource_map) {
			tuple<string,int,bool> first = rmap.first;
			cout << indent;
			if (get<2>(first))
				cout << "stream : ";
			cout << get<0>(first) << "[" << get<1>(first) << "] = ";
			cout << print_resource (rmap.second) << endl;
		}
	}
	// Remove non-shared memory planes
	for (auto rmap : resource_map) {
		tuple<string,int,bool> first = rmap.first;
		assert (temp_access_stats.find (first) != temp_access_stats.end ());
		if (rmap.second != SHARED_MEM)
			temp_access_stats.erase (first);
	}

	// 4. Factor in the unrolling factors
	map<string,int> unrolldecls = get_unroll_decls ();
	int ufactor = 1;
	for (auto ud : unrolldecls) {
		ufactor *= ud.second;	
	}
	int explicit_regs = 0, shmem_used = 0;
	for (auto rmap : resource_map) {
		tuple<string,int,bool> first = rmap.first;
		if (rmap.second == SHARED_MEM) {
			assert (temp_access_stats.find (first) != temp_access_stats.end ());
			tuple<int,int,int> &second = temp_access_stats[first];
			get<2>(second) = get<2>(second) * ufactor;
			shmem_used += get<2>(second);
		}
		if (rmap.second == REGISTER) 
			explicit_regs += ufactor;	
	}
	if (DEBUG) {
		cout << "\nAccess Stats\n";
		for (auto amap : temp_access_stats) {
			tuple<string,int,bool> first = amap.first;
			tuple<int,int,int> second = amap.second;
			cout << indent;
			if (get<2>(first))
				cout << "stream : ";
			cout << get<0>(first) << "[" << get<1>(first) << "] = ";
			cout << get<0>(second) << " reads, " << get<1>(second) << " writes, ";
			cout << get<2>(second) << " bytes shmem\n"; 
		}
		cout << "Shared memory size = " << shmem_used << ", explicit registers = " << explicit_regs << endl;
	}

	//5. Modify accesses until the shared memory constraint is satisfied
	bool demotion_required = shmem_used > shmem_size;
	// Keep demoting the storage from shared memory to global memory till 
	// all the data fits in the shared memory capacity
	while (shmem_used > shmem_size) {
		// Find the plane with the least accesses
		int min_val=INT_MAX;
		map<tuple<string,int,bool>, tuple<int,int,int>>::iterator min_pos;
		for (map<tuple<string,int,bool>, tuple<int,int,int>>::iterator amap=temp_access_stats.begin(); amap!=temp_access_stats.end(); amap++) {
			tuple<string,int,bool> first = amap->first;
			string arr_name = trim_string (get<0>(first), "@");
			// Check if the array appears in the shmem_decl, or is a temporary array
			bool shm_decl_found = false;
			for (auto shm : get_shmem_decl ()) {
				if (arr_name.compare (shm) == 0) {
					shm_decl_found = true;
					break;
				}
			}
			if (shm_decl_found || is_temp_array (arr_name))
				continue;
			tuple<int,int,int> &second = amap->second;
			int accesses = get<0>(second) + get<1>(second);
			int new_min = min (accesses, min_val);
			if (new_min < min_val) {
				min_pos = amap;
				min_val = new_min;
			}
		}
		if (min_val == INT_MAX) {
			fprintf (stderr, "Cannot generate code due to shared memory constraints\n");	
			exit (1);
		}
		// Modify storage for the entry at min_pos
		tuple<string,int,bool> first = min_pos->first;
		tuple<int,int,int> &second = min_pos->second;
		assert (resource_map.find (first) != resource_map.end ());
		resource_map[first] = GLOBAL_MEM;
		shmem_used -= get<2>(second);
		temp_access_stats.erase (first);
	}

	// 6. Shave off unnecessary entries from resource map
	// Find the arrays that are written, and then read. 
	set<string> reads, writes;
	for (auto st : stmts) {
		for (auto stmt : decomposed_stmts[st->get_stmt_num()]) {
			exprNode *lhs = stmt->get_lhs_expr ();
			exprNode *rhs = stmt->get_rhs_expr ();
			rhs->compute_wbr (writes, reads, false);
			if (stmt->get_op_type () != ST_EQ)
				lhs->compute_wbr (writes, reads, false);
			lhs->compute_wbr (writes, reads, true);
		}
	}
	for (map<tuple<string,int,bool>,RESOURCE>::iterator it=resource_map.begin(); it!=resource_map.end();) {
		string arr_name = trim_string (get<0>(it->first), "@");
		if (it->second == GLOBAL_MEM && (reads.find (arr_name) == reads.end ()))
			it = resource_map.erase (it);
		else 
			++it;
	}
	if (DEBUG) {
		cout << "\nArrays that are read after writes : ";
		for (auto r : reads) {
			cout << r << " ";
		}
		cout << "\nAmended resource map after removing arrays that were read first before writes\n";
		for (auto rmap : resource_map) {
			tuple<string,int,bool> first = rmap.first;
			cout << indent;
			if (get<2>(first))
				cout << "stream : ";
			cout << get<0>(first) << "[" << get<1>(first) << "] = ";
			cout << print_resource (rmap.second) << endl;
		}
	}
	if (DEBUG & demotion_required) {
		cout << "\nAmended resource map after demoting shared memory planes\n";
		for (auto rmap : resource_map) {
			tuple<string,int,bool> first = rmap.first;
			cout << indent;
			if (get<2>(first))
				cout << "stream : ";
			cout << get<0>(first) << "[" << get<1>(first) << "] = ";
			cout << print_resource (rmap.second) << endl;
		}
		cout << "\nFinal shared memory planes\n";
		for (auto amap : temp_access_stats) {
			tuple<string,int,bool> first = amap.first;
			tuple<int,int,int> second = amap.second;
			cout << indent;
			if (get<2>(first))
				cout << "stream : ";
			cout << get<0>(first) << "[" << get<1>(first) << "] = ";
			cout << get<0>(second) << " reads, " << get<1>(second) << " writes, ";
			cout << get<2>(second) << " bytes shmem\n"; 
		}
	}
	// Check if we need to reset halo dims if no shared memory is used
	bool shmem_is_used = false;
	for (auto rmap : resource_map) {
		shmem_is_used |= (rmap.second == SHARED_MEM);
	}
	if (!shmem_is_used) {
		reset_halo_dims ();
		use_shmem = false;
	}
}

void stencilDefn::verify_immutable_indices (void) {
	// First collect all the writes
	vector<string> writes;
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	for (auto s : stmts) {
		exprNode *lhs = s->get_lhs_expr ();
		string write_name = lhs->get_name ();
		writes.push_back (write_name);
	}
	// Now iterate over the rhs of each statement, and for each array access,
	// verify that the indices comprise immutable parameters/iterators/scalars
	for (auto s : stmts) {
		exprNode *rhs = s->get_rhs_expr ();
		bool start_verification = false;
		bool verified = rhs->verify_immutable_expr (writes, start_verification);
		assert (verified && "The index expression of arrays comprise mutable values (verify_immutable_indices)");
	}
}

void stencilDefn::compute_pointwise_occurrences (vector<int> &pointwise_occurrences) {
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	for (vector<stmtNode*>::iterator i=stmts.begin(); i!=stmts.end(); i++) {
		exprNode *src = (*i)->get_lhs_expr ();
		bool pointwise = true;
		for (vector<stmtNode*>::iterator j=stmts.begin(); j!=stmts.end(); j++) {
			if ((*i)->get_stmt_num () == (*j)->get_stmt_num ())
				continue;
			exprNode *lhs = (*j)->get_lhs_expr ();
			exprNode *rhs = (*j)->get_rhs_expr ();
			pointwise &= lhs->pointwise_occurrence (src);
			pointwise &= rhs->pointwise_occurrence (src);
		}
		if (pointwise) 
			pointwise_occurrences.push_back ((*i)->get_stmt_num ());
	}
	if (DEBUG) {
		cout << "\nPointwise occurrences are lhs of stmts ";
		for (auto p : pointwise_occurrences) {
			cout << p << " ";
		}
		cout << "\n\n"; 
	}
}

// A single dependence graph for RAW, WAR, WAW dependences
void stencilDefn::unify_dependences (map<int,vector<int>> &unified_deps) {
	unified_deps.insert (raw_dependence_graph.begin(), raw_dependence_graph.end());
	for (auto w : war_dependence_graph) {
		int key = w.first;
		vector<int> value = w.second;
		if (unified_deps.find (key) == unified_deps.end ())
			unified_deps[key] = value;
		else {
			vector<int> &entry = unified_deps[key];
			for (auto v : value) {
				if (find (entry.begin(), entry.end(), v) == entry.end())
					entry.push_back (v);
			}
			unified_deps[key] = entry; 
		} 
	}
	for (auto w : waw_dependence_graph) {
		int key = w.first;
		vector<int> value = w.second;
		if (unified_deps.find (key) == unified_deps.end ())
			unified_deps[key] = value;
		else {
			vector<int> &entry = unified_deps[key];
			for (auto v : value) {
				if (find (entry.begin(), entry.end(), v) == entry.end())
					entry.push_back (v);
			}
			unified_deps[key] = entry; 
		}
	}
}

vector<stmtNode*> stencilDefn::reconstruct_retimed_rhs (vector<stmtNode*> substmts, stmtNode *orig_stmt) {
	vector<stmtNode*> ret;
	map<exprNode*,vector<stmtNode*>> same_lhs_stmts; 
	for (auto it : substmts) {
		exprNode *lhs = it->get_lhs_expr ();
		bool inserted = false;
		for (auto jt : same_lhs_stmts) {
			if (lhs->same_expr (jt.first)) {
				(same_lhs_stmts[jt.first]).push_back (it);
				inserted = true;
				break;
			}
		}
		if (!inserted) 
			(same_lhs_stmts[lhs]).push_back (it);
	}

	for (auto it : same_lhs_stmts) {
		exprNode *lhs = it.first;
		stmtNode *first_stmt = (it.second).front();
		exprNode *combined_rhs = first_stmt->get_rhs_expr ();
		STMT_OP stmt_op = first_stmt->get_op_type ();
		if (stmt_op == ST_MINUSEQ) {
			// Convert it to +=, and RHS to uminus
			combined_rhs = new uminusNode (combined_rhs);
			stmt_op = ST_PLUSEQ;
		}
		for (vector<stmtNode*>::iterator jt=next((it.second).begin()); jt!=(it.second).end(); jt++) {
			exprNode *rhs = (*jt)->get_rhs_expr ();
			OP_TYPE op = convert_stmt_op_to_op ((*jt)->get_op_type ());
			combined_rhs = new binaryNode (op, combined_rhs, rhs, infer_data_type (combined_rhs->get_type(), rhs->get_type()), false);
		}
		ret.push_back (new stmtNode (stmt_op, lhs, combined_rhs, orig_stmt->get_stmt_num(), orig_stmt->get_stmt_domain()));
	}
	// Move all the assignments before in the vector
	for (vector<stmtNode*>::iterator it=ret.begin(); it!=ret.end(); it++) {
		if ((*it)->get_op_type () == ST_EQ) {
			stmtNode *st = *it;
			it = ret.erase (it);
			vector<stmtNode*>::iterator jt=ret.begin ();
			for (; jt!=ret.end(); jt++) {
				if ((*jt)->get_op_type () == ST_EQ)
					continue;
				break;
			}
			ret.insert (jt, st);
		}
	}

	for (auto r : ret) {
		stringstream lhs, rhs;
		(r->get_lhs_expr())->print_node (lhs);
		(r->get_rhs_expr())->print_node (rhs);
		cout << "New statements are : " << lhs.str() << print_stmt_op (r->get_op_type()) << rhs.str () << endl;
	}
	if (CONSOLIDATE) {
		// Identify common subexpressions
		for (auto it : ret) {
			map<exprNode*, vector<exprNode*>> same_accesses;
			exprNode *rhs = it->get_rhs_expr ();
			rhs->identify_same_accesses (same_accesses, NULL);
			if (DEBUG) {
				for (auto s : same_accesses) {
					if (!(s.second).empty()) {
						stringstream lhs;
						(s.first)->print_node (lhs);
						cout << "same accesses for " << lhs.str () << " : ";
						for (auto t : s.second) {
							stringstream rhs;
							t->print_node (rhs);
							cout << rhs.str() << " ";
						}
						cout << "\n";
					}
				}
				cout << "\n";
			}
			// Remove all common entries
			for (map<exprNode*, vector<exprNode*>>::iterator kt=same_accesses.begin(); kt!=same_accesses.end(); kt++) {
				for (map<exprNode*, vector<exprNode*>>::iterator jt=next(kt); jt!=same_accesses.end(); jt++) {
					for (auto r : kt->second) {
						(jt->second).erase (remove((jt->second).begin(), (jt->second).end(), r), (jt->second).end());
					}
					same_accesses[jt->first] = jt->second;	
				}
			}
			// Remove all entries with size <= 1
			for (auto s : same_accesses) {
				if ((s.second).size() <= 1)
					same_accesses.erase (s.first);
			}
			if (!same_accesses.empty ()) {
				map<exprNode *, exprNode *> cmap;
				vector<tuple<OP_TYPE, exprNode*>> consolidated_exprs; 
				rhs->consolidate_same_accesses (consolidated_exprs, same_accesses, cmap, NULL, false);
				for (auto c : cmap) {
					(c.second)->set_nested ();
					exprNode *new_node = new binaryNode (T_MULT, c.first, c.second, infer_data_type((c.first)->get_type(), (c.second)->get_type()), false);
					consolidated_exprs.insert (consolidated_exprs.begin(), make_tuple (T_PLUS, new_node));
				}
				tuple<OP_TYPE, exprNode*> seed = consolidated_exprs.front();
				exprNode *new_rhs = (get<0>(seed) == T_MINUS) ? new uminusNode (get<1>(seed)) : get<1>(seed);
				for (vector<tuple<OP_TYPE, exprNode*>>::iterator ce=next(consolidated_exprs.begin()); ce!=consolidated_exprs.end(); ce++) {
					new_rhs = new binaryNode (get<0>(*ce), new_rhs, get<1>(*ce), infer_data_type((new_rhs)->get_type(), (get<1>(*ce))->get_type()), false);  	
				}
				it->set_rhs_expr (new_rhs);
			}
		}
		for (auto r : ret) {
			stringstream lhs, rhs;
			(r->get_lhs_expr())->print_node (lhs);
			(r->get_rhs_expr())->print_node (rhs);
			cout << "Consolidated statements are : " << lhs.str() << print_stmt_op (r->get_op_type()) << rhs.str () << endl;
		}
	}
	return ret;
}

void stencilDefn::decompose_statements (bool decompose, bool retime, bool fold_computations) {
	vector<stmtNode*> orig_stmts = stmt_list->get_stmt_list ();
	if (decompose) {
		//Get the reuse in each statement to exploit symmetry
		//Map from array name to the statements in which it is used 
		map<string, set<int>> array_use_map;
		for (auto stmt : orig_stmts) {
			set<string> arrays;
			int stmt_num = stmt->get_stmt_num ();
			stmt->accessed_arrays (arrays);
			for (auto a : arrays) {
				if (array_use_map.find (a) != array_use_map.end())
					array_use_map[a].insert (stmt_num);
				else {
					set<int> st; 
					st.insert (stmt_num);
					array_use_map[a] = st;
				}
			}
		}
		if (DEBUG) {
			cout << "Accessed array map :\n";
			for (auto a : array_use_map) {
				cout << a.first << " : (";
				for (auto b : a.second) {
					cout << b << " ";
				}
				cout << ")\n";
			}
		}

		vector<stmtNode*> init_stmts;
		for (auto stmt : orig_stmts) {
			vector<stmtNode*> substmts;
			vector<tuple<string,string>> sym_initializations;
			stmt->decompose_statements (substmts, fold_computations, array_use_map, sym_initializations, init_stmts);
			for (auto s : sym_initializations) {
				string domain_arr = get<0>(s);
				string sym_arr = get<1>(s);
				//Find the array declaration for domain_arr, and replicate it for sym_arr
				for (auto a : get_array_decls()) {
					if (domain_arr.compare (a->get_array_name()) == 0) {
						arrayDecl *sym_decl = new arrayDecl (*a);
						sym_decl->set_array_name (sym_arr);
						sym_decl->set_temp_array (true);
						push_array_decl (sym_decl);		
						break;
					}
				}
			}
			decomposed_stmts[stmt->get_stmt_num()] = substmts;
		}
		// Delay and insert new statements
		stmt_list->insert_stmt (init_stmts);
		int decomposed_stmt_size = (int)decomposed_stmts.size();
		int init_stmts_size = (int)init_stmts.size();
		for (int i=decomposed_stmt_size+init_stmts_size-1; i>=0; i--) {
			decomposed_stmts[i] = decomposed_stmts[i-init_stmts_size];
		}
		for (int i=0; i<init_stmts_size; i++) {
			vector<stmtNode*> substmts;
			substmts.push_back (init_stmts[i]);
			decomposed_stmts[i] = substmts;
		}
		// Check if retiming is beneficial 
		retiming_feasible = stream && retime;
		if (!retiming_feasible) return;
		for (auto d : decomposed_stmts) {
			for (auto s : d.second) {
				// LHS must be an array, or the statement must have just scalars
				exprNode *lhs = s->get_lhs_expr ();
				retiming_feasible &= (lhs->is_shiftvec_node () || s->is_scalar_stmt ());
				// All of RHS must have the same plane in the streaming dimension
				exprNode *rhs = s->get_rhs_expr ();
				vector<int> offset;
				retiming_feasible &= rhs->retiming_feasible (stream_dim, offset);
			}
		}
		cout << "Retiming feasible = " << retiming_feasible << endl;
		for (auto d : decomposed_stmts) {
			for (auto s : d.second) {
				exprNode *rhs = s->get_rhs_expr ();
				int retiming_offset = (stream && retiming_feasible) ? rhs->get_retiming_offset (stream_dim) : 0;
				s->retime_statement (stream_dim, -1*retiming_offset);
			}
		}
		// Sort the statements based on their LHS
		for (auto d : decomposed_stmts) {
			map<int,vector<stmtNode*>,greater<int>> sorted_offsets;
			for (auto it : d.second) {
				exprNode *lhs = it->get_lhs_expr ();
				int offset = lhs->streaming_offset (stream_dim);
				sorted_offsets[offset].push_back (it);
			}
			vector<stmtNode*> substmt_vec;
			for (auto it : sorted_offsets) {
				substmt_vec.insert (substmt_vec.end(), (it.second).begin(), (it.second).end());
			}
			decomposed_stmts[d.first] = substmt_vec;
		}

		// Move the highest access first, and make it = in case the statement was an assignment
		if (retiming_feasible) {
			for (auto d : decomposed_stmts) {
				int max_offset = INT_MIN;
				vector<stmtNode*>::iterator pos;
				for (vector<stmtNode*>::iterator it=(d.second).begin(); it!=(d.second).end(); it++) {
					exprNode *lhs = (*it)->get_lhs_expr ();
					int offset = lhs->streaming_offset (stream_dim);
					if (offset > max_offset)
						pos = it;
					max_offset = max (max_offset, offset);
				}
				// Move element at position pos to the front of the vector
				if (pos != (d.second).begin()) {
					stmtNode *it = *pos;
					(d.second).erase (pos);
					(d.second).insert ((d.second).begin(), it);
					decomposed_stmts[d.first] = d.second;
				}
			}
		}
		// Now correct the initialization
		for (auto d : decomposed_stmts) {
			STMT_OP stmt_op = (orig_stmts[d.first])->get_op_type ();
			if (stmt_op == ST_EQ) {
				vector<stmtNode*>::iterator it=(d.second).begin();
				if ((*it)->get_op_type () == ST_MINUSEQ) {
					exprNode *orig_rhs = (*it)->get_rhs_expr ();
					DATA_TYPE type = orig_rhs->get_type ();
					bool nested = orig_rhs->is_nested ();	
					(*it)->set_rhs_expr (new uminusNode (orig_rhs, type, nested));
				}
				(*it)->set_op_type (ST_EQ);	
			}
		}
		// Try to reconstruct the statement back for retiming
		if (retiming_feasible) {
			vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
			for (auto d : decomposed_stmts) {
				vector<stmtNode*> it = reconstruct_retimed_rhs (d.second, stmts[d.first]);
				decomposed_stmts[d.first] = it;	
			}
			// Sort the statements based on their LHS
			for (auto d : decomposed_stmts) {
				map<int,vector<stmtNode*>,greater<int>> sorted_offsets;
				for (auto it : d.second) {
					exprNode *lhs = it->get_lhs_expr ();
					int offset = lhs->streaming_offset (stream_dim);
					sorted_offsets[offset].push_back (it);
				}
				vector<stmtNode*> substmt_vec;
				for (auto it : sorted_offsets) {
					substmt_vec.insert (substmt_vec.end(), (it.second).begin(), (it.second).end());
				}
				decomposed_stmts[d.first] = substmt_vec;
			}
		}
	}
	else {
		for (auto stmt : orig_stmts) {
			vector<stmtNode*> stmt_vec;
			stmt_vec.push_back (stmt);
			decomposed_stmts[stmt->get_stmt_num()] = stmt_vec;
		}
	}
}

void stencilDefn::compute_dependences (void) {
	vector<stmtNode*> stmts = stmt_list->get_stmt_list ();
	// Clear the dependence graphs, start from scratch
	raw_dependence_graph.clear ();
	war_dependence_graph.clear ();
	waw_dependence_graph.clear ();
	accumulation_dependence_graph.clear ();
	for (vector<stmtNode*>::iterator i=stmts.begin(); i!=stmts.end(); i++) {
		int i_stmt_num = (*i)->get_stmt_num ();
		// Go over all previous statements to figure out dependences. 
		for (vector<stmtNode*>::iterator j=stmts.begin(); j!=i; j++) {
			int j_stmt_num = (*j)->get_stmt_num ();
			// Only compute dependences if the statements have different original number
			if (i_stmt_num == j_stmt_num) 
				continue;

			exprNode *j_lhs = (*j)->get_lhs_expr ();
			exprNode *i_lhs = (*i)->get_lhs_expr ();
			exprNode *j_rhs = (*j)->get_rhs_expr ();
			exprNode *i_rhs = (*i)->get_rhs_expr ();

			// 1. WAW dependence 
			// only if either of them is an assignment
			bool waw_candidate = (*i)->get_op_type() == ST_EQ || (*j)->get_op_type() == ST_EQ;
			waw_candidate &= i_lhs->waw_dependence (j_lhs);
			if (waw_candidate) {
				if (DEBUG)
					printf ("Found WAW dependence between source cluster %d and dest cluster %d\n", j_stmt_num, i_stmt_num);
				if (waw_dependence_graph.find (i_stmt_num) == waw_dependence_graph.end ()) 
					waw_dependence_graph[i_stmt_num].push_back (j_stmt_num);
				else if (find (waw_dependence_graph[i_stmt_num].begin(), waw_dependence_graph[i_stmt_num].end(), j_stmt_num) == waw_dependence_graph[i_stmt_num].end())
					waw_dependence_graph[i_stmt_num].push_back (j_stmt_num); 
			}
			// 2. Accumulation dependence
			// only if dest is +=, and both have same lhs
			bool accumulation_candidate = (*i)->get_op_type() != ST_EQ;
			accumulation_candidate &= i_lhs->waw_dependence (j_lhs);
			if (accumulation_candidate) {
				if (DEBUG)
					printf ("Found ACC dependence between source cluster %d and dest cluster %d\n", j_stmt_num, i_stmt_num);
				if (accumulation_dependence_graph.find (i_stmt_num) == accumulation_dependence_graph.end ()) 
					accumulation_dependence_graph[i_stmt_num].push_back (j_stmt_num);
				else if (find (accumulation_dependence_graph[i_stmt_num].begin(), accumulation_dependence_graph[i_stmt_num].end(), j_stmt_num) == accumulation_dependence_graph[i_stmt_num].end())
					accumulation_dependence_graph[i_stmt_num].push_back (j_stmt_num); 
			}
			// 3. WAR dependence
			bool war_candidate = j_rhs->war_dependence (i_lhs);
			if (war_candidate) {
				if (DEBUG)
					printf ("Found WAR dependence between source cluster %d and dest cluster %d\n", j_stmt_num, i_stmt_num);
				if (war_dependence_graph.find (i_stmt_num) == war_dependence_graph.end ()) 
					war_dependence_graph[i_stmt_num].push_back (j_stmt_num);
				else if (find (war_dependence_graph[i_stmt_num].begin(), war_dependence_graph[i_stmt_num].end(), j_stmt_num) == war_dependence_graph[i_stmt_num].end())
					war_dependence_graph[i_stmt_num].push_back (j_stmt_num); 
			}
			// 4. RAW dependence
			bool raw_candidate = i_rhs->raw_dependence (j_lhs); 
			if (raw_candidate) {
				if (DEBUG)
					printf ("Found RAW dependence between source cluster %d and dest cluster %d\n", j_stmt_num, i_stmt_num);
				if (raw_dependence_graph.find (i_stmt_num) == raw_dependence_graph.end ()) 
					raw_dependence_graph[i_stmt_num].push_back (j_stmt_num);
				else if (find (raw_dependence_graph[i_stmt_num].begin(), raw_dependence_graph[i_stmt_num].end(), j_stmt_num) == raw_dependence_graph[i_stmt_num].end())
					raw_dependence_graph[i_stmt_num].push_back (j_stmt_num); 
			}
		}
	}
}

void stencilDefn::print_dependences (string name) {
	cout << "\nDependence graph for stencil " << name << " : " << endl;
	if (!raw_dependence_graph.empty ()) {
		cout << "RAW dependences: " << endl;
		for (auto d : raw_dependence_graph) {
			cout << indent << d.first << " - ";
			vector<int> rhs = d.second;
			for (auto s : d.second) 
				cout << s << " ";
			cout << endl;
		}
	}
	if (!war_dependence_graph.empty ()) {
		cout << "WAR dependences: " << endl;
		for (auto d : war_dependence_graph) {
			cout << indent << d.first << " - ";
			vector<int> rhs = d.second;
			for (auto s : d.second) 
				cout << s << " ";
			cout << endl;
		}
	}
	if (!waw_dependence_graph.empty ()) {
		cout << "WAW dependences: " << endl;
		for (auto d : waw_dependence_graph) {
			cout << indent << d.first << " - ";
			vector<int> rhs = d.second;
			for (auto s : d.second) 
				cout << s << " ";
			cout << endl;
		}
	}
	if (!accumulation_dependence_graph.empty ()) {
		cout << "ACC dependences: " << endl;
		for (auto d : accumulation_dependence_graph) {
			cout << indent << d.first << " - ";
			vector<int> rhs = d.second;
			for (auto s : d.second) 
				cout << s << " ";
			cout << endl;
		}
	}
}

stencilDefn* funcDefn::get_stencil_defn (string name) {
	stencilDefn *result = NULL;
	bool found = false;
	for (auto s : get_stencil_defns ()) {
		if (name.compare (s.first) == 0) {
			result = s.second;
			found = true;
			break;
		}
	}
	assert (found && "Called stencil not found (get_stencil_defn)");
	return result;
}

void funcDefn::print_stencil_calls (map<string, map<int,vector<int>>> &stencil_clusters, stringstream &out) {
	for (auto sc : stencil_calls) {
		vector<Range *> dom = sc->get_domain ();
		string stencil_name = sc->get_name ();
		vector<string> args = sc->get_arg_list ();
		int cluster_num = 0;
		for (map<int,vector<int>>::iterator c=stencil_clusters[stencil_name].begin(); c!=stencil_clusters[stencil_name].end(); c++,cluster_num++) {
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
			out << stencil_name;
			if (stencil_clusters[stencil_name].size() > 0)
				out << cluster_num;
			out << " (";
			// Get the accesess in the statements
			set<string> accesses;
			vector<int> cluster_stmts = c->second;
			cluster_stmts.push_back (c->first);
			stencilDefn *called_sdefn = get_stencil_defn (stencil_name);
			for (auto stmt : called_sdefn->get_stmt_list()) {
				int stmt_num = stmt->get_stmt_num ();
				if (find (cluster_stmts.begin(), cluster_stmts.end(), stmt_num) == cluster_stmts.end())
					continue;	
				stringstream lhs_out, rhs_out;
				exprNode *lhs = stmt->get_lhs_expr ();
				exprNode *rhs = stmt->get_rhs_expr ();
				lhs->collect_accesses (accesses);
				rhs->collect_accesses (accesses);
			}
			// Print the arguments
			bool first = true;
			for (vector<string>::iterator a=args.begin(); a!=args.end(); a++) {
				if (accesses.find (*a) == accesses.end())
					continue;
				if (!first)
					out << ", ";
				out << *a;
				first = false;
			}
			out << ");\n";
		}
	}
}

void funcDefn::determine_stmt_resource_mapping (void) {
	// Go over each statement, and determine the mapping.
	map<string,stencilDefn*> sdefns = stencil_defns->get_symbol_map ();
	for (auto sd : sdefns) {
		stencilDefn *sdefn = sd.second;
		string stencil_name = sd.first;
		sdefn->determine_stmt_resource_mapping ();
		if (DEBUG) {
			sdefn->print_stmt_resource_map (sd.first);
		}
		// Offset planes in case of streaming
		if (sdefn->perform_streaming ()) {
			sdefn->offset_dependent_planes ();
			sdefn->print_stencil_defn (sd.first);
		}
		sdefn->amend_resource_map ();
	}
}

void funcDefn::compute_hull (void) {
	// Go over each statement, and determine the mapping.
	map<string,stencilDefn*> sdefns = stencil_defns->get_symbol_map ();
	for (auto sd : sdefns) {
		stencilDefn *sdefn = sd.second;
		string stencil_name = sd.first;
		vector<Range*> stencil_domain;
		bool domain_found = false;
		for (auto sc : stencil_calls) {
			string called_stencil = sc->get_name ();
			if (stencil_name.compare (called_stencil) == 0) {
				stencil_domain = sc->get_domain ();
				domain_found = true;
			}
		}
		assert (domain_found && "Stencil domain not found (compute_hull)");
		cout << "\nFor stencil " << stencil_name << " :\n";
		sdefn->set_initial_hull (stencil_domain);
		vector<int> pointwise_occurrences;
		sdefn->compute_pointwise_occurrences (pointwise_occurrences);
		sdefn->compute_hull (stencil_domain, pointwise_occurrences);
		sdefn->compute_resource_hull (stencil_domain, pointwise_occurrences);
	}
}

void funcDefn::print_device_code (stringstream &device) {
	map<string,stencilDefn*> sdefns = stencil_defns->get_symbol_map ();
	stringstream constant_out;
	print_constant_memory (constant_out);
	if (!(constant_out.str()).empty())
		device << "\n" << constant_out.str();
	for (auto sd : sdefns) {
		stringstream out, header_out, decl_out, shm_out, reg_out, stmt_out, rotate_out;
		stencilDefn *sdefn = sd.second;
		sdefn->print_stencil_header (sd.first, header_out);
		sdefn->print_declarations (sd.first, decl_out);
		bool print_condition = sdefn->print_shm_initializations (shm_out);
		sdefn->print_reg_initializations (reg_out, print_condition);
		sdefn->print_mapped_stencil_stmts (stmt_out, (sdefn->get_copyouts()));
		sdefn->print_rotating_planes (rotate_out);
		// Now put all the strings into out
		out << header_out.str() << decl_out.str() << shm_out.str() << reg_out.str() << stmt_out.str() << rotate_out.str();
		out << "}\n\n";
		device << out.str ();	
		cout << "\n\nPrinting stencil out : \n" << out.str ();  
	}
}

void funcDefn::print_host_code (stringstream &host) {
	stringstream free_mem, copy_in, copy_constant, copy_out, launch_params, kernel_calls;
	host << "\nextern \"C\" void host_code (";
	// Array arguments
	bool first = true;
	for (vector<arrayDecl*>::iterator a=array_decls.begin(); a!=array_decls.end(); a++) {
		string name = (*a)->get_array_name ();
		DATA_TYPE t = (*a)->get_array_type ();
		if (!first) 
			host << ", ";
		host << print_data_type (t) << "*h_" << name;
		first = false;
	}
	// Scalar arguments
	map<string,DATA_TYPE> vd_map = var_decls->get_symbol_map ();
	for (map<string,DATA_TYPE>::iterator m=vd_map.begin(); m!=vd_map.end(); m++) {
		if (!first)
			host << ", ";
		host << print_data_type (m->second) << m->first;
		first = false;
	}
	// Parameters
	vector<string> params = get_parameters ();
	for(vector<string>::iterator p=params.begin(); p!=params.end(); p++) {
		if (!first)
			host << ", ";
		host << "int " << *p;
		first = false;
	}
	// Stencil call iterators
	for(vector<string>::iterator p=sc_iterators.begin(); p!=sc_iterators.end(); p++) {
		if (!first)
			host << ", ";
		host << "int " << *p;
		first = false;
	}
	host << ") {\n";
	// Print the cudaMalloc based on allocin 
	for (auto in : allocins) {
		// Look for the value in arrays
		for (auto a : array_decls) {
			string name = a->get_array_name ();
			if (name.compare (in) == 0) {
				DATA_TYPE t = a->get_array_type ();
				copy_in << indent << print_data_type (t) << "*" << name << ";\n";
				copy_in << indent << "cudaMalloc (&" << name << ", sizeof(" << print_data_type (t) << ")";
				// Print the bounds
				vector<Range*> range = a->get_array_range ();
				for (auto r : range) {
					exprNode *lo = r->get_lo_range ();
					exprNode *hi = r->get_hi_range ();
					char *name = (char *) "temp";
					binaryNode *temp = new binaryNode (T_MINUS, hi, lo, name, INT, true); 
					stringstream temp_out;
					temp->print_node (temp_out);
					copy_in << "*";
					copy_in << temp_out.str (); 
				}
				copy_in << ");\n";
				copy_in << indent << "check_error (\"Failed to allocate device memory for " << name << "\\n\");\n";
			}
		}
		// Look for the value in temporary arrays
		for (auto a : temp_array_decls) {
			string name = a->get_array_name ();
			if (name.compare (in) == 0) {
				DATA_TYPE t = a->get_array_type ();
				copy_in << indent << print_data_type (t) << "*" << name << ";";
				copy_in << indent << "cudaMalloc (&" << name << ", sizeof(" << print_data_type (t) << ")";
				// Print the bounds
				vector<Range*> range = a->get_array_range ();
				for (auto r : range) {
					exprNode *lo = r->get_lo_range ();
					exprNode *hi = r->get_hi_range ();
					char *name = (char *) "temp";
					binaryNode *temp = new binaryNode (T_MINUS, hi, lo, name, INT, true); 
					stringstream temp_out;
					temp->print_node (temp_out);
					copy_in << "*";
					copy_in << temp_out.str ();
				}
				copy_in << ");\n";
				copy_in << indent << "check_error (\"Failed to allocate device memory for " << name << "\\n\");\n";
			}
		}
	}
	// Print the cudaMalloc and memcpy based on copyin
	for (auto in : copyins) {
		// Look for the value in arrays
		for (auto a : array_decls) {
			string name = a->get_array_name ();
			if (name.compare (in) == 0) {
				DATA_TYPE t = a->get_array_type ();
				copy_in << indent << print_data_type (t) << "*" << name << ";\n";
				copy_in << indent << "cudaMalloc (&" << name << ", sizeof(" << print_data_type (t) << ")";
				// Print the bounds
				vector<Range*> range = a->get_array_range ();
				for (auto r : range) {
					exprNode *lo = r->get_lo_range ();
					exprNode *hi = r->get_hi_range ();
					char *name = (char *) "temp";
					binaryNode *temp = new binaryNode (T_MINUS, hi, lo, name, INT, true); 
					stringstream temp_out;
					temp->print_node (temp_out);
					copy_in << "*";
					copy_in << temp_out.str (); 
				}
				copy_in << ");\n";
				copy_in << indent << "check_error (\"Failed to allocate device memory for " << name << "\\n\");\n";
				copy_in << indent << "cudaMemcpy (" << name << ", h_" << name << ", sizeof(" << print_data_type (t) << ")";
				for (auto r : range) {
					exprNode *lo = r->get_lo_range ();
					exprNode *hi = r->get_hi_range ();
					char *name = (char *) "temp";
					binaryNode *temp = new binaryNode (T_MINUS, hi, lo, name, INT, true);
					stringstream temp_out;
					temp->print_node (temp_out);
					copy_in << "*";
					copy_in << temp_out.str ();
				}
				copy_in << ", cudaMemcpyHostToDevice);\n";
			}
		}
		// Look for the value in temporary arrays
		for (auto a : temp_array_decls) {
			string name = a->get_array_name ();
			if (name.compare (in) == 0) {
				DATA_TYPE t = a->get_array_type ();
				copy_in << indent << print_data_type (t) << "*" << name << ";";
				copy_in << indent << "cudaMalloc (&" << name << ", sizeof(" << print_data_type (t) << ")";
				// Print the bounds
				vector<Range*> range = a->get_array_range ();
				for (auto r : range) {
					exprNode *lo = r->get_lo_range ();
					exprNode *hi = r->get_hi_range ();
					char *name = (char *) "temp";
					binaryNode *temp = new binaryNode (T_MINUS, hi, lo, name, INT, true); 
					stringstream temp_out;
					temp->print_node (temp_out);
					copy_in << "*";
					copy_in << temp_out.str ();
				}
				copy_in << ");\n";
				copy_in << indent << "check_error (\"Failed to allocate device memory for " << name << "\\n\");\n";
				copy_in << indent << "cudaMemcpy (" << name << ", h_" << name << ", sizeof(" << print_data_type (t) << ")";
				for (auto r : range) {
					exprNode *lo = r->get_lo_range ();
					exprNode *hi = r->get_hi_range ();
					char *name = (char *) "temp";
					binaryNode *temp = new binaryNode (T_MINUS, hi, lo, name, INT, true); 
					stringstream temp_out;
					temp->print_node (temp_out);
					copy_in << "*";
					copy_in << temp_out.str ();
				}
				copy_in << ", cudaMemcpyHostToDevice);\n";
			}
		}
	}
	// Print the copy to constants
	for (auto cst : constants) {
		for (auto a : array_decls) {
			string name = a->get_array_name ();
			if (name.compare (cst) == 0) {
				DATA_TYPE t = a->get_array_type ();
				copy_constant << indent << "cudaMemcpyToSymbol (" << name << ", h_" << name << ", sizeof(" << print_data_type (t) << ")";
				// Print the bounds
				vector<Range*> range = a->get_array_range ();
				for (auto r : range) {
					exprNode *lo = r->get_lo_range ();
					exprNode *hi = r->get_hi_range ();
					char *name = (char *) "temp";
					binaryNode *temp = new binaryNode (T_MINUS, hi, lo, name, INT, true); 
					stringstream temp_out;
					temp->print_node (temp_out);
					copy_constant << "*";
					copy_constant << temp_out.str ();
				}
				copy_constant << ");\n"; 
			}
		}
	}
	// Print the memcpy on copyout
	for (auto out : copyouts) {
		// Look for the value in arrays
		for (auto a : array_decls) {
			string name = a->get_array_name ();
			if (name.compare (out) == 0) {
				DATA_TYPE t = a->get_array_type ();
				copy_out << indent << "cudaMemcpy (h_" << name << ", " << name << ", sizeof(" << print_data_type (t) << ")";
				free_mem << indent << "cudaFree (" << name << ");\n";
				// Print the bounds
				vector<Range*> range = a->get_array_range ();
				for (auto r : range) {
					exprNode *lo = r->get_lo_range ();
					exprNode *hi = r->get_hi_range ();
					char *name = (char *) "temp";
					binaryNode *temp = new binaryNode (T_MINUS, hi, lo, name, INT, true); 
					stringstream temp_out;
					temp->print_node (temp_out);
					copy_out << "*";
					copy_out << temp_out.str ();
				}
				copy_out << ", cudaMemcpyDeviceToHost);\n";
			}
		}
		// Look for the value in temporary arrays
		for (auto a : temp_array_decls) {
			string name = a->get_array_name ();
			if (name.compare (out) == 0) {
				DATA_TYPE t = a->get_array_type ();
				copy_out << indent << "cudaMemcpy (h_" << name << ", " << name << ", sizeof(" << print_data_type (t) << ")";
				free_mem << indent << "cudaFree (" << name << ");\n";
				// Print the bounds
				vector<Range*> range = a->get_array_range ();
				for (auto r : range) {
					exprNode *lo = r->get_lo_range ();
					exprNode *hi = r->get_hi_range ();
					char *name = (char *) "temp";
					binaryNode *temp = new binaryNode (T_MINUS, hi, lo, name, INT, true); 
					stringstream temp_out;
					temp->print_node (temp_out);
					copy_out << "*";
					copy_out << temp_out.str ();
				}
				copy_out << ", cudaMemcpyDeviceToHost);\n";
			}
		}
	}

	// Print the stencil calls
	int call_num = 1;
	Range *current_iteration_dom;
	bool dom_open = false;
	for (auto sc : stencil_calls) {
		// Get the current iteration domain
		Range *it = sc->get_iterations ();
		exprNode *lo = it->get_lo_range ();
		exprNode *hi = it->get_hi_range ();
		bool print_iterator = true;
		if (call_num > 1) {
			// Check if there has been a status change
			bool changed = false;
			exprNode *cur_lo = current_iteration_dom->get_lo_range ();
			exprNode *cur_hi = current_iteration_dom->get_hi_range ();
			if (!cur_lo->same_expr (cur_hi)) {
				changed |= !cur_lo->same_expr (lo);
				changed |= !cur_hi->same_expr (hi);
			}
			if (changed) { 
				kernel_calls << indent << "}";
				dom_open = false;
			}
			print_iterator = changed;
		}
		print_iterator &= !lo->same_expr (hi);

		vector<string> blockdims = ((get_iterators()).size() == 3) ? vector<string>({"x", "y", "z"}) : (((get_iterators()).size() == 2) ? vector<string>({"x", "y"}) : vector<string> ({"x"}));
		launch_params << indent << "dim3 blockconfig_" << call_num << " (";
		int pos=0;
		for (vector<string>::iterator b=blockdims.begin(); b!=blockdims.end(); b++,pos++) {
			if (stream && stream_dim.compare ((get_iterators())[blockdims.size()-pos-1]) == 0)
				launch_params << "1";
			else	
				launch_params << "b" << *b;
			if (b != prev(blockdims.end()))
				launch_params << ", ";
		}
		launch_params << ");\n";
		launch_params << indent << "dim3 gridconfig_" << call_num << " (";
		// Get the domain size from stencil call, else use parameters
		vector<Range*> dom = sc->get_domain ();
		int range_num = 0;
		map<string,int> udecls = get_unroll_decls ();
		if (!dom.empty ()) {
			stencilDefn *called_sdefn = get_stencil_defn (sc->get_name());
			vector<Range*> recompute_hull = called_sdefn->get_recompute_hull();
			vector<Range*>::reverse_iterator rh = recompute_hull.rbegin();
			for (vector<Range*>::reverse_iterator d=dom.rbegin(); d!=dom.rend(); d++,rh++) {
				if (full_stream && stream_dim.compare ((get_iterators())[blockdims.size()-range_num-1]) == 0)
					launch_params << 1;
				else if (stream && stream_dim.compare ((get_iterators())[blockdims.size()-range_num-1]) == 0) {
					exprNode *lo_range = (*d)->get_lo_range ();
					exprNode *hi_range = (*d)->get_hi_range ();
					// Really weird - had to move this before the print_node() call, otherwise rh was corrupted! 
					binaryNode *temp1 = new binaryNode (T_MINUS, hi_range, lo_range, false);
					binaryNode *temp = new binaryNode (T_PLUS, temp1, new datatypeNode<int> (1, INT), false);
					stringstream temp_out;
					temp->print_node (temp_out);
					launch_params << "ceil (";
					launch_params << temp_out.str () << ", ";
					int block_sz = (get_block_dims())[blockdims[range_num]];
					launch_params << block_sz << ")";
				}	
				else {
					exprNode *lo_range = (*d)->get_lo_range ();
					exprNode *hi_range = (*d)->get_hi_range ();
					// Really weird - had to move this before the print_node() call, otherwise rh was corrupted! 
					exprNode *lo = (*rh)->get_lo_range ();
					exprNode *hi = (*rh)->get_hi_range ();
					binaryNode *temp1 = new binaryNode (T_MINUS, hi_range, lo_range, false);
					binaryNode *temp = new binaryNode (T_PLUS, temp1, new datatypeNode<int> (1, INT), false);
					stringstream temp_out;
					temp->print_node (temp_out);
					launch_params << "ceil (";
					launch_params << temp_out.str () << ", ";
					string iter_id = (get_iterators())[blockdims.size()-1-range_num];
					int ufactor = (udecls.find (iter_id) != udecls.end()) ? udecls[iter_id] : 1;
					if (ufactor > 1) 
						launch_params << ufactor << "*";
					launch_params << "blockconfig_" << call_num << "." << blockdims[range_num];
					// Adjust the block size wrt hull
					string lo_id = "", hi_id = "";
					int lo_val = 0, hi_val = 0;
					lo->decompose_access_fsm (lo_id, lo_val);
					hi->decompose_access_fsm (hi_id, hi_val);
					int offset = hi_val - lo_val + 1;
					if (offset < 0)
						launch_params << offset;
					launch_params << ")";
				}
				if (d != prev(dom.rend()))
					launch_params << ", ";
				range_num++; 
			}
		}
		else {
			stencilDefn *called_sdefn = get_stencil_defn (sc->get_name());
			vector<Range*>::reverse_iterator rh = (called_sdefn->get_recompute_hull()).rbegin();
			for (vector<string>::reverse_iterator p=parameters.rbegin(); p!=parameters.rend(); p++,rh++) {
				if (full_stream && stream_dim.compare ((get_iterators())[blockdims.size()-range_num-1]) == 0)
					launch_params << 1;
				else if (stream && stream_dim.compare ((get_iterators())[blockdims.size()-range_num-1]) == 0) {
					launch_params << "ceil (";
					launch_params << *p << ", ";
					int block_sz = (get_block_dims())[blockdims[range_num]];
					launch_params << block_sz << ")";
				}
				else {
					launch_params << "ceil (";
					launch_params << *p << ", ";
					string iter_id = (get_iterators())[blockdims.size()-1-range_num];
					int ufactor = (udecls.find (iter_id) != udecls.end()) ? udecls[iter_id] : 1;
					if (ufactor > 1) 
						launch_params << ufactor << "*";
					launch_params << "blockconfig_" << call_num << "." << blockdims[range_num];
					// Adjust the block size wrt hull
					exprNode *lo = (*rh)->get_lo_range ();
					exprNode *hi = (*rh)->get_hi_range ();
					string lo_id = "", hi_id = "";
					int lo_val = 0, hi_val = 0;
					lo->decompose_access_fsm (lo_id, lo_val);
					hi->decompose_access_fsm (hi_id, hi_val);
					int offset = hi_val - lo_val + 1;
					if (offset < 0)
						launch_params << offset;
					launch_params << ")";
				}
				if (p!= prev(parameters.rend()))
					launch_params << ", ";
				range_num++; 
			}
		}
		launch_params << ");\n";

		// Print the stencil call iterator if specified
		if (print_iterator) {
			kernel_calls << indent << "for (int t=";
			string lo_id = "", hi_id = "";
			int lo_val = 0, hi_val = 0;
			lo->decompose_access_fsm (lo_id, lo_val);
			hi->decompose_access_fsm (hi_id, hi_val);
			if (!lo_id.empty()) {
				kernel_calls << lo_id;
				if (lo_val > 0)
					kernel_calls << "+";	
			}
			kernel_calls << lo_val;
			kernel_calls << "; t<=";
			if (!hi_id.empty()) {
				kernel_calls << hi_id;
				if (hi_val > 0)
					kernel_calls << "+";	
			}
			kernel_calls << hi_val;
			kernel_calls << "; t++) {\n";
			dom_open = true;
		}

		string st_indent = dom_open ? (indent + indent) : indent;
		string name = sc->get_name ();
		vector<string> args = sc->get_arg_list ();
		kernel_calls << st_indent << name << " <<<gridconfig_" << call_num << ", blockconfig_" << call_num << ">>> (";
		// Print the arguments
		bool first_arg = true;
		for (vector<string>::iterator a=args.begin(); a!=args.end(); a++) {
			// Check if a must be added to constant memory
			bool is_constant = false;
			for (auto cst : constants) {
				if (cst.compare (*a) == 0)
					is_constant = true;
			}
			if (!is_constant) {
				if (!first_arg) 
					kernel_calls << ", ";
				else 
					first_arg = false;
				kernel_calls << *a;
			}
		}
		// Print the parameters, if they are not already in the args
		for (auto p : params) {
			if (find (args.begin(), args.end(), p) == args.end()) {
				kernel_calls << ", " << p;	
			}
		}
		kernel_calls << ");\n";
		call_num++;
		current_iteration_dom = sc->get_iterations ();
	}
	if (dom_open)
		kernel_calls << indent << "}\n";
	// Write out all the data
	if (!(copy_in.str()).empty())
		host << copy_in.str () << "\n";
	if (!(copy_constant.str()).empty())	
		host << copy_constant.str () << "\n";
	if (!(launch_params.str()).empty())
		host << launch_params.str () << "\n";
	if (!(kernel_calls.str()).empty())
		host << kernel_calls.str () << "\n";
	if (!(copy_out.str()).empty())
		host << copy_out.str ();
	host << "}"; 
}

arrayDecl *funcDefn::get_array_decl (string s) {
	for (auto a : array_decls) {
		string array_name = a->get_array_name ();
		if (array_name.compare (s) == 0) 
			return a;	
	}
	for (auto a : temp_array_decls) {
		string array_name = a->get_array_name ();
		if (array_name.compare (s) == 0) 
			return a;	
	}
	return NULL;
}

void funcDefn::get_arg_info (string d, DECL_TYPE &decl_type, DATA_TYPE &data_type) {
	bool found = false;
	// Check if d is in var_decl
	map<string,DATA_TYPE> vdecl = var_decls->get_symbol_map ();
	for (auto v : vdecl) {
		if (d.compare (v.first) == 0) { 
			decl_type = VAR_DECL;
			data_type = v.second;
			found = true;
		}
	}
	map<string,DATA_TYPE> tvdecl = temp_var_decls->get_symbol_map ();
	for (auto v : tvdecl) {
		if (d.compare (v.first) == 0) {
			decl_type = VAR_DECL;
			data_type = v.second;
			found = true;
		}
	}
	for (auto a : array_decls) {
		string name = a->get_array_name ();
		if (d.compare (name) == 0) {
			decl_type = ARRAY_DECL;	
			data_type = a->get_array_type ();
			found = true;
		}
	}
	for (auto t : temp_array_decls) {
		string name = t->get_array_name ();
		if (d.compare (name) == 0) { 
			decl_type = ARRAY_DECL;	
			data_type = t->get_array_type ();
			found = true;
		}
	}
	if (!found) {
		cout << "Argument " << d << " does not match either array or variable declaration\n";
		exit (1);
	}
}

void funcDefn::set_data_type (void) {
	map<string,stencilDefn*> sdefn = stencil_defns->get_symbol_map ();
	for (auto sd : sdefn) {
		(sd.second)->set_data_type ();
	}
}

void funcDefn::map_args_to_parameters (map<tuple<string,string>,string> &parameter_map) {
	// Iterate over each stencil call and find the matching stencil definition
	for (auto sc : stencil_calls) {
		string called_stencil = sc->get_name ();
		vector<string> arg_list = sc->get_arg_list ();
		// Get the matching stencil definition
		map<string,stencilDefn*> sdefn = stencil_defns->get_symbol_map ();
		for (auto sd : sdefn) {
			string stencil_name = sd.first;
			if (stencil_name.compare (called_stencil) == 0) {
				vector<string> param_list = (sd.second)->get_arg_list ();
				// Now infer the datatype of parameters based on the arguments
				assert (param_list.size () == arg_list.size () && "Stencil parameters and call arguments differ (map_args_to_parameters)");
				vector<string>::iterator pi = param_list.begin();
				for (vector<string>::iterator ai=arg_list.begin(); ai!=arg_list.end(); ai++,pi++) {
					// Map the parameter to a var decl
					tuple<string,string> key = make_tuple (stencil_name, *pi);
					assert (parameter_map.find (key) == parameter_map.end ());
					parameter_map[key] = *ai;
				}
			}
		}
	}
}

void funcDefn::map_args_to_parameters (void) {
	// Iterate over each stencil call and find the matching stencil definition
	for (auto sc : stencil_calls) {
		string called_stencil = sc->get_name ();
		vector<string> arg_list = sc->get_arg_list ();
		// Get the matching stencil definition
		map<string,stencilDefn*> sdefn = stencil_defns->get_symbol_map ();
		for (auto sd : sdefn) {
			string stencil_name = sd.first;
			if (stencil_name.compare (called_stencil) == 0) {
				vector<string> param_list = (sd.second)->get_arg_list ();
				// Now infer the datatype of parameters based on the arguments
				assert (param_list.size () == arg_list.size () && "Stencil parameters and call arguments differ (map_args_to_parameters)");
				vector<string>::iterator pi = param_list.begin();
				for (vector<string>::iterator ai=arg_list.begin(); ai!=arg_list.end(); ai++,pi++) {
					// Check if ai is a variable or array declaration
					DECL_TYPE decl_type;
					DATA_TYPE data_type;	
					get_arg_info (*ai, decl_type, data_type);
					if (decl_type == VAR_DECL) {
						// Map the parameter to a var decl
						(sd.second)->push_var_decl (*pi, data_type);
					}
					else {
						// Map the parameter to an array decl
						arrayDecl *arg_arr = get_array_decl (*ai);
						arrayDecl *new_arr = new arrayDecl (*pi, arg_arr->get_array_range(), data_type);
						(sd.second)->push_array_decl (new_arr);		 
					}
				}
			}
		}
	}
}

void funcDefn::create_unique_stencil_defns (void) {
	// Create the necessary data structures
	map<string, vector<stencilCall*>> unique_calls;
	map<stencilCall*, stencilCall*> representative_call;

	// Identify the unique stencil calls based on the call domain
	for (auto sc : stencil_calls) {
		string stencil_name = sc->get_name ();
		vector<Range*> dom_a = sc->get_domain ();
		assert (!dom_a.empty () && "stencil call domain empty (create_unique_stencil_defns)");
		bool found = false;
		if (unique_calls.find (stencil_name) != unique_calls.end ()) {
			// Compare the two domains to see if they are the same
			for (auto uc : unique_calls[stencil_name]) {
				vector<Range*> dom_b = uc->get_domain ();
				if (dom_a.size () != dom_b.size ())
					continue;
				bool same_dom = true;
				vector<Range*>::iterator b = dom_b.begin();
				for (vector<Range*>::iterator a=dom_a.begin(); a!=dom_a.end(); a++,b++) {
					// Check if lo is same
					exprNode *a_lo_expr = (*a)->get_lo_range ();
					exprNode *b_lo_expr = (*b)->get_lo_range ();
					same_dom &= a_lo_expr->same_expr (b_lo_expr);
					// Check if hi is same
					exprNode *a_hi_expr = (*a)->get_hi_range ();
					exprNode *b_hi_expr = (*b)->get_hi_range ();
					same_dom &= a_hi_expr->same_expr (b_hi_expr);
				}
				if (same_dom) {
					found = true;
					// Make an entry in rep_map
					representative_call[sc] = uc; 
					break;
				}
			}
		}
		if (!found) {
			unique_calls[stencil_name].push_back (sc);
			representative_call[sc] = sc;
		}
	}
	// Create copies of stencil definitions for stencil calls to 
	// the same stencil function, but with different domains 
	for (auto uc : unique_calls) {
		string stencil_name = uc.first;
		vector<stencilCall*> second = uc.second;
		if (second.size () == 1)
			continue;
		// Iterate over the stencil calls, and find the calls with matching name
		for (auto sc : stencil_calls) {
			string called_stencil = sc->get_name ();
			if (called_stencil.compare (stencil_name) == 0) {
				assert (representative_call.find (sc) != representative_call.end ());
				vector<stencilCall*>::iterator it = find (second.begin(), second.end(), representative_call[sc]);
				assert (it != second.end ());
				int extension = distance (second.begin(), it);
				// The representative gets the original stencil function. Rest get copies 
				if (extension > 0) {
					bool leader = representative_call[sc] == sc;
					// Create a new stencil function (deep copy)
					map<string,stencilDefn*> sdefn = stencil_defns->get_symbol_map ();
					string new_name = called_stencil + "_v" + to_string (extension);
					if (leader) {
						for (auto sd : sdefn) {
							string sdefn_name = sd.first;
							if (sdefn_name.compare (called_stencil) == 0) {
								stencilDefn *new_stencil = new stencilDefn (*(sd.second));
								stencil_defns->push_symbol (new_name, new_stencil);
								break;	
							}
						}
					}
					// change the stencil name to call 
					sc->set_name (new_name);
				}
			}
		}
	}
	unique_calls.clear ();
	representative_call.clear ();
}

void funcDefn::verify_correctness (map<string,int> &funcnames) {
	map<string,int> test_map;
	// Verify uniqueness of parameters
	for (auto p : parameters) {
		//assert (test_map.find (p) == test_map.end () && "Parameters not unique (verify_correctness)");
		test_map[p] = 0; 
	}
	// Verify uniqueness of iterators
	for (auto i : iterators) {
		assert (test_map.find (i) == test_map.end () && "Iterators not unique (verify_correctness)");
		test_map[i] = 0; 
	}
	//If streaming, verify that the streaming dimension exists in iterators
	if (stream) {
		assert (!stream_dim.empty () && "Streaming dimension empty (verify_correctness)");
		assert ((find (iterators.begin(), iterators.end(), stream_dim) != iterators.end()) && "Streaming dim not found in iterators (verify_correctness)"); 
	} 
	// Verify uniqueness of coefficients 
	for (auto c : coefficients) {
		assert (test_map.find (c) == test_map.end () && "Coefficients not unique (verify_correctness)");
		test_map[c] = 0; 
	}
	// Verify uniqueness of copy-ins 
	for (auto i : copyins) {
		assert (test_map.find (i) == test_map.end () && "Copy-ins not unique (verify_correctness)");
		test_map[i] = 1; 
	}
	// Verify uniqueness of copy-outs
	for (auto o : copyouts) {
		bool confirms = false;
		if (test_map.find (o) == test_map.end ())
			confirms = true;
		else
			confirms = (test_map[o] == 1); 
		assert (confirms && "Copy-outs not unique (verify_correctness)");
		test_map[o] = 1; 
	}
	// Verify uniqueness of variable declarations
	map<string,DATA_TYPE> vdecl = var_decls->get_symbol_map ();
	for (auto v : vdecl) {
		bool confirms = false;
		if (test_map.find (v.first) == test_map.end ())
			confirms = true;
		else 
			confirms = (test_map[v.first] == 1);
		assert (confirms && "Variable declarations not unique (verify_correctness)");
		test_map[v.first] = 1; 
	}
	map<string,DATA_TYPE> tvdecl = temp_var_decls->get_symbol_map ();
	for (auto v : tvdecl) {
		bool confirms = false;
		if (test_map.find (v.first) == test_map.end ())
			confirms = true;
		else 
			confirms = (test_map[v.first] == 1);
		assert (confirms && "Temporary variable declarations not unique (verify_correctness)");
		test_map[v.first] = 1; 
	}
	// Verify uniquness of array declarations
	for (auto a : array_decls) {
		string name = a->get_array_name ();
		bool confirms = false;
		if (test_map.find (name) == test_map.end ())
			confirms = true;
		else 
			confirms = (test_map[name] == 1);
		assert (confirms && "Array declarations not unique (verify_correctness)"); 
		test_map[name] = 1; 	
	}
	for (auto t : temp_array_decls) {
		string name = t->get_array_name ();
		bool confirms = false;
		if (test_map.find (name) == test_map.end ())
			confirms = true;
		else 
			confirms = (test_map[name] == 1);
		assert (confirms && "Temporary array declarations not unique (verify_correctness)"); 
		test_map[name] = 1; 	
	}
	// Verify uniqueness of function names
	map<string,stencilDefn*> sdefn = stencil_defns->get_symbol_map ();
	for (auto sd : sdefn) {
		//// Set the iterators
		//(sd.second)->set_iterators (iterators); 
		// Compute dimensionality
		(sd.second)->compute_dimensionality ();
		assert ((sd.second)->get_dim () <= (int)parameters.size() && "There must be one parameter for each dimension (verify_correctness)");
		if (DEBUG) 
			cout << "\nFor stencil " << sd.first << ", dimensionality = " << (sd.second)->get_dim () << endl;

		assert (test_map.find (sd.first) == test_map.end () && "Stencil/Function names not unique (verify_correctness");
		assert (funcnames.find (sd.first) == funcnames.end () && "Stencil/Function names not unique (verify_correctness");
		funcnames[sd.first] = 0;
		// Verify uniqueness of arg list
		map<string,int> arg_map;
		vector<string> arglist = (sd.second)->get_arg_list ();
		for (auto a : arglist) {
			assert (arg_map.find (a) == arg_map.end () && "Argument name not unique (verify_correctness)");
			arg_map[a] = 0;
			// Verify that argument is either a variable or array declaration
			assert (test_map.find (a) != test_map.end () && "Argument not a variable or array declaration (verify_correctness)");	 
		}
		arg_map.clear ();
		// Verify that the accesses in the index expressions are immutable
		(sd.second)->verify_immutable_indices (); 
	}

	// Not a verification step exactly, but extracts the stencil call iterators
	for (auto sc : stencil_calls) {
		Range *it = sc->get_iterations ();
		exprNode *lo = it->get_lo_range ();
		exprNode *hi = it->get_hi_range ();
		string lo_id = "", hi_id = "";
		int lo_val = 0, hi_val = 0;
		lo->decompose_access_fsm (lo_id, lo_val);
		hi->decompose_access_fsm (hi_id, hi_val);
		if (!hi_id.empty ()) {
			bool add = find (parameters.begin(), parameters.end(), hi_id) == parameters.end();
			add &= find (sc_iterators.begin(), sc_iterators.end(), hi_id) == sc_iterators.end();
			if (add) 
				sc_iterators.push_back (hi_id);
		}
		if (!lo_id.empty ()) {
			bool add = find (parameters.begin(), parameters.end(), lo_id) == parameters.end();
			add &= find (sc_iterators.begin(), sc_iterators.end(), lo_id) == sc_iterators.end();
			if (add) 
				sc_iterators.push_back (lo_id);
		}
	}
	// Verify that sc_iterators do not occur elsewhere 
	for (auto i : sc_iterators) {
		assert (test_map.find (i) == test_map.end () && "sc_iterators not unique (verify_correctness)");
		test_map[i] = 1; 
	}

	test_map.clear ();

	// Not a verification step exactly, but creates missing domains for stencil calls
	for (auto sc : stencil_calls) {
		vector<Range*> dom = sc->get_domain ();
		if (dom.empty ()) {
			for (auto p : parameters) {
				// lo expr is 0
				exprNode *lo = new datatypeNode<int>(0, INT);
				exprNode *hi = new binaryNode (T_MINUS, new idNode (p), new datatypeNode<int>(1, INT), false);
				dom.push_back (new Range (lo, hi));
			}
			sc->set_domain (dom);
		}
	}
}

void startNode::verify_correctness (void) {
	map<string,int> funcnames;
	for (auto f : func_defns) {
		f->verify_correctness (funcnames);
	}
	funcnames.clear ();
}

void startNode::map_args_to_parameters (void) {
	for (auto f : func_defns) {
		f->map_args_to_parameters ();
		f->set_data_type ();
	}
}

// Fission of stencil. This does trivial fissions
void startNode::create_trivial_stencil_versions (void) {
	int version_num = 0;
	bool print_stencil = false;
	map<string, map<int,vector<int>>> stencil_clusters;

	stringstream prologue, epilogue, version0;
	// Print stencil bodies
	for (auto f : func_defns) {
		// Print the parameters
		vector<string> param = f->get_parameters ();
		if (!param.empty ()) {
			prologue << "parameter ";
			for (vector<string>::iterator p=param.begin(); p!=param.end(); p++) {
				prologue << *p;
				if (p != prev(param.end()))
					prologue << ", ";
			}
			prologue << ";\n";
		}
		// Print the iterator
		vector<string> iters = f->get_iterators ();
		if (!iters.empty ()) {
			prologue << "iterator ";
			for (vector<string>::iterator i=iters.begin(); i!=iters.end(); i++) {
				prologue << *i;
				if (i != prev(iters.end())) 
					prologue << ", ";
			}
			prologue << ";\n";
		}
		map<string,DATA_TYPE> vd_map = f->get_var_decls ();
		if (!vd_map.empty ()) {
			for (map<string,DATA_TYPE>::iterator m=vd_map.begin(); m!=vd_map.end(); m++) {
				prologue << print_data_type (m->second) << m->first;
				prologue << ";\n";
			}
			prologue << "\n";
		}
		vector<arrayDecl*> ad = f->get_array_decls ();
		if (!ad.empty ()) {
			for (vector<arrayDecl*>::iterator a=ad.begin(); a!=ad.end(); a++) {
				string name = (*a)->get_array_name ();
				DATA_TYPE t = (*a)->get_array_type ();
				prologue << print_data_type (t) << name;
				vector<Range*> range = (*a)->get_array_range ();
				for (auto r : range) {
					exprNode *lo = r->get_lo_range ();
					stringstream lo_out;
					lo->print_node (lo_out);
					exprNode *hi = r->get_hi_range ();
					stringstream hi_out;
					hi->print_node (hi_out);
					prologue << "[" << lo_out.str() << ":" << hi_out.str() << "]";
				}
				prologue << ";\n";	 
			}
		}
		// Print the copyin list
		vector<string> copyin = f->get_copyins ();
		if (!copyin.empty ()) {
			prologue << "copyin ";
			for (vector<string>::iterator i=copyin.begin(); i!=copyin.end(); i++) {
				prologue << *i;
				if (i != prev(copyin.end())) 
					prologue << ", ";
			}
			prologue << ";\n";
		}
		// Print the constants 
		vector<string> consts = f->get_constants ();
		if (!consts.empty ()) {
			prologue << "constant ";
			for (vector<string>::iterator i=consts.begin(); i!=consts.end(); i++) {
				prologue << *i;
				if (i != prev(consts.end())) 
					prologue << ", ";
			}
			prologue << ";\n";
		}
		// Print the coefficients
		vector<string> coefs = f->get_coefficients ();
		if (!coefs.empty ()) {
			prologue << "coefficient ";
			for (vector<string>::iterator i=coefs.begin(); i!=coefs.end(); i++) {
				prologue << *i;
				if (i != prev(coefs.end())) 
					prologue << ", ";
			}
			prologue << ";\n";
		}

		// Print the trivial version
		version0 << prologue.str ();
		map<string,stencilDefn*> sd = f->get_stencil_defns ();
		for (auto s : sd) {
			map<int, vector<int>> clusters;
			stencilDefn *sdefn = s.second;
			sdefn->identify_separable_clusters (s.first, clusters);
			stencil_clusters[s.first] = clusters;
			sdefn->print_stencil_body (s.first, clusters, version0);
			if (clusters.size() > 1) 
				print_stencil = true;
		}
		version0 << endl;
		f->print_stencil_calls (stencil_clusters, version0);
		// Print the copyout values	
		vector<string> copyout = f->get_copyouts ();
		if (!copyout.empty ()) {
			epilogue << "copyout ";
			for (vector<string>::iterator o=copyout.begin(); o!=copyout.end(); o++) {
				epilogue << *o;
				if (o != prev(copyout.end())) 
					epilogue << ", ";
			}
			epilogue << ";\n";
		}
		epilogue << endl;
		version0 << epilogue.str ();
	}

	// Write the modified stencil to file
	if (print_stencil) {
		string filename = "version" + to_string(version_num) + ".idsl";
		ofstream uf_out (filename, ofstream::out);
		uf_out << version0.rdbuf ();
		version_num++;
		uf_out.close ();
	}
}

//// Fission of stencil. This does trivial fissions
//void startNode::create_stencil_versions (void) {
//	int version_num = 0;
//	bool print_stencil = false;
//	map<string, map<int,vector<int>>> stencil_clusters;
//	map<int,map<string, map<int,vector<int>>>> new_stencil_clusters;
//	map<int,string> versions;
//
//	stringstream prologue, epilogue, version0;
//	// Print stencil bodies
//	for (auto f : func_defns) {
//		// Print the parameters
//		vector<string> param = f->get_parameters ();
//		if (!param.empty ()) {
//			prologue << "parameter ";
//			for (vector<string>::iterator p=param.begin(); p!=param.end(); p++) {
//				prologue << *p;
//				if (p != prev(param.end()))
//					prologue << ", ";
//			}
//			prologue << ";\n";
//		}
//		// Print the iterator
//		vector<string> iters = f->get_iterators ();
//		if (!iters.empty ()) {
//			prologue << "iterator ";
//			for (vector<string>::iterator i=iters.begin(); i!=iters.end(); i++) {
//				prologue << *i;
//				if (i != prev(iters.end())) 
//					prologue << ", ";
//			}
//			prologue << ";\n";
//		}
//		map<string,DATA_TYPE> vd_map = f->get_var_decls ();
//		if (!vd_map.empty ()) {
//			for (map<string,DATA_TYPE>::iterator m=vd_map.begin(); m!=vd_map.end(); m++) {
//				prologue << print_data_type (m->second) << m->first;
//				prologue << ";\n";
//			}
//			prologue << "\n";
//		}
//		vector<arrayDecl*> ad = f->get_array_decls ();
//		if (!ad.empty ()) {
//			for (vector<arrayDecl*>::iterator a=ad.begin(); a!=ad.end(); a++) {
//				string name = (*a)->get_array_name ();
//				DATA_TYPE t = (*a)->get_array_type ();
//				prologue << print_data_type (t) << name;
//				vector<Range*> range = (*a)->get_array_range ();
//				for (auto r : range) {
//					exprNode *lo = r->get_lo_range ();
//					stringstream lo_out;
//					lo->print_node (lo_out);
//					exprNode *hi = r->get_hi_range ();
//					stringstream hi_out;
//					hi->print_node (hi_out);
//					prologue << "[" << lo_out.str() << ":" << hi_out.str() << "]";
//				}
//				prologue << ";\n";	 
//			}
//		}
//		// Print the allocin list
//		vector<string> allocin = f->get_allocins ();
//		if (!allocin.empty ()) {
//			prologue << "allocin ";
//			for (vector<string>::iterator i=allocin.begin(); i!=allocin.end(); i++) {
//				prologue << *i;
//				if (i != prev(allocin.end())) 
//					prologue << ", ";
//			}
//			prologue << ";\n";
//		}
//		// Print the copyin list
//		vector<string> copyin = f->get_copyins ();
//		if (!copyin.empty ()) {
//			prologue << "copyin ";
//			for (vector<string>::iterator i=copyin.begin(); i!=copyin.end(); i++) {
//				prologue << *i;
//				if (i != prev(copyin.end())) 
//					prologue << ", ";
//			}
//			prologue << ";\n";
//		}
//		// Print the constants 
//		vector<string> consts = f->get_constants ();
//		if (!consts.empty ()) {
//			prologue << "constant ";
//			for (vector<string>::iterator i=consts.begin(); i!=consts.end(); i++) {
//				prologue << *i;
//				if (i != prev(consts.end())) 
//					prologue << ", ";
//			}
//			prologue << ";\n";
//		}
//		// Print the coefficients
//		vector<string> coefs = f->get_coefficients ();
//		if (!coefs.empty ()) {
//			prologue << "coefficient ";
//			for (vector<string>::iterator i=coefs.begin(); i!=coefs.end(); i++) {
//				prologue << *i;
//				if (i != prev(coefs.end())) 
//					prologue << ", ";
//			}
//			prologue << ";\n";
//		}
//
//		// Print the trivial version
//		version0 << prologue.str ();
//		map<string,stencilDefn*> sd = f->get_stencil_defns ();
//		for (auto s : sd) {
//			map<int, vector<int>> clusters;
//			stencilDefn *sdefn = s.second;
//			sdefn->identify_separable_clusters (s.first, clusters);
//			stencil_clusters[s.first] = clusters;
//			sdefn->print_stencil_body (s.first, clusters, version0);
//			if (clusters.size() > 1) 
//				print_stencil = true;
//
//			// Compute the number of flops per cluster
//			map<int,vector<int>,greater<int>> flops_per_cluster;
//			for (auto c : clusters) {
//				int flops = 0;
//				vector<int> cluster_stmts = c.second;
//				cluster_stmts.push_back (c.first);
//				for (auto stmt : sdefn->get_stmt_list()) {
//					int stmt_num = stmt->get_stmt_num ();
//					if (find (cluster_stmts.begin(), cluster_stmts.end(), stmt_num) == cluster_stmts.end())
//						continue;
//					exprNode *rhs = stmt->get_rhs_expr ();
//					flops += rhs->get_flop_count ();
//				}
//				(flops_per_cluster[flops]).push_back (c.first);
//				cout << "Flops for cluster " << c.first << " = " << flops << endl;
//			}
//			// Flops per cluster <= 128
//			int max_clusters = 2;
//			bool create_versions = max_clusters < (int)clusters.size();
//			while (create_versions) {
//				int max_flops_per_cluster = 0;
//				map<int, vector<int>> cluster_grouping;
//				vector<int> flop_sum (max_clusters, 0);
//				for (auto f : flops_per_cluster) {
//					int flop = f.first;
//					for (auto c : f.second) {
//						// Find the cluster with lowest flop count
//						int min_flops = INT_MAX, min_pos = 0;
//						for (int i=0; i<max_clusters; i++) {
//							if (flop_sum[i] < min_flops) {
//								min_flops = flop_sum[i];
//								min_pos = i;
//							}
//						}
//						// Add c to it
//						(cluster_grouping[min_pos]).push_back (c);
//						flop_sum[min_pos] += flop;
//						max_flops_per_cluster = max (max_flops_per_cluster, flop_sum[min_pos]);
//					}
//				}
//				for (int i=0; i<max_clusters; i++) {
//					cout << "flop_sum[" << i << "] = " << flop_sum[i] << endl;
//				}
//				for (auto cg : cluster_grouping) {
//					cout << "cluster_grouping[" << cg.first << "] = ";
//					for (auto g : cg.second) 
//						cout << g << " ";
//					cout << endl;
//				}
//				map<int,vector<int>> new_clusters;
//				for (auto cg : cluster_grouping) {
//					bool first = true;
//					int lhs; 
//					vector<int> rhs;
//					for (auto g : cg.second) {
//						if (first) {
//							lhs = g;
//							first = false;
//						}
//						else {
//							if (find (rhs.begin(), rhs.end(), g) == rhs.end())
//								rhs.push_back (g);
//						}
//						for (auto c : clusters[g]) {
//							if (find (rhs.begin(), rhs.end(), c) == rhs.end())
//								rhs.push_back (c);
//						}
//					}
//					new_clusters[lhs] = rhs;
//				}
//				cout << "New clusters when max_cluster = " << max_clusters << " :" << endl;
//				for (auto s : new_clusters) {
//					cout << s.first << " : ";
//					for (auto c : s.second) 
//						cout <<  c << " ";
//					cout << endl;
//				}
//				stringstream version;
//				(new_stencil_clusters[max_clusters])[s.first] = new_clusters;
//				sdefn->print_stencil_body (s.first, new_clusters, version);
//				versions[max_clusters] = versions[max_clusters] + version.str();
//				max_clusters++;
//				create_versions = max_flops_per_cluster > 128 && max_clusters < (int)clusters.size();
//			}
//		}
//		version0 << endl;
//		f->print_stencil_calls (stencil_clusters, version0);
//		// Print the copyout values	
//		vector<string> copyout = f->get_copyouts ();
//		if (!copyout.empty ()) {
//			epilogue << "copyout ";
//			for (vector<string>::iterator o=copyout.begin(); o!=copyout.end(); o++) {
//				epilogue << *o;
//				if (o != prev(copyout.end())) 
//					epilogue << ", ";
//			}
//			epilogue << ";\n";
//		}
//		epilogue << endl;
//		version0 << epilogue.str ();
//
//		// Now print the other versions 
//		for (auto s : versions) {
//			stringstream version;
//			version << prologue.str ();
//			version << s.second;
//			version << endl;
//			f->print_stencil_calls (new_stencil_clusters[s.first], version);
//			version << epilogue.str ();
//			versions[s.first] = version.str ();	
//		}
//	}
//
//	// Write the modified stencil to file
//	if (print_stencil) {
//		string filename = "version" + to_string(version_num) + ".idsl";
//		ofstream uf_out (filename, ofstream::out);
//		uf_out << version0.rdbuf ();
//		version_num++;
//		uf_out.close ();
//
//		for (auto s : versions) {
//			string filename = "version" + to_string(version_num) + ".idsl";
//			ofstream uf_out (filename, ofstream::out);
//			stringstream version;
//			version << s.second;
//			uf_out << version.rdbuf ();
//			version_num++;
//			uf_out.close ();
//		}
//	}
//
//	// Now a different split: Identify the largest 
//}

void startNode::decompose_statements (bool decompose, bool retime, bool fold_computations) {
	for (auto f : func_defns) {
		map<string,stencilDefn*> sd = f->get_stencil_defns ();
		for (auto s : sd) {
			stencilDefn *sdefn = s.second;
			sdefn->decompose_statements (decompose, retime, fold_computations);
		}
	}
}

void startNode::compute_dependences (void) {
	for (auto f : func_defns) {
		map<string,stencilDefn*> sd = f->get_stencil_defns ();
		for (auto s : sd) {
			stencilDefn *sdefn = s.second;
			sdefn->compute_dependences ();
			if (DEBUG) {
				sdefn->print_dependences (s.first);
			}
		}
	}
}

void startNode::set_load_style (bool b) {
	for (auto f : func_defns) {
		map<string,stencilDefn*> sd = f->get_stencil_defns ();
		for (auto s : sd) {
			(s.second)->set_load_style (b);
		}
	}
}

void startNode::set_loop_prefetch_strategy (bool p) {
	for (auto f : func_defns) {
		map<string,stencilDefn*> sd = f->get_stencil_defns ();
		for (auto s : sd) {
			(s.second)->set_loop_prefetch_strategy (p);
		}
	}
}

void startNode::set_loop_gen_strategy (bool b) {
	for (auto f : func_defns) {
		map<string,stencilDefn*> sd = f->get_stencil_defns ();
		for (auto s : sd) {
			(s.second)->set_loop_gen_strategy (b);
		}
	}
}

void startNode::set_halo_dims (map<string,tuple<int,int>> hd) {
	for (auto f : func_defns) {
		map<string,stencilDefn*> sd = f->get_stencil_defns ();
		for (auto s : sd) {
			(s.second)->set_halo_dims (hd);
		}
	}
}

void startNode::set_block_dims (symtabTemplate<int> *bd) {
	for (auto f : func_defns) {
		f->set_block_dims (bd);
		map<string,stencilDefn*> sd = f->get_stencil_defns ();
		for (auto s : sd) {
			(s.second)->set_block_dims (bd);
		}
	}
}

void startNode::set_allocins (void) { 
	for (auto f : func_defns) {
		map<string,stencilDefn*> sd = f->get_stencil_defns ();
		for (auto s : sd) {
			(s.second)->set_allocins (f->get_allocins ());
		}
	}
}

void startNode::set_copyins (void) { 
	for (auto f : func_defns) {
		map<string,stencilDefn*> sd = f->get_stencil_defns ();
		for (auto s : sd) {
			(s.second)->set_copyins (f->get_copyins ());
		}
	}
}

void startNode::set_constants (void) { 
	for (auto f : func_defns) {
		map<string,stencilDefn*> sd = f->get_stencil_defns ();
		for (auto s : sd) {
			(s.second)->set_constants (f->get_constants ());
		}
	}
}

void startNode::set_copyouts (void) { 
	for (auto f : func_defns) {
		map<string,stencilDefn*> sd = f->get_stencil_defns ();
		for (auto s : sd) {
			vector<string> formal_copyouts;
			//Get the last call to that stencil definition
			vector<stencilCall*> st_calls = f->get_stencil_calls ();
			for (vector<stencilCall*>::reverse_iterator it=st_calls.rbegin(); it!=st_calls.rend(); it++) {
				string called_stencil = (*it)->get_name ();
				if (called_stencil.compare (s.first) == 0) {
					vector<string> actual_args = (*it)->get_arg_list ();
					vector<string> formal_args = (s.second)->get_arg_list (); 	
					for (auto c : f->get_copyouts()) {
						vector<string>::iterator jt = find (actual_args.begin(), actual_args.end(), c);
						assert (jt != actual_args.end() && "copyout does not occur in the last stencil call");
						assert (actual_args.size() == formal_args.size());
						formal_copyouts.push_back (formal_args[jt-actual_args.begin()]);
					}
					break;
				}
			}
			(s.second)->set_copyouts (formal_copyouts);
		}
	}
}

void startNode::set_iterators (void) { 
	for (auto f : func_defns) {
		map<string,stencilDefn*> sd = f->get_stencil_defns ();
		for (auto s : sd) {
			(s.second)->set_iterators (f->get_iterators());
		}
	}
}

void startNode::set_parameters (void) { 
	for (auto f : func_defns) {
		map<string,stencilDefn*> sd = f->get_stencil_defns ();
		for (auto s : sd) {
			(s.second)->set_parameters (f->get_parameters());
		}
	}
}

void startNode::set_unroll_decls (symtabTemplate<int> *ud) {
	for (auto f : func_defns) {
		f->set_unroll_decls (ud);
		map<string,stencilDefn*> sd = f->get_stencil_defns ();
		for (auto s : sd) {
			(s.second)->set_unroll_decls (ud);
			//(s.second)->unroll_stmts ();
		}
	}
}

void startNode::set_gen_code_diff (bool diff) {
	for (auto f : func_defns) {
		map<string,stencilDefn*> sd = f->get_stencil_defns ();
		for (auto s : sd) {
			(s.second)->set_gen_code_diff (diff);
		}
	}
}

void startNode::set_linearize_accesses (bool linearize) {
	for (auto f : func_defns) {
		map<string,stencilDefn*> sd = f->get_stencil_defns ();
		for (auto s : sd) {
			(s.second)->set_linearize_accesses (linearize);
		}
	}
}

void startNode::set_use_shmem (bool use_shmem) {
	for (auto f : func_defns) {
		map<string,stencilDefn*> sd = f->get_stencil_defns ();
		for (auto s : sd) {
			(s.second)->set_use_shmem (use_shmem);
		}
	}
}

void startNode::set_streaming_dimension (bool full_stream, string stream) {
	for (auto f : func_defns) {
		f->set_streaming_dimension (full_stream, stream);
		// Default to the outermost iterator
		if (stream.empty ()) {
			vector<string> iters = f->get_iterators ();
			stream = iters.front ();
		}
		map<string,stencilDefn*> sd = f->get_stencil_defns ();
		for (auto s : sd) {
			(s.second)->set_streaming_dimension (full_stream, stream);
		}
	}
}

void startNode::generate_code (stringstream &opt) {
	stringstream host_out, device_out;	
	for (auto f : func_defns) {
		f->determine_stmt_resource_mapping ();
		f->create_unique_stencil_defns ();
		f->compute_hull ();
		f->print_device_code (device_out);
		f->print_host_code (host_out);
	}
	opt << device_out.str ();
	opt << host_out.str ();
}
