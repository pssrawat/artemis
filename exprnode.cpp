#include "exprnode.hpp"
using namespace std;

void idNode::collect_accesses (set<string> &accesses) {
	accesses.insert (name);
}

void idNode::print_node (stringstream &out) {
	if (nested) 
		out << "(";
	out << name;
	if (nested) 
		out << ")";
}

void idNode::print_node (stringstream &out, map<tuple<string,int,bool>,RESOURCE> rmap, bool is_lhs, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays, map<string,int> unroll_instance, bool blocked_loads, map<string,int> udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, string &cstream_shift) {
	if (unroll_instance.find (name) != unroll_instance.end()) {
		if (blocked_loads) {
			exprNode *temp = new binaryNode (T_PLUS, this, new datatypeNode<int>(unroll_instance[name], INT), type, false);
			temp->print_node (out);
		}
		else {
			vector<string> blockdims = (iters.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iters.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));
			print_node (out);
			for (vector<string>::iterator it=iters.begin(); it!=iters.end(); it++) {
				if (name.compare (*it) == 0) {
					out << "+"; 
					if (unroll_instance[name] > 1) 
						out << unroll_instance[name] << "*";
					out << "blockDim." << blockdims[it-iters.begin()];
				}
			}
		}
	}
	else
		print_node (out);
}

void idNode::print_node (stringstream &out, map<tuple<string,int,bool>,RESOURCE> rmap, bool is_lhs, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays, bool blocked_loads, map<string,int> udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, map<string,tuple<int,int>> halo_dims, int reg_count, string &cstream_shift) {
	if (udecls.find (name) != udecls.end()) {
		vector<string> blockdims = (iters.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iters.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));
		print_node (out);
		for (vector<string>::iterator it=iters.begin(); it!=iters.end(); it++) {
			if (name.compare (*it) == 0) {
				int ufactor = (udecls.find (*it) != udecls.end ()) ? udecls[*it] : 1;
				tuple<int,int> halo = (halo_dims.find (*it) != halo_dims.end ()) ? halo_dims[*it] : make_tuple (0, 0);
				if (ufactor > 1 || (get<0>(halo) != 0 || get<1>(halo) != 0)) 
					out << ufactor; 
			}
		}
	}
	else
		print_node (out);
}

void idNode::decompose_access_fsm (string &id, int &offset) {
	id = id + name;
}

void idNode::set_data_type (map<string, DATA_TYPE> data_types) {
	assert (data_types.find (name) != data_types.end ());
	type = data_types[name]; 
}

bool idNode::same_expr (exprNode *node) {
	bool same_expr = type == node->get_type ();
	same_expr &= expr_type == node->get_expr_type ();
	if (same_expr)
		same_expr &= name.compare (node->get_name ()) == 0;
	return same_expr; 
}

bool idNode::consolidate_same_accesses (vector<tuple<OP_TYPE,exprNode*>> &consolidated_exprs, map<exprNode*,vector<exprNode*>> &same_accesses, map<exprNode*,exprNode*> &cmap, exprNode* access, bool flip) {
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

bool idNode::identify_same_accesses (map<exprNode*,vector<exprNode*>> &same_accesses, exprNode *access) {
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

bool idNode::waw_dependence (exprNode *node) {
	return same_expr (node); 
}

bool idNode::raw_dependence (exprNode *node) {
	return same_expr (node); 
}

bool idNode::war_dependence (exprNode *node) {
	return same_expr (node); 
}

bool idNode::pointwise_occurrence (exprNode *node) {
	return true; 
}

bool idNode::verify_immutable_expr (vector<string> writes, bool start_verification) {
	if (start_verification) {
		bool result = true;
		for (auto w : writes) {
			result &= (name.compare (w) != 0);
		}
		return result;
	}
	return true;
}

void idNode::compute_hull (map<string, vector<Range*>> &hull_map, vector<Range*> &hull, vector<Range*> &initial_hull, bool stream, string stream_dim,  int offset) {
	if (hull_map.find(name) != hull_map.end()) {
		vector<Range*> new_hull = vector<Range*> (hull_map[name]);
		// Take an intersection with the current hull
		vector<Range*>::iterator b_dom = hull.begin();
		int pos = 0;
		for (vector<Range*>::iterator a_dom=new_hull.begin(); a_dom!=new_hull.end(); a_dom++,b_dom++,pos++) {
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
			hull[pos] = new Range (lo, hi);
		}
	}
}

void idNode::compute_hull (map<string, vector<Range*>> &hull_map, vector<Range*> &hull, vector<Range*> &initial_hull, map<tuple<string,int,bool>,RESOURCE> rmap, bool stream, string stream_dim, int offset) {
	if (hull_map.find(name) != hull_map.end()) {
		vector<Range*> new_hull = vector<Range*> (hull_map[name]);
		// Take an intersection with the current hull
		vector<Range*>::iterator b_dom = hull.begin();
		int pos = 0;
		for (vector<Range*>::iterator a_dom=new_hull.begin(); a_dom!=new_hull.end(); a_dom++,b_dom++,pos++) {
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
			hull[pos] = new Range (lo, hi);
		}
	}
}

exprNode *idNode::unroll_expr (string iter, int unroll_instance, map<string,int> &scalar_version_map, bool is_lhs) {
	exprNode *result;
	if (is_lhs) {
		int val = (scalar_version_map.find (name) == scalar_version_map.end ()) ? 0 : scalar_version_map[name]; 
		scalar_version_map[name] = val + 1;
	}

	if ((scalar_version_map.find (name) == scalar_version_map.end ()) || scalar_version_map[name] == 0)
		result = new idNode (name, type, false);
	else {
		string new_name = name + "_" + to_string (scalar_version_map[name]);
		result = new idNode (new_name, type, false); 
	}
	if ((name.compare (iter) == 0) && unroll_instance != 0) 
		result = new binaryNode (T_PLUS, result, new datatypeNode<int>(unroll_instance, INT), type, true);	
	return result;
}

exprNode *uminusNode::retime_node (string stream_dim, int offset) {
	uminusNode *ret = new uminusNode (*this);
	ret->base_expr = base_expr->retime_node (stream_dim, offset);
	return ret;
}

exprNode *uminusNode::distribute_node (void) {
	uminusNode *ret = new uminusNode (*this);
	ret->base_expr = (ret->get_base_expr())->distribute_node ();	
	return ret;
}

void uminusNode::remove_redundant_nesting (void) {
	base_expr->remove_redundant_nesting();
}

bool uminusNode::same_operator (OP_TYPE p) {
	return (base_expr->is_nested() || base_expr->same_operator(p)); 
}

void uminusNode::decompose_node (exprNode* lhs, vector<tuple<exprNode*,STMT_OP,exprNode*>> &substmts, bool flip) {
	base_expr->decompose_node (lhs, substmts, !flip);
}

bool uminusNode::identify_same_accesses (map<exprNode*,vector<exprNode*>> &same_accesses, exprNode* access) {
	return base_expr->identify_same_accesses (same_accesses, access);
}

bool uminusNode::consolidate_same_accesses (vector<tuple<OP_TYPE,exprNode*>> &consolidated_exprs, map<exprNode*,vector<exprNode*>> &same_accesses, map<exprNode*,exprNode*> &cmap, exprNode* access, bool flip) {
	return base_expr->consolidate_same_accesses (consolidated_exprs, same_accesses, cmap, access, !flip);
}

void uminusNode::print_node (stringstream &out) {
	if (nested)
		out << "(";
	out << "-";
	base_expr->print_node (out);
	if (nested)
		out << ")";
}

void uminusNode::decompose_access_fsm (string &id, int &offset) {
	string temp_id = "";
	int temp_offset = 0;
	base_expr->decompose_access_fsm (temp_id, temp_offset);
	if (temp_id.length () > 0)
		id = id + "-" + temp_id;
	offset += -1*temp_offset;
}

bool uminusNode::same_expr (exprNode *node) {
	bool result = (node->get_expr_type () == T_UMINUS);
	if (result) 
		result &= base_expr->same_expr (dynamic_cast<uminusNode*>(node)->get_base_expr());
	return result;
}

void uminusNode::print_node (stringstream &out, map<tuple<string,int,bool>,RESOURCE> rmap, bool is_lhs, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays, map<string,int> unroll_instance, bool blocked_loads, map<string,int> udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, string &cstream_shift) {
	if (nested) 
		out << "(";
	out << "-";
	base_expr->print_node (out, rmap, stream, is_lhs, full_stream, stream_dim, temp_arrays, unroll_instance, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, cstream_shift);
	if (nested)
		out << ")";
}

void uminusNode::print_node (stringstream &out, map<tuple<string,int,bool>,RESOURCE> rmap, bool is_lhs, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays, bool blocked_loads, map<string,int> udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, map<string,tuple<int,int>> halo_dims, int reg_count, string &cstream_shift) {
	if (nested) 
		out << "(";
	out << "-";
	base_expr->print_node (out, rmap, is_lhs, full_stream, stream, stream_dim, temp_arrays, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, halo_dims, reg_count, cstream_shift);
	if (nested)
		out << ")";
}

void uminusNode::print_last_write (stringstream &out, map<tuple<string,int,bool>,RESOURCE> rmap, map<string,exprNode*> last_writes, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays, map<string,int> unroll_instance, bool blocked_loads, map<string,int> udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, string &cstream_shift) {
	out << "-";
	base_expr->print_last_write (out, rmap, last_writes, full_stream, stream, stream_dim, temp_arrays, unroll_instance, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, cstream_shift);
}

void uminusNode::print_last_write (stringstream &out, map<tuple<string,int,bool>,RESOURCE> rmap, map<string,exprNode*> last_writes, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays, bool blocked_loads, map<string,int> udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, map<string,tuple<int,int>> halo_dims, int reg_count, string &cstream_shift) {
	out << "-";
	base_expr->print_last_write (out, rmap, last_writes, full_stream, stream, stream_dim, temp_arrays, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, halo_dims, reg_count, cstream_shift);
}

exprNode *uminusNode::unroll_expr (string iter, int unroll_instance, map<string,int> &scalar_version_map, bool is_lhs) {
	exprNode *temp = base_expr->unroll_expr (iter, unroll_instance, scalar_version_map, is_lhs);
	return new uminusNode (temp, type, nested);
}

bool binaryNode::is_scalar_expr (void) {
	bool ret = true;
	ret &= lhs->is_scalar_expr ();
	ret &= rhs->is_scalar_expr ();
	return ret;
}

exprNode *binaryNode::retime_node (string stream_dim, int offset) {
	binaryNode *new_node = new binaryNode (*this);
	new_node->lhs = lhs->retime_node (stream_dim, offset);
	new_node->rhs = rhs->retime_node (stream_dim, offset);
	return new_node;	
}

int binaryNode::get_retiming_offset (string stream_dim) {
	int lhs_offset = INT_MIN, rhs_offset = INT_MIN;
	lhs_offset = lhs->get_retiming_offset (stream_dim);
	rhs_offset = rhs->get_retiming_offset (stream_dim);
	return max (lhs_offset, rhs_offset);
}

bool binaryNode::retiming_feasible (string stream_dim, vector<int> &offset) {
	bool ret = lhs->retiming_feasible (stream_dim, offset);
	ret &= rhs->retiming_feasible (stream_dim, offset);
	return ret;	
}

bool binaryNode::same_subscripts_for_array (string arr_name, vector<string> &subscripts, DATA_TYPE &arr_type) {
	bool ret = true;
	ret &= lhs->same_subscripts_for_array (arr_name, subscripts, arr_type);
 	ret &= rhs->same_subscripts_for_array (arr_name, subscripts, arr_type);
	return ret;
}

void binaryNode::replace_array_name (string old_name, string new_name) {
	lhs->replace_array_name (old_name, new_name);
	rhs->replace_array_name (old_name, new_name);
}

int binaryNode::get_flop_count (void) {
	int lhs_flop_count = lhs->get_flop_count ();
	int rhs_flop_count = rhs->get_flop_count ();
	return flops_per_op (op) + lhs_flop_count + rhs_flop_count; 
}

void binaryNode::accessed_arrays (set<string> &arrays) {
	lhs->accessed_arrays (arrays);
	rhs->accessed_arrays (arrays);
}

bool binaryNode::identify_same_accesses (map<exprNode*,vector<exprNode*>> &same_accesses, exprNode *access) {
	bool inserted= false;
	if (op == T_MINUS || op == T_PLUS) {
		if (!lhs->is_nested())
			inserted |= lhs->identify_same_accesses (same_accesses, NULL);
		if (!rhs->is_nested())
			inserted |= rhs->identify_same_accesses (same_accesses, NULL);
	}
	if (op == T_MULT) {
		bool lhs_simple = (lhs->is_id_node() || lhs->is_shiftvec_node() || lhs->is_data_node());
		bool lhs_nested = lhs->is_nested ();
		bool rhs_simple = (rhs->is_id_node() || rhs->is_shiftvec_node() || rhs->is_data_node());
		bool rhs_nested = rhs->is_nested ();
		// If lhs is simple, and rhs is simple or (), then check for similary of lhs
		if (lhs_simple && (rhs_simple || rhs_nested)) {
			inserted = lhs->identify_same_accesses (same_accesses, rhs);
		}
		// If rhs is simple, and lhs is simple or (), then check for similary of rhs
		if (!inserted && rhs_simple && (lhs_simple || lhs_nested)) {
			inserted = rhs->identify_same_accesses (same_accesses, lhs);
		}
	}
	return inserted;
}

bool binaryNode::consolidate_same_accesses (vector<tuple<OP_TYPE,exprNode*>> &consolidated_exprs, map<exprNode*,vector<exprNode*>> &same_accesses, map<exprNode*,exprNode*> &cmap, exprNode* access, bool flip) {
	bool inserted= false;
	if (op == T_MINUS || op == T_PLUS) {
		if (!lhs->is_nested() && lhs->is_binary_node()) {
			inserted |= lhs->consolidate_same_accesses (consolidated_exprs, same_accesses, cmap, NULL, false);
		}
		else { 
			OP_TYPE lhs_op = flip ? T_MINUS : T_PLUS; 
			consolidated_exprs.push_back (make_tuple (lhs_op, lhs));
		}
		if (!rhs->is_nested() && rhs->is_binary_node()) {
			inserted |= rhs->consolidate_same_accesses (consolidated_exprs, same_accesses, cmap, NULL, op==T_MINUS);
		}
		else { 
			consolidated_exprs.push_back (make_tuple (op, rhs));
		}
	}
	if (op == T_MULT) {
		bool lhs_simple = (lhs->is_id_node() || lhs->is_shiftvec_node() || lhs->is_data_node());
		bool lhs_nested = lhs->is_nested ();
		bool rhs_simple = (rhs->is_id_node() || rhs->is_shiftvec_node() || rhs->is_data_node());
		bool rhs_nested = rhs->is_nested ();
		// If lhs is simple, and rhs is simple or (), then check for similary of lhs
		if (lhs_simple && (rhs_simple || rhs_nested)) {
			inserted = lhs->consolidate_same_accesses (consolidated_exprs, same_accesses, cmap, rhs, flip);
		}
		// If rhs is simple, and lhs is simple or (), then check for similary of rhs
		if (!inserted && rhs_simple && (lhs_simple || lhs_nested)) {
			inserted = rhs->consolidate_same_accesses (consolidated_exprs, same_accesses, cmap, lhs, flip);
		}
		if (!inserted) {
			OP_TYPE new_op = flip ? T_MINUS : T_PLUS; 
			consolidated_exprs.push_back (make_tuple (new_op, this));	
		}
	}
	return inserted;
}

void binaryNode::decompose_node (exprNode* stmt_lhs, vector<tuple<exprNode*,STMT_OP,exprNode*>> &substmts, bool flip) {
	//if (DEBUG) {
	//	stringstream npf;
	//	npf << "Currently, visiting node with lhs ";
	//	lhs->print_node (npf);
	//	npf << " and rhs ";
	//	rhs->print_node (npf);
	//	npf << "\n";
	//	cout << npf.str ();
	//}
	// The LHS gets the sign of flip
	if (op == T_MINUS || op == T_PLUS) {
		vector<tuple<exprNode*,STMT_OP,exprNode*>> lhs_substmts, rhs_substmts;
		bool lhs_flip = flip; 
		lhs->decompose_node (stmt_lhs, lhs_substmts, lhs_flip);
		if (lhs_substmts.empty ()) {
			substmts.push_back (make_tuple (stmt_lhs, get_acc_op (T_PLUS, lhs_flip), lhs));	
		}
		else 
			substmts.insert (substmts.end(), lhs_substmts.begin(), lhs_substmts.end());

		bool rhs_flip = (op==T_MINUS && rhs->is_nested()) ? !flip : flip;	
		rhs->decompose_node (stmt_lhs, rhs_substmts, rhs_flip);
		if (rhs_substmts.empty ()) {
			substmts.push_back (make_tuple (stmt_lhs, get_acc_op (op, rhs_flip), rhs));	
		}
		else { 
			substmts.insert (substmts.end(), rhs_substmts.begin(), rhs_substmts.end());
		}
	}
}

// chain of division/multiplication
void binaryNode::nonassoc_chain (bool &ret) {
	if (!(op == T_MULT || op == T_DIV))
		ret = false;
	if (ret) {
		lhs->nonassoc_chain (ret);
		rhs->nonassoc_chain (ret);
	}
}

// chain of addition/subtraction
void binaryNode::assoc_chain (bool &ret) {
	if (!(op == T_PLUS || op == T_MINUS))
		ret = false;
}

bool binaryNode::same_operator (OP_TYPE p) {
	bool ret = (op == p);
	if (ret) {
		ret &= (lhs->is_nested() || lhs->same_operator(op));
		ret &= (rhs->is_nested() || rhs->same_operator(op));
	}
	return ret;	
}

void binaryNode::remove_redundant_nesting (void) {
	//If my operator is +,/,*, and the lhs/rhs have only operators the
	//same operators, remove the nesting from lhs/rhs
	lhs->remove_redundant_nesting();
	rhs->remove_redundant_nesting(); 
	if (op==T_MULT || op==T_PLUS || op==T_DIV) {
		stringstream temp_lhs, temp_rhs;
		lhs->print_node (temp_lhs);
		rhs->print_node (temp_rhs);
		if (lhs->is_nested() && lhs->same_operator(op)) {
			lhs->set_nested (false);
		}
		if (rhs->is_nested() && rhs->same_operator(op)) {
			rhs->set_nested (false);
		}
	}
}

exprNode *binaryNode::distribute_node (void) {
	// LHS must be a chain of multiplication/division,
	bool lhs_nonassoc_chain = true;
	lhs->nonassoc_chain (lhs_nonassoc_chain);
	stringstream temp_lhs, temp_rhs;
	lhs->print_node (temp_lhs);
	rhs->print_node (temp_rhs);
	// RHS must be nested in (), and a chain of addition/subtraction
	bool rhs_assoc_chain = (rhs->is_nested()) && (rhs->get_expr_type()==T_BINARY);
	rhs->assoc_chain (rhs_assoc_chain);
	if (DEBUG) {
		cout << "lhs = " << temp_lhs.str () << "\nrhs = " << temp_rhs.str () << endl;
		cout <<" initial rhs assoc = " << rhs_assoc_chain << ", rhs nested = " << rhs->is_nested();
		cout << ", lhs nonassoc = " << lhs_nonassoc_chain << ", rhs_assoc = " << rhs_assoc_chain << endl;
	}
	if (lhs_nonassoc_chain && rhs_assoc_chain && (op==T_MULT)) {
		// Distribute lhs over the binary rhs 
		exprNode *t_lhs = dynamic_cast<binaryNode*>(rhs)->get_lhs ();
		exprNode *t_rhs = dynamic_cast<binaryNode*>(rhs)->get_rhs ();
		if (t_lhs->get_expr_type()==T_BINARY)
			t_lhs->set_nested ();
		if (t_rhs->get_expr_type()==T_BINARY)
			t_rhs->set_nested ();
		OP_TYPE new_op = dynamic_cast<binaryNode*>(rhs)->get_operator ();
		exprNode *new_lhs = new binaryNode (op, lhs, t_lhs, type, false);
		exprNode *new_rhs = new binaryNode (op, lhs, t_rhs, type, false);
		return new binaryNode (new_op, new_lhs->distribute_node(), new_rhs->distribute_node(), type, rhs->is_nested());
	}
	else if (op == T_PLUS || op == T_MINUS) {
		binaryNode *new_node = new binaryNode (*this);
		new_node->lhs = (new_node->lhs)->distribute_node ();
		new_node->rhs = (new_node->rhs)->distribute_node ();
		return new_node;	
	}
	return this;
}

void binaryNode::infer_expr_type (DATA_TYPE &ret) {
	lhs->infer_expr_type (ret);
	rhs->infer_expr_type (ret);
}

void binaryNode::print_node (stringstream &out) {
	if (nested)
		out << "(";
	lhs->print_node (out);
	out << print_operator (op);
	rhs->print_node (out);
	if (nested)
		out << ")";
}

void binaryNode::print_node (stringstream &out, map<tuple<string,int,bool>,RESOURCE> rmap, bool is_lhs, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays, map<string,int> unroll_instance, bool blocked_loads, map<string,int> udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, string &cstream_shift) {
	if (nested)
		out << "(";
	lhs->print_node (out, rmap, is_lhs, full_stream, stream, stream_dim, temp_arrays, unroll_instance, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, cstream_shift);
	out << print_operator (op);
	rhs->print_node (out, rmap, is_lhs, full_stream, stream, stream_dim, temp_arrays, unroll_instance, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, cstream_shift);
	if (nested)
		out << ")";
}

void binaryNode::print_node (stringstream &out, map<tuple<string,int,bool>,RESOURCE> rmap, bool is_lhs, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays, bool blocked_loads, map<string,int> udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, map<string,tuple<int,int>> halo_dims, int reg_count, string &cstream_shift) {
	if (nested)
		out << "(";
	lhs->print_node (out, rmap, is_lhs, full_stream, stream, stream_dim, temp_arrays, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, halo_dims, reg_count, cstream_shift);
	out << print_operator (op);
	rhs->print_node (out, rmap, is_lhs, full_stream, stream, stream_dim, temp_arrays, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, halo_dims, reg_count, cstream_shift);
	if (nested)
		out << ")";
}

void binaryNode::print_last_write (stringstream &out, map<tuple<string,int,bool>,RESOURCE> rmap, map<string,exprNode*> last_writes, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays, map<string,int> unroll_instance, bool blocked_loads, map<string,int> udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, string &cstream_shift) {
	lhs->print_last_write (out, rmap, last_writes, full_stream, stream, stream_dim, temp_arrays, unroll_instance, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, cstream_shift);
	rhs->print_last_write (out, rmap, last_writes, full_stream, stream, stream_dim, temp_arrays, unroll_instance, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, cstream_shift);
}

void binaryNode::print_last_write (stringstream &out, map<tuple<string,int,bool>,RESOURCE> rmap, map<string,exprNode*> last_writes, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays, bool blocked_loads, map<string,int> udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, map<string,tuple<int,int>> halo_dims, int reg_count, string &cstream_shift) {
	lhs->print_last_write (out, rmap, last_writes, full_stream, stream, stream_dim, temp_arrays, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, halo_dims, reg_count, cstream_shift);
	rhs->print_last_write (out, rmap, last_writes, full_stream, stream, stream_dim, temp_arrays, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, halo_dims, reg_count, cstream_shift);
}

void binaryNode::determine_stmt_resource_mapping (map<tuple<string,int,bool>,RESOURCE> &rmap, vector<string> iters, bool stream, string stream_dim, bool use_shmem) {
	lhs->determine_stmt_resource_mapping (rmap, iters, stream, stream_dim, use_shmem);
	rhs->determine_stmt_resource_mapping (rmap, iters, stream, stream_dim, use_shmem);
}

int binaryNode::compute_dimensionality (vector<string> iters) {
	int lhs_dim = lhs->compute_dimensionality (iters);
	int rhs_dim = rhs->compute_dimensionality (iters);
	return max (lhs_dim, rhs_dim);	
}

void binaryNode::decompose_access_fsm (string &id, int &offset) {
	string l_id = "", r_id = "";
	int l_val = 0, r_val = 0;
	lhs->decompose_access_fsm (l_id, l_val);
	rhs->decompose_access_fsm (r_id, r_val);
	if (op == T_DIV) {
		if (DEBUG) assert ((l_id.length () == 0 || l_val == 0) && "Complicated div unhandled (decompose_access_fsm)");
		if (DEBUG) assert ((r_id.length () == 0 || r_val == 0) && "Complicated div unhandled (decompose_access_fsm)");
		if (l_id.length () == 0 && r_id.length () == 0) {
			if (l_val % r_val == 0) 
				offset += l_val / r_val;
			else
				id = id + to_string(l_val) + "/" + to_string (r_val);
		}
		else if (l_id.length () != 0) {
			if (r_id.length () == 0)
				id = id + l_id + "/" + to_string (r_val);
			else
				id = id + l_id + "/" + r_id;
		}
		else if (r_id.length () != 0)
			id = id + to_string (l_val) + "/" + r_id;
	}
	else if (op == T_MULT) {
		// Arrange IDs on left, and val on right.
		if (DEBUG) assert ((l_id.length () == 0 || l_val == 0) && "Complicated mult unhandled (decompose_access_fsm)");
		if (DEBUG) assert ((r_id.length () == 0 || r_val == 0) && "Complicated mult unhandled (decompose_access_fsm)");
		if (l_id.length () == 0 && r_id.length () == 0)
			offset += l_val * r_val;
		else if (l_id.length () != 0) {
			if (r_id.length () == 0)
				id = id + l_id + "*" + to_string (r_val);
			else {
				string first = (l_id.compare (r_id) <= 0) ? l_id : r_id;
				string second = (l_id.compare (r_id) > 0) ? l_id : r_id;
				id = id + first + "*" + second;
			}
		}
		else if (r_id.length () != 0)
			id = id + r_id + "*" + to_string (l_val);
	}
	else if (op == T_PLUS) {
		if (l_id.length () > 0 && r_id.length () > 0) {
			string first = (l_id.compare (r_id) <= 0) ? l_id : r_id;
			string second = (l_id.compare (r_id) > 0) ? l_id : r_id;
			id = id + first + "+" + second;
		}
		else if (l_id.length () > 0)
			id = id + l_id;
		else if (r_id.length () > 0)
			id = id + r_id;
		offset += l_val + r_val;
	}
	else if (op == T_MINUS) {
		if (l_id.length () > 0 && r_id.length () > 0) {
			r_id = "-" + r_id;
			string first = (l_id.compare (r_id) <= 0) ? l_id : r_id;
			string second = (l_id.compare (r_id) > 0) ? l_id : r_id;
			id = id + first + "-" + second;
		}
		else if (l_id.length () > 0)
			id = id + l_id;
		else if (r_id.length () > 0)
			id = id + "-" + r_id;
		offset += l_val - r_val;
	}
}

void binaryNode::set_data_type (map<string, DATA_TYPE> data_types) {
	lhs->set_data_type (data_types);
	rhs->set_data_type (data_types);
}

void binaryNode::collect_accesses (set<string> &accesses) {
	lhs->collect_accesses (accesses);
	rhs->collect_accesses (accesses);
}

bool binaryNode::same_expr (exprNode *node) {
	bool same_expr = type == node->get_type ();
	same_expr &= expr_type == node->get_expr_type ();
	if (same_expr) {
		same_expr &= op == dynamic_cast<binaryNode*>(node)->get_operator (); 
		same_expr &= lhs->same_expr (dynamic_cast<binaryNode*>(node)->get_lhs ());
		same_expr &= rhs->same_expr (dynamic_cast<binaryNode*>(node)->get_rhs ());
	}
	return same_expr;		
}

bool binaryNode::waw_dependence (exprNode *node) {
	bool waw_dependence = lhs->waw_dependence (node);
	waw_dependence |= rhs->waw_dependence (node);
	return waw_dependence;		
}

bool binaryNode::raw_dependence (exprNode *node) {
	bool raw_dependence = lhs->raw_dependence (node);
	raw_dependence |= rhs->raw_dependence (node);
	return raw_dependence;		
}

bool binaryNode::war_dependence (exprNode *node) {
	bool war_dependence = lhs->war_dependence (node);
	war_dependence |= rhs->war_dependence (node);
	return war_dependence;		
}

bool binaryNode::pointwise_occurrence (exprNode *node) {
	bool pointwise_occurrence = lhs->pointwise_occurrence (node);
	pointwise_occurrence &= rhs->pointwise_occurrence (node);
	return pointwise_occurrence;		
}

void binaryNode::compute_rbw (set<string> &reads, set<string> &writes, bool is_read) {
	lhs->compute_rbw (reads, writes, is_read);
	rhs->compute_rbw (reads, writes, is_read);
}

void binaryNode::compute_wbr (set<string> &writes, set<string> &reads, bool is_write) {
	lhs->compute_wbr (writes, reads, is_write);
	rhs->compute_wbr (writes, reads, is_write);
}

bool binaryNode::verify_immutable_expr (vector<string> writes, bool start_verification) {
	bool result = true;
	result &= lhs->verify_immutable_expr (writes, start_verification);
	result &= rhs->verify_immutable_expr (writes, start_verification);
	return result; 
}

int binaryNode::maximum_streaming_offset (exprNode *src_lhs, string stream_dim) {
	int lhs_result = lhs->maximum_streaming_offset (src_lhs, stream_dim);
	int rhs_result = rhs->maximum_streaming_offset (src_lhs, stream_dim);	
	return max (lhs_result, rhs_result);
}

int binaryNode::minimum_streaming_offset (string stream_dim) {
	int lhs_result = lhs->minimum_streaming_offset (stream_dim);
	int rhs_result = rhs->minimum_streaming_offset (stream_dim);	
	return min (lhs_result, rhs_result);
}

void binaryNode::offset_expr (int offset, string stream_dim) {
	lhs->offset_expr (offset, stream_dim);
	rhs->offset_expr (offset, stream_dim);	
}

void binaryNode::compute_hull (map<string, vector<Range*>> &hull_map, vector<Range*> &hull, vector<Range*> &initial_hull, bool stream, string stream_dim, int offset) {
	lhs->compute_hull (hull_map, hull, initial_hull, stream, stream_dim, offset);
	rhs->compute_hull (hull_map, hull, initial_hull, stream, stream_dim, offset);
}

void binaryNode::compute_hull (map<string, vector<Range*>> &hull_map, vector<Range*> &hull, vector<Range*> &initial_hull, map<tuple<string,int,bool>,RESOURCE> rmap, bool stream, string stream_dim, int offset) {
	lhs->compute_hull (hull_map, hull, initial_hull, rmap, stream, stream_dim, offset);
	rhs->compute_hull (hull_map, hull, initial_hull, rmap, stream, stream_dim, offset);
}

void binaryNode::shift_hull (vector<Range*> &hull) {
	lhs->shift_hull (hull);
	rhs->shift_hull (hull);
}

void binaryNode::shift_hull (vector<Range*> &hull, bool stream, string stream_dim) {
	lhs->shift_hull (hull, stream, stream_dim);
	rhs->shift_hull (hull, stream, stream_dim);
}

void binaryNode::collect_access_stats (map<tuple<string,int,bool>,tuple<int,int,int>> &access_map, bool is_read, map<string,int> blockdims, vector<string> iters, bool stream, string stream_dim) {
	lhs->collect_access_stats (access_map, is_read, blockdims, iters, stream, stream_dim); 
	rhs->collect_access_stats (access_map, is_read, blockdims, iters, stream, stream_dim);
}

void binaryNode::collect_access_stats (map<tuple<string,int,bool>, tuple<int,int,map<int,vector<string>>>> &access_map, bool is_read, vector<string> iters, bool stream, string stream_dim) {
	lhs->collect_access_stats (access_map, is_read, iters, stream, stream_dim);
	rhs->collect_access_stats (access_map, is_read, iters, stream, stream_dim);
}

exprNode *binaryNode::unroll_expr (string iter, int unroll_instance, map<string,int> &scalar_version_map, bool is_lhs) {
	exprNode *temp1 = lhs->unroll_expr (iter, unroll_instance, scalar_version_map, is_lhs);
	exprNode *temp2 = rhs->unroll_expr (iter, unroll_instance, scalar_version_map, is_lhs);
	return new binaryNode (op, temp1, temp2, type, nested);
}

exprNode *shiftvecNode::retime_node (string stream_dim, int offset) {
	shiftvecNode *ret = new shiftvecNode (*this);
	int pos = 0;
	for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,pos++) {
		string id = "";
		int val = 0;
		(*it)->decompose_access_fsm (id, val);
		if (!id.empty () & (id.compare (stream_dim) == 0)) {
			// Start retiming
			OP_TYPE op = (offset > 0) ? T_PLUS : T_MINUS;
			exprNode *temp = new binaryNode (op, *it, new datatypeNode<int>(abs(offset), INT));
			ret->set_index (temp, pos);
			break;	
		}
	}
	return ret;
}

int shiftvecNode::get_retiming_offset (string stream_dim) {
	int ret = INT_MIN;
	for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++) {
		string id = "";
		int val = 0;
		(*it)->decompose_access_fsm (id, val);
		if (!id.empty () & (id.compare (stream_dim) == 0)) {
			ret = val;
			break;
		}
	}
	return ret;
}


bool shiftvecNode::retiming_feasible (string stream_dim, vector<int> &offset) {
	bool ret = false;
	for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++) {
		string id = "";
		int val = 0;
		(*it)->decompose_access_fsm (id, val);
		if (!id.empty () & (id.compare (stream_dim) == 0)) {
			if (offset.empty ()) {
				offset.push_back (val);
				ret = true;
			}
			else { 
				ret = (val == offset.front());
			}
			break;
		}
	}
	return ret;
}

bool shiftvecNode::identify_same_accesses (map<exprNode*,vector<exprNode*>> &same_accesses, exprNode *access) {
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

bool shiftvecNode::consolidate_same_accesses (vector<tuple<OP_TYPE,exprNode*>> &consolidated_exprs, map<exprNode*,vector<exprNode*>> &same_accesses, map<exprNode*,exprNode*> &cmap, exprNode* access, bool flip) {
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

void shiftvecNode::collect_accesses (set<string> &accesses) {
	accesses.insert (name);
}

void shiftvecNode::print_node (stringstream &out) {
	if (nested)
		out << "(";
	out << print_array ();
	if (nested)
		out << ")";
}

void shiftvecNode::print_node (stringstream &out, map<tuple<string,int,bool>,RESOURCE> rmap, bool is_lhs, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays, map<string,int> unroll_instance, bool blocked_loads, map<string,int> udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, string &cstream_shift) {
	if (nested)
		out << "(";
	out << print_array (rmap, is_lhs, full_stream, stream, stream_dim, temp_arrays, unroll_instance, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, cstream_shift);
	if (nested)
		out << ")";	
}

void shiftvecNode::print_node (stringstream &out, map<tuple<string,int,bool>,RESOURCE> rmap, bool is_lhs, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays, bool blocked_loads, map<string,int> udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, map<string,tuple<int,int>> halo_dims, int reg_count, string &cstream_shift) {
	if (nested)
		out << "(";
	out << print_array (rmap, is_lhs, full_stream, stream, stream_dim, temp_arrays, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, halo_dims, reg_count, cstream_shift);
	if (nested)
		out << ")";	
}

void shiftvecNode::print_last_write (stringstream &out, map<tuple<string,int,bool>,RESOURCE> rmap, map<string,exprNode*> last_writes, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays, map<string,int> unroll_instance, bool blocked_loads, map<string,int> udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, string &cstream_shift) {
	RESOURCE res = GLOBAL_MEM;
	bool resource_found = false;
	bool found_streaming_iterator = false;
	int streaming_pos = 0, offset = 0;
	tuple<string,int,bool> access;
	string access_name = name;
	// Change the name according to the resource map
	if (stream) {
		int pos = 0;
		for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,pos++) {
			string id = "";
			int val = 0;
			(*it)->decompose_access_fsm (id, val);
			if (!id.empty () & (id.compare (stream_dim) == 0)) {
				found_streaming_iterator = true;
				streaming_pos = pos;
			}
			if (id.empty ()) {
				access_name = access_name + "@" + to_string(val) + "@";
			}
		}
	}
	if (found_streaming_iterator) {
		exprNode *it = indices[streaming_pos];
		string id = "";
		it->decompose_access_fsm (id, offset);
		assert (stream_dim.compare (id) == 0 && "streaming dimension not found (print_array)");
		access = make_tuple (access_name, offset, stream);
	}
	else
		access = make_tuple (access_name, offset, false);
	if (rmap.find (access) != rmap.end ()) {
		res = rmap[access];
		resource_found = true;
	}
	// Check if this is the last write
	if (resource_found && (res != GLOBAL_MEM) && (last_writes.find (access_name) != last_writes.end ())) {
		if (last_writes[access_name] == this) {
			out << print_array (temp_arrays, full_stream, stream, stream_dim, unroll_instance, blocked_loads, params, iters, linearize_accesses, code_diff, cstream_shift);
			out << " = ";
			out << print_array (rmap, false, full_stream, stream, stream_dim, temp_arrays, unroll_instance, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, cstream_shift);
			out << ";\n";
		}
	}
}

void shiftvecNode::print_last_write (stringstream &out, map<tuple<string,int,bool>,RESOURCE> rmap, map<string,exprNode*> last_writes, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays,bool blocked_loads, map<string,int> udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, map<string,tuple<int,int>> halo_dims, int reg_count, string &cstream_shift) {
	RESOURCE res = GLOBAL_MEM;
	bool resource_found = false;
	bool found_streaming_iterator = false;
	int streaming_pos = 0, offset = 0;
	tuple<string,int,bool> access;
	string access_name = name;
	// Change the name according to the resource map
	if (stream) {
		int pos = 0;
		for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,pos++) {
			string id = "";
			int val = 0;
			(*it)->decompose_access_fsm (id, val);
			if (!id.empty () & (id.compare (stream_dim) == 0)) {
				found_streaming_iterator = true;
				streaming_pos = pos;
			}
			if (id.empty ()) {
				access_name = access_name + "@" + to_string(val) + "@";
			}
		}
	}
	if (found_streaming_iterator) {
		exprNode *it = indices[streaming_pos];
		string id = "";
		it->decompose_access_fsm (id, offset);
		assert (stream_dim.compare (id) == 0 && "streaming dimension not found (print_array)");
		access = make_tuple (access_name, offset, stream);
	}
	else
		access = make_tuple (access_name, offset, false);
	if (rmap.find (access) != rmap.end ()) {
		res = rmap[access];
		resource_found = true;
	}

	// Check if this is the last write
	if (resource_found && (res != GLOBAL_MEM) && (last_writes.find (access_name) != last_writes.end ())) {
		if (last_writes[access_name] == this) {
			out << print_array (temp_arrays, full_stream, stream, stream_dim, udecls, blocked_loads, params, iters, linearize_accesses, code_diff, halo_dims, reg_count, cstream_shift);
			out << " = ";
			out << print_array (rmap, false, full_stream, stream, stream_dim, temp_arrays, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, halo_dims, reg_count, cstream_shift);
			out << ";\n";
		}
	}
}

string shiftvecNode::print_array (void) { 
	stringstream out; 
	out << name; 
	for (auto ind : indices) {
		string id = ""; 
		int offset = 0; 
		ind->decompose_access_fsm (id, offset);
		out << "["; 
		if (id.empty ())
			out << offset;       
		else { 
			out << id; 
			if (offset > 0) 
				out << "+"; 
			if (offset != 0) 
				out << offset; 
		}
		out << "]";
	}
	return out.str ();
}

string shiftvecNode::print_array (vector<string> temp_arrays, bool full_stream, bool stream, string stream_dim, map<string,int> unroll_instance, bool blocked_loads, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, string &cstream_shift) { 
	stringstream out; 
	out << name;
	bool temp_array = find (temp_arrays.begin(), temp_arrays.end(), name) != temp_arrays.end();
	vector<string> blockdims = (iters.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iters.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));
	for (auto ind : indices) {
		string id = ""; 
		int offset = 0; 
		ind->decompose_access_fsm (id, offset);
		if (!id.empty() && stream && (stream_dim.compare (id) == 0) && temp_array)
			continue;
		out << "["; 
		if (id.empty ())
			out << offset;       
		else {
			if (!full_stream && stream && (stream_dim.compare (id) == 0)) {
				out << "max (" << cstream_shift << ", ";
			}
			if (stream && (stream_dim.compare (id) == 0))
				out << "max (0, ";
			out << id;
			if (temp_array || code_diff) {
				out << "-" << id << "0";
			}
			if (blocked_loads) {
				if (unroll_instance.find (id) != unroll_instance.end ())
					offset += unroll_instance[id];	 
			}
			else {
				if ((unroll_instance.find (id) != unroll_instance.end ()) && (unroll_instance[id]>0)) {
					for (vector<string>::iterator it=iters.begin(); it!=iters.end(); it++) {
						if (id.compare (*it) == 0) {
							out << "+";
							if (unroll_instance[id] > 1)
								out << unroll_instance[id] << "*";
							out << "blockDim." << blockdims[it-iters.begin()];
						}
					}
				}
			}
			if (offset > 0) 
				out << "+"; 
			if (offset != 0) 
				out << offset;
			if (stream && (stream_dim.compare (id) == 0))
				out << ")";
			if (!full_stream && stream && (stream_dim.compare (id) == 0)) {
				out << ")";
			} 
		}
		out << "]";
	}
	return out.str ();
}

string shiftvecNode::print_array (vector<string> temp_arrays, bool full_stream, bool stream, string stream_dim, map<string,int> unroll_instance, bool blocked_loads, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, map<string,tuple<int,int>> halo_dims, int reg_count, string &cstream_shift) {
	stringstream out; 
	out << name;
	bool temp_array = find (temp_arrays.begin(), temp_arrays.end(), name) != temp_arrays.end();
	vector<string> blockdims = (iters.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iters.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));
	for (auto ind : indices) {
		string id = ""; 
		int offset = 0; 
		ind->decompose_access_fsm (id, offset);
		if (!id.empty() && stream && (stream_dim.compare (id) == 0) && temp_array)
			continue;
		out << "["; 
		if (id.empty ())
			out << offset;       
		else {
			if (!full_stream && stream && (stream_dim.compare (id) == 0)) {
				out << "max (" << cstream_shift << ", ";
			}
			if (stream && (stream_dim.compare (id) == 0))
				out << "max (0, ";
			out << id;
			if (temp_array || code_diff) {
				out << "-" << id << "0";
			}
			int ufactor = (unroll_instance.find (id) != unroll_instance.end ()) ? unroll_instance[id] : 1;
			//tuple<int,int> halo = (halo_dims.find (id) != halo_dims.end ()) ? halo_dims[id] : make_tuple (0, 0);
			if (ufactor > 1) // || (get<0>(halo) != 0 || get<1>(halo) != 0))
				out << ufactor;

			if (offset > 0) 
				out << "+"; 
			if (offset != 0) 
				out << offset;
			if (stream && (stream_dim.compare (id) == 0)) 
				out << ")";
			if (!full_stream && stream && (stream_dim.compare (id) == 0)) {
				out << ")";
			}
		}
		out << "]";
	}
	return out.str ();
}

string shiftvecNode::print_array (map<tuple<string,int,bool>,RESOURCE> rmap, bool is_lhs, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays, map<string,int> unroll_instance, bool blocked_loads, map<string,int>udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, string &cstream_shift) {
	stringstream out;
	vector<string> blockdims = (iters.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iters.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));
	RESOURCE res = GLOBAL_MEM;
	bool resource_found = false;
	bool found_streaming_iterator = false;
	int streaming_pos = 0, offset = 0;
	tuple<string,int,bool> access;
	string access_name = name;
	bool temp_array = find (temp_arrays.begin(), temp_arrays.end(), name) != temp_arrays.end();
	// Change the name according to the resource map
	if (stream) {
		int pos = 0;
		for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,pos++) {
			string id = "";
			int val = 0;
			(*it)->decompose_access_fsm (id, val);
			if (!id.empty () & (id.compare (stream_dim) == 0)) {
				found_streaming_iterator = true;
				streaming_pos = pos;
			}
			if (id.empty ()) {
				access_name = access_name + "@" + to_string(val) + "@";
			}
		}
	}
	if (found_streaming_iterator) {
		exprNode *it = indices[streaming_pos];
		string id = "";
		it->decompose_access_fsm (id, offset);
		assert (stream_dim.compare (id) == 0 && "streaming dimension not found (print_array)");
		access = make_tuple (access_name, offset, stream);
	}
	else 
		access = make_tuple (access_name, offset, false);
	if (rmap.find (access) != rmap.end ()) { 
		res = rmap[access];
		resource_found = true;
	}
	out << print_trimmed_string (access_name, '@');
	out << print_resource (res, offset, get<2>(access));
	//cout << "printing resource for " << print_trimmed_string (access_name, '@') << ", offset = " << offset << ", stream = " << stream << endl;

	// Print the indices, but leave out the streaming dimension if streaming was enabled
	int pos = 0;
	for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,pos++) {
		if (!(found_streaming_iterator && pos == streaming_pos && resource_found && (res!=GLOBAL_MEM || temp_array))) {
			string id = ""; 
			int offset = 0; 
			(*it)->decompose_access_fsm (id, offset);
			if (id.empty () && res != GLOBAL_MEM)
				continue;
			if (res != REGISTER) {
				out << "["; 
				if (id.empty ())
					out << offset;       
				else {
					if (is_lhs && (res == GLOBAL_MEM) && !full_stream && stream && (stream_dim.compare (id) == 0)) {
						out << "max (" << cstream_shift << ", ";
					}
					if (is_lhs && (res == GLOBAL_MEM) && stream && (stream_dim.compare (id) == 0))
						out << "max (0, ";
					out << id;
					if (res == SHARED_MEM || (res == GLOBAL_MEM && (code_diff || temp_array))) {
						out << "-" << id << "0";
					}
					if (blocked_loads) {
						if (unroll_instance.find (id) != unroll_instance.end ())
							offset += unroll_instance[id]; 
					}
					else {
						if ((unroll_instance.find (id) != unroll_instance.end ()) && (unroll_instance[id]>0)) {
							for (vector<string>::iterator it=iters.begin(); it!=iters.end(); it++) {
								if (id.compare (*it) == 0) {
									out << "+"; 
									if (unroll_instance[id] > 1) 
										out << unroll_instance[id] << "*";
									out << "blockDim." << blockdims[it-iters.begin()];
								}
							}
						}
					}
					if (offset > 0) 
						out << "+"; 
					if (offset != 0) 
						out << offset;
					if (is_lhs && (res == GLOBAL_MEM) && stream && (stream_dim.compare (id) == 0))
						out << ")";
					if (is_lhs && (res == GLOBAL_MEM) && !full_stream && stream && (stream_dim.compare (id) == 0)) {
						out << ")";
					} 
				}
				out << "]";
			}
			else if (res == REGISTER) {
				if (!id.empty () && (unroll_instance.find (id) != unroll_instance.end ())) {
					assert (udecls.find (id) != udecls.end ());
					if (udecls[id] > 1)	
						out << "[" << unroll_instance[id] << "]"; 
				}
			}
		}
	}
	return out.str ();
}

string shiftvecNode::print_array (map<tuple<string,int,bool>,RESOURCE> rmap, bool is_lhs, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays, bool blocked_loads, map<string,int>udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, map<string,tuple<int,int>> halo_dims, int reg_count, string &cstream_shift) {
	stringstream out;
	vector<string> blockdims = (iters.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iters.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));
	RESOURCE res = GLOBAL_MEM;
	bool resource_found = false;
	bool found_streaming_iterator = false;
	int streaming_pos = 0, offset = 0;
	tuple<string,int,bool> access;
	string access_name = name;
	bool temp_array = find (temp_arrays.begin(), temp_arrays.end(), name) != temp_arrays.end();
	// Change the name according to the resource map
	if (stream) {
		int pos = 0;
		for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,pos++) {
			string id = "";
			int val = 0;
			(*it)->decompose_access_fsm (id, val);
			if (!id.empty () & (id.compare (stream_dim) == 0)) {
				found_streaming_iterator = true;
				streaming_pos = pos;
			}
			if (id.empty ()) {
				access_name = access_name + "@" + to_string(val) + "@";
			}
		}
	}
	if (found_streaming_iterator) {
		exprNode *it = indices[streaming_pos];
		string id = "";
		it->decompose_access_fsm (id, offset);
		assert (stream_dim.compare (id) == 0 && "streaming dimension not found (print_array)");
		access = make_tuple (access_name, offset, stream);
	}
	else 
		access = make_tuple (access_name, offset, false);
	if (rmap.find (access) != rmap.end ()) { 
		res = rmap[access];
		resource_found = true;
	}
	out << print_trimmed_string (access_name, '@');
	out << print_resource (res, offset, get<2>(access));
	//cout << "printing resource for " << print_trimmed_string (access_name, '@') << ", offset = " << offset << ", stream = " << stream << endl;

	// Print the indices, but leave out the streaming dimension if streaming was enabled
	int pos = 0;
	for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,pos++) {
		if (!(found_streaming_iterator && pos == streaming_pos && resource_found && (res!=GLOBAL_MEM || temp_array))) {
			string id = ""; 
			int offset = 0; 
			(*it)->decompose_access_fsm (id, offset);
			if (id.empty () && res != GLOBAL_MEM)
				continue;
			if (res != REGISTER) {
				out << "["; 
				if (id.empty ())
					out << offset;       
				else {
					if (is_lhs && (res == GLOBAL_MEM) && !full_stream && stream && (stream_dim.compare (id) == 0)) {
						out << "max (" << cstream_shift << ", ";
					}  
					if (is_lhs && (res == GLOBAL_MEM) && stream && (stream_dim.compare (id) == 0)) 
						out << "max (0, ";
					out << id;
					int ufactor = (udecls.find (id) != udecls.end ()) ? udecls[id] : 1;
					//tuple<int,int> halo = (halo_dims.find (id) != halo_dims.end ()) ? halo_dims[id] : make_tuple (0, 0);
					if (ufactor > 1) // || (get<0>(halo) != 0 || get<1>(halo) != 0))
						out << ufactor;
					if (res == SHARED_MEM || (res == GLOBAL_MEM && (code_diff || temp_array))) {
						out << "-" << id << "0";
					}
					if (offset > 0) 
						out << "+"; 
					if (offset != 0) 
						out << offset;
					if (is_lhs && (res == GLOBAL_MEM) && stream && (stream_dim.compare (id) == 0))
						out << ")";
					if (is_lhs && (res == GLOBAL_MEM) && !full_stream && stream && (stream_dim.compare (id) == 0)) {
						out << ")";
					} 
				}
				out << "]";
			}
			else if (res == REGISTER) {
				if (!id.empty () && (udecls.find (id) != udecls.end ())) {
					assert (udecls.find (id) != udecls.end ());
					int ufactor = (udecls.find (id) != udecls.end ()) ? udecls[id] : 1;
					//tuple<int,int> halo = (halo_dims.find (id) != halo_dims.end ()) ? halo_dims[id] : make_tuple (0, 0);
					if (ufactor > 1)// || (get<0>(halo) != 0 || get<1>(halo) != 0))
						out << "[r" << reg_count << "_" << id << udecls[id] << "]"; 
				}
			}
		}
	}
	return out.str ();
}

void shiftvecNode::set_data_type (map<string, DATA_TYPE> data_types) {
	assert (data_types.find (name) != data_types.end ());
	type = data_types[name];
}

bool shiftvecNode::same_expr (exprNode *node) {
	bool same_expr = type == node->get_type ();
	same_expr &= expr_type == node->get_expr_type ();
	if (!same_expr)
		return same_expr;
	same_expr &= name.compare (node->get_name ()) == 0;
	if (!same_expr)
		return same_expr;
	// All the offsets must be similar
	vector<exprNode*> t_indices = dynamic_cast<shiftvecNode*>(node)->get_indices ();
	same_expr &= indices.size () == t_indices.size ();
	if (same_expr) {
		vector<exprNode*>:: iterator jt=t_indices.begin();
		for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,jt++) {
			bool temp_same_expr = (*it)->same_expr (*jt);
			if (!temp_same_expr) {
				string it_id = "", jt_id = "";
				int it_val = 0, jt_val = 0;
				(*it)->decompose_access_fsm (it_id, it_val);
				(*jt)->decompose_access_fsm (jt_id, jt_val);
				temp_same_expr = (it_val == jt_val) && (it_id.compare (jt_id) == 0);
			}
			same_expr &= temp_same_expr;
		}
	}
	return same_expr;	
}

int shiftvecNode::compute_dimensionality (vector<string> iters) {
	int dim = 0;
	for (auto it : indices) {
		string id = "";
		int val = 0;
		it->decompose_access_fsm (id, val);
		bool found = false;
		for (auto iter : iters) {
			found |= (id.find (iter) != string::npos); 	
		}
		if (found) 
			dim++;
	}
	return dim;
}

void shiftvecNode::determine_stmt_resource_mapping (map<tuple<string,int,bool>,RESOURCE> &rmap, vector<string> iters, bool stream, string stream_dim, bool use_shmem) {
	// Check if this access conforms to shared memory or register load.
	// Otherwise, just assign it global memory.
	bool confirms = false, central = false, found_streaming_iterator = false;
	string access_name (name);
	int streaming_pos = 0;
	vector<string> access_id;
	for (auto it : indices) {
		string id = "";
		int val = 0;
		it->decompose_access_fsm (id, val);
		if (!id.empty ())
			access_id.push_back (id);
	}
	vector<string>::iterator it = iters.begin();
	unsigned int confirming_size = 0;
	for (auto ac : access_id) {
		for (; it!=iters.end(); it++) {
			if (ac.compare (*it) == 0) {
				it++;
				confirming_size++;
				break;
			}
		}
	}
	confirms = (confirming_size == access_id.size ());
	// Check if this is central access, i.e., val component along other non-streaming dimensions is 0
	if (confirms) {
		central = true;
		int pos = 0;
		for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,pos++) {
			string id = "";
			int val = 0;
			(*it)->decompose_access_fsm (id, val);
			if (stream) {
				if (!id.empty () & (id.compare (stream_dim) != 0))
					central &= (val == 0);
				if (!id.empty () & (id.compare (stream_dim) == 0)) {
					streaming_pos = pos; 
					found_streaming_iterator = true;
				}
			}
			else {
				if (!id.empty ())
					central &= (val == 0);
			}
			if (id.empty ()) {
				access_name = access_name + "@" + to_string(val) + "@";
			}
		}
	}
	// Only bother about allocation of resource if it confirms to a standard stencil. Otherwise
	// default to global memory 
	if (confirms && found_streaming_iterator) {
		//Get the streaming plane
		exprNode *streaming_plane = indices[streaming_pos];
		string streaming_id = "";
		int streaming_val = 0;
		streaming_plane->decompose_access_fsm (streaming_id, streaming_val);
		assert (streaming_id.compare (stream_dim) == 0 && "Error in finding the streaming dimension");
		tuple<string,int,bool> access = make_tuple (access_name, streaming_val, true);
		bool exists = false;
		for (auto rm : rmap) {
			tuple<string,int,bool> first = rm.first;
			// Modify storage if the entry already exists
			if (get<0>(first).compare(access_name)==0 && get<1>(first)==streaming_val && get<2>(first)==true) {
				exists = true;
				// and the other indices are not central (val component is 0)
				if (!central) 
					rmap[rm.first] = downgrade_resource (rm.second, use_shmem);	
			}
		}
		// Create new entry for the access if not already present in resource map
		if (!exists) {
			// Put in register only if central, otherwise put in either shared memory or global memory
			RESOURCE res = central ? REGISTER : (use_shmem ? SHARED_MEM : GLOBAL_MEM);
			rmap[access] = res;	
		}
	}
	// For accesses that are n-k dimensional, without streaming dimension
	else if (confirms) {
		tuple<string,int,bool> access = make_tuple (access_name, 0, false);
		bool exists = false;
		for (auto rm : rmap) {
			tuple<string,int,bool> first = rm.first;
			// Downgrade storage if the entry already exists
			if (get<0>(first).compare(access_name)==0 && get<1>(first)==0 && get<2>(first)==false) {
				exists = true;
				if (!central) {
					rmap[rm.first] = downgrade_resource (rm.second, use_shmem);	
				}
			}
		}
		// Create new entry for the access if not already present in resource map
		if (!exists) {
			// Put the first access in register
			rmap[access] = central ? REGISTER : (use_shmem ? SHARED_MEM : GLOBAL_MEM);	
		}
	}
}

bool shiftvecNode::waw_dependence (exprNode *node) {
	bool waw_dependence = type == node->get_type ();
	waw_dependence &= expr_type == node->get_expr_type ();
	waw_dependence &= name.compare (node->get_name ()) == 0;
	if (waw_dependence) {
		vector<exprNode*> t_indices = dynamic_cast<shiftvecNode*>(node)->get_indices ();
		waw_dependence &= indices.size () == t_indices.size ();
	}
	// Last case, where they belong to different degree of freedom
	if (waw_dependence) {
		vector<exprNode*> t_indices = dynamic_cast<shiftvecNode*>(node)->get_indices ();
		vector<exprNode*>::iterator jt = t_indices.begin();
		for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,jt++) {
			string it_id = "", jt_id = "";
			int it_val = 0, jt_val = 0;
			(*it)->decompose_access_fsm (it_id, it_val);
		 	(*jt)->decompose_access_fsm (jt_id, jt_val);
			if (it_id.empty () && jt_id.empty () && (it_val != jt_val)) 
				waw_dependence = false;
		}
	}
	return waw_dependence;	
}

bool shiftvecNode::raw_dependence (exprNode *node) {
	bool raw_dependence = type == node->get_type ();
	raw_dependence &= expr_type == node->get_expr_type ();
	raw_dependence &= name.compare (node->get_name ()) == 0;
	if (raw_dependence) {
		vector<exprNode*> t_indices = dynamic_cast<shiftvecNode*>(node)->get_indices ();
		raw_dependence &= indices.size () == t_indices.size ();
	}
	// Last case, where they belong to different degree of freedom
	if (raw_dependence) {
		vector<exprNode*> t_indices = dynamic_cast<shiftvecNode*>(node)->get_indices ();
		vector<exprNode*>::iterator jt = t_indices.begin();
		for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,jt++) {
			string it_id = "", jt_id = "";
			int it_val = 0, jt_val = 0;
			(*it)->decompose_access_fsm (it_id, it_val);
		 	(*jt)->decompose_access_fsm (jt_id, jt_val);
			if (it_id.empty () && jt_id.empty () && (it_val != jt_val)) 
				raw_dependence = false;
		}
	}
	return raw_dependence;	
}

bool shiftvecNode::war_dependence (exprNode *node) {
	bool war_dependence = type == node->get_type ();
	war_dependence &= expr_type == node->get_expr_type ();
	war_dependence &= name.compare (node->get_name ()) == 0;
	if (war_dependence) {
		vector<exprNode*> t_indices = dynamic_cast<shiftvecNode*>(node)->get_indices ();
		war_dependence &= indices.size () == t_indices.size ();
	}
	// Last case, where they belong to different degree of freedom
	if (war_dependence) {
		vector<exprNode*> t_indices = dynamic_cast<shiftvecNode*>(node)->get_indices ();
		vector<exprNode*>::iterator jt = t_indices.begin();
		for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,jt++) {
			string it_id = "", jt_id = "";
			int it_val = 0, jt_val = 0;
			(*it)->decompose_access_fsm (it_id, it_val);
		 	(*jt)->decompose_access_fsm (jt_id, jt_val);
			if (it_id.empty () && jt_id.empty () && (it_val != jt_val)) 
				war_dependence = false;
		}
	}
	return war_dependence;	
}

//Compute the arrays that are read before they are written
void shiftvecNode:: compute_rbw (set<string> &reads, set<string> &writes, bool is_read) {
	if (writes.find (name) == writes.end ()) {
		if (is_read)
			reads.insert (name);
		else 
			writes.insert (name);
	}
}

//Compute the arrays whose written values are consumed
void shiftvecNode:: compute_wbr (set<string> &writes, set<string> &reads, bool is_write) {
	if (is_write) 
		writes.insert (name);
	else if (writes.find (name) != writes.end ()) 
	  reads.insert (name);
}

void shiftvecNode::compute_last_writes (map<string,exprNode*> & last_writes, bool stream, string stream_dim) {
	bool stream_dim_found = false;
	// Create the access name
	string access_name = name;
	for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++) {
		string id = "";
		int val = 0;
		(*it)->decompose_access_fsm (id, val);
		if (id.empty ()) {
			access_name = access_name + "@" + to_string(val) + "@";
		}
		else if (id.compare (stream_dim) == 0) 
			stream_dim_found = true;
	}
	if (last_writes.find (access_name) != last_writes.end ()) {
		exprNode *maccess = last_writes[access_name];
		// Compare both to see which is previous in streaming dimension
		if (stream && stream_dim_found) {
			// Find the streaming index for both maccess and current exprNode
			int cur_val=0, maccess_val=0;
			for (auto it : indices) {
				string id = "";
				int val = 0;
				it->decompose_access_fsm (id, val);
				if (!id.empty() && (id.compare (stream_dim) == 0)) {
					cur_val = val;
					break;
				}
			}
			for (auto it: dynamic_cast<shiftvecNode*>(maccess)->get_indices()) {
				string id = "";
				int val = 0;
				it->decompose_access_fsm (id, val);
				if (!id.empty() && (id.compare (stream_dim) == 0)) {
					maccess_val = val;
					break;
				}
			}
			if (cur_val <= maccess_val) 
				last_writes[access_name] = this; 
		}
		else 
			last_writes[access_name] = this;
	}
	else 
		last_writes[access_name] = this;
}

bool shiftvecNode::is_last_write (map<string,exprNode*> & last_writes) {
	// Create the access name
	string access_name = name;
	for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++) {
		string id = "";
		int val = 0;
		(*it)->decompose_access_fsm (id, val);
		if (id.empty ()) {
			access_name = access_name + "@" + to_string(val) + "@";
		}
	}
	return ((last_writes.find (access_name) != last_writes.end ()) && (last_writes[access_name] == this));
}

bool shiftvecNode::pointwise_occurrence (exprNode *node) {
	bool result = (type != node->get_type ());
	result |= (expr_type != node->get_expr_type ());
	result |= (name.compare (node->get_name ()) != 0);
	if (!result) {	
		vector<exprNode*> t_indices = dynamic_cast<shiftvecNode*>(node)->get_indices ();
		result = (indices.size () == t_indices.size ());
		if (result) {
			vector<exprNode*>::iterator jt=t_indices.begin();
			for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,jt++) {
				string it_id="", jt_id="";
				int it_val=0, jt_val=0;
				(*it)->decompose_access_fsm (it_id, it_val);
				(*jt)->decompose_access_fsm (jt_id, jt_val);
				if (it_id.empty () && jt_id.empty () && (it_val != jt_val)) {
					result = true;
					break; 
				}
				result &= (it_id.compare (jt_id) == 0) && (it_val == jt_val);
			}
		}
	}
	return result;
}

bool shiftvecNode::verify_immutable_expr (vector<string> writes, bool start_verification) {
	bool result = true;
	for (auto it : indices) {
		result &= it->verify_immutable_expr (writes, true);
	}
	return result;
}

int shiftvecNode::minimum_streaming_offset (string stream_dim) {
	int result = INT_MAX;
	// Get the offset in source along streaming dimension
	for (auto it : indices) {
		string id = "";
		int val = 0;
		it->decompose_access_fsm (id, val);
		if (!id.empty () && (stream_dim.compare (id) == 0)) {
			result = min (result, val);
		}
	}
	return result;
}

int shiftvecNode::streaming_offset (string stream_dim) {
	int result = 0;
	// Get the offset in source along streaming dimension
	for (auto it : indices) {
		string id = "";
		int val = 0;
		it->decompose_access_fsm (id, val);
		if (!id.empty () && (stream_dim.compare (id) == 0)) {
			result = val;
			break;
		}
	}
	return result;
}

int shiftvecNode::maximum_streaming_offset (exprNode *src_lhs, string stream_dim) {
	int result = 0;
	bool raw_dependence = type == src_lhs->get_type ();
	raw_dependence &= expr_type == src_lhs->get_expr_type ();
	raw_dependence &= name.compare (src_lhs->get_name ()) == 0;
	if (raw_dependence) {
		vector<exprNode*> t_indices = dynamic_cast<shiftvecNode*>(src_lhs)->get_indices ();
		raw_dependence &= indices.size () == t_indices.size ();
	}
	if (raw_dependence) {
		// Get the offset in source along streaming dimension
		vector<exprNode*> src_vec = dynamic_cast<shiftvecNode*>(src_lhs)->get_indices ();
		int src_offset = 0;
		for (auto it : src_vec) {
			string id = "";
			int val = 0;
			it->decompose_access_fsm (id, val);
			if (!id.empty () && (stream_dim.compare (id) == 0)) {
				src_offset = val;
			}		
		}
		for (auto it : indices) {
			string id = "";
			int val = 0;
			it->decompose_access_fsm (id, val);
			if (!id.empty () && (stream_dim.compare (id) == 0)) {
				result = max (result, val-src_offset);
			}
		}
	}
	return result;
}

void shiftvecNode::offset_expr (int offset, string stream_dim) {
	// First find the streaming iterator
	bool found_streaming_iterator = false;
	int streaming_pos = 0, pos = 0;
	for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,pos++) {
		string id = "";
		int val = 0;
		(*it)->decompose_access_fsm (id, val);
		if (!id.empty () & (id.compare (stream_dim) == 0)) {
			found_streaming_iterator = true;
			streaming_pos = pos;
		}
	}
	if (found_streaming_iterator) {
		exprNode *it = indices[streaming_pos];
		exprNode *temp = new binaryNode (T_MINUS, it, new datatypeNode<int> (offset, INT));
		indices[streaming_pos] = temp;
	}
}

void shiftvecNode::compute_hull (map<string, vector<Range*>> &hull_map, vector<Range*> &hull, vector<Range*> &initial_hull, bool stream, string stream_dim, int streaming_offset) {
	vector<Range*> new_hull = (hull_map.find(name) != hull_map.end()) ? hull_map[name] : initial_hull;
	unsigned int size = 0;
	for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++) {
		string id = "";
		int val = 0;
		(*it)->decompose_access_fsm (id, val);
		if (stream && !id.empty () && (id.compare (stream_dim) == 0)) {
			val += streaming_offset;
		}
		if (!id.empty ()) {
			Range *range = new_hull[size];
			exprNode *lo = range->get_lo_range ();
			exprNode *hi = range->get_hi_range ();
			if (val > 0) {
				// Modify hi
				exprNode *new_hi = new binaryNode (T_MINUS, hi, new datatypeNode<int>(val, INT), true);
				new_hull[size] = new Range (lo, new_hi);
			}
			else if (val < 0) {
				// Modify lo
				exprNode *new_lo = new binaryNode (T_PLUS, lo, new datatypeNode<int>(abs(val), INT), true);
				new_hull[size] = new Range (new_lo, hi);
			}
		}
		if (!id.empty ())
			size++;
	}
	// Take an intersection with the current hull
	vector<Range*>::iterator b_dom = hull.begin();
	int pos = 0;
	for (vector<Range*>::iterator a_dom=new_hull.begin(); a_dom!=new_hull.end(); a_dom++,b_dom++,pos++) {
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
		hull[pos] = new Range (lo, hi);
	}
}

void shiftvecNode::compute_hull (map<string, vector<Range*>> &hull_map, vector<Range*> &hull, vector<Range*> &initial_hull, map<tuple<string,int,bool>,RESOURCE> rmap, bool stream, string stream_dim, int streaming_offset) {
	RESOURCE res = GLOBAL_MEM;
	bool found_streaming_iterator = false;
	int streaming_pos = 0, offset = 0;
	tuple<string,int,bool> access;
	if (stream) {
		int pos = 0;
		for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,pos++) {
			string id = "";
			int val = 0;
			(*it)->decompose_access_fsm (id, val);
			if (!id.empty () && (id.compare (stream_dim) == 0)) {
				found_streaming_iterator = true;
				streaming_pos = pos;
			}
		}
	}
	if (found_streaming_iterator) {
		exprNode *it = indices[streaming_pos];
		string id = "";
		it->decompose_access_fsm (id, offset);
		assert (stream_dim.compare (id) == 0 && "streaming dimension not found (print_array)");
		access = make_tuple (name, offset, stream);
	}
	else
		access = make_tuple (name, offset, stream);
	if (rmap.find (access) != rmap.end ())
		res = rmap[access];

	// Compute hull only if the resource was not global memory
	bool not_in_hull_map = (hull_map.find(name) == hull_map.end());
	vector<Range*> new_hull = (hull_map.find(name) != hull_map.end()) ? hull_map[name] : initial_hull;
	if (!(res == GLOBAL_MEM && not_in_hull_map)) {
		unsigned int size = 0;
		for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++) {
			string id = "";
			int val = 0;
			(*it)->decompose_access_fsm (id, val);
			bool compute = !id.empty ();
                	if (stream && !id.empty () && (id.compare (stream_dim) == 0)) {
                	        val += streaming_offset;
                	}
#if 0
			if (compute) 
				compute &= !found_streaming_iterator || id.compare (stream_dim) != 0;
#endif
			if (compute) {
				Range *range = new_hull[size];
				exprNode *lo = range->get_lo_range ();
				exprNode *hi = range->get_hi_range ();
				if (val > 0) {
					// Modify hi
					exprNode *new_hi = new binaryNode (T_MINUS, hi, new datatypeNode<int>(val, INT), true);
					new_hull[size] = new Range (lo, new_hi);
				}
				else if (val < 0) {
					// Modify lo
					exprNode *new_lo = new binaryNode (T_PLUS, lo, new datatypeNode<int>(abs(val), INT), true);
					new_hull[size] = new Range (new_lo, hi);
				}
			}
			if (!id.empty ())
				size++;
		}
	}
	// Take an intersection with the current hull
	vector<Range*>::iterator b_dom = hull.begin();
	int pos = 0;
	for (vector<Range*>::iterator a_dom=new_hull.begin(); a_dom!=new_hull.end(); a_dom++,b_dom++,pos++) {
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
		hull[pos] = new Range (lo, hi);
	}
}

void shiftvecNode::shift_hull (vector<Range*> &hull, bool stream, string stream_dim) {
	bool found_streaming_iterator = false;
	if (stream) {
		for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++) {
			string id = "";
			int val = 0;
			(*it)->decompose_access_fsm (id, val);
			if (!id.empty () & (id.compare (stream_dim) == 0)) {
				found_streaming_iterator = true;
			}
		}
	}
	unsigned int size = 0;
	for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++) {
		string id = "";
		int val = 0;
		(*it)->decompose_access_fsm (id, val);
		bool shift = !id.empty ();
		if (shift) 
			shift &= !found_streaming_iterator || id.compare (stream_dim) != 0;
		if (shift) {
			Range *range = hull[size];
			exprNode *lo = range->get_lo_range ();
			exprNode *hi = range->get_hi_range ();
			if (val > 0) {
				// Modify lo and hi
				exprNode *new_lo = new binaryNode (T_PLUS, lo, new datatypeNode<int>(val, INT), true);
				exprNode *new_hi = new binaryNode (T_PLUS, hi, new datatypeNode<int>(val, INT), true);
				hull[size] = new Range (new_lo, new_hi);
			}
			else if (val < 0) {
				// Modify lo and hi
				exprNode *new_lo = new binaryNode (T_MINUS, lo, new datatypeNode<int>(abs(val), INT), true);
				exprNode *new_hi = new binaryNode (T_MINUS, hi, new datatypeNode<int>(abs(val), INT), true);
				hull[size] = new Range (new_lo, new_hi);
			}
		}
		if (!id.empty ())
			size++;
	}
}

void shiftvecNode::shift_hull (vector<Range*> &hull) {
	unsigned int size = 0;
	for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++) {
		string id = "";
		int val = 0;
		(*it)->decompose_access_fsm (id, val);
		if (!id.empty ()) {
			Range *range = hull[size];
			exprNode *lo = range->get_lo_range ();
			exprNode *hi = range->get_hi_range ();
			if (val > 0) {
				// Modify lo and hi
				exprNode *new_lo = new binaryNode (T_PLUS, lo, new datatypeNode<int>(val, INT), true);
				exprNode *new_hi = new binaryNode (T_PLUS, hi, new datatypeNode<int>(val, INT), true);
				hull[size] = new Range (new_lo, new_hi);
			}
			else if (val < 0) {
				// Modify lo and hi
				exprNode *new_lo = new binaryNode (T_MINUS, lo, new datatypeNode<int>(abs(val), INT), true);
				exprNode *new_hi = new binaryNode (T_MINUS, hi, new datatypeNode<int>(abs(val), INT), true);
				hull[size] = new Range (new_lo, new_hi);
			}
		}
		if (!id.empty ())
			size++;
	}
}

bool shiftvecNode::same_subscripts_for_array (string arr_name, vector<string> &subscripts, DATA_TYPE &arr_type) {
	bool ret = true;
	if (name.compare (arr_name) == 0) {
		if (subscripts.empty ()) {
			for (auto it : indices) {
				string id = "";
				int val = 0;
				it->decompose_access_fsm (id, val);
				if (!id.empty ())
					subscripts.push_back (id);
				else 
					subscripts.push_back (to_string(val));
			}
			arr_type = type;
		}
		else {
			ret &= ((arr_type == type) && (indices.size() == subscripts.size()));
			if (ret) {
				int index = 0;
				for (auto it : indices) {
					string id = "";
					int val = 0;
					it->decompose_access_fsm (id, val);
					if (!id.empty ()) 
						ret &= (id.compare (subscripts[index]) == 0);
					else 
						ret &= ((to_string(val)).compare (subscripts[index]) == 0);
					index++;
				}
			}
		}
	}
        return ret;
}

void shiftvecNode::replace_array_name (string old_name, string new_name) {
	if (name.compare (old_name) == 0) {
		name = new_name;
	}
}

void shiftvecNode::accessed_arrays (set<string> &arrays) {
	string access_id = name;	
	for (auto it : indices) {
		string id = "";
		int val = 0;
		it->decompose_access_fsm (id, val);
		if (id.empty ())
			access_id += "[" + to_string(val) + "]";
	}
	arrays.insert (access_id);
}

void shiftvecNode::collect_access_stats (map<tuple<string,int,bool>,tuple<int,int,int>> &access_map, bool is_read, map<string,int> blockdims, vector<string> iters, bool stream, string stream_dim) {
	int read =  is_read ? 1 : 0;
	int write = is_read ? 0 : 1;
	int domsize = 1;
	vector<string> dim_string = (iters.size() == 3) ? vector<string>({"z", "y", "x"}) : ((iters.size() == 2) ? vector<string>({"y", "x"}) : vector<string> ({"x"}));

	// Create the key for access_map
	bool confirms = false, central = false, found_streaming_iterator = false;
	string access_name (name);
	int streaming_pos = 0;
	vector<string> access_id;
	for (auto it : indices) {
		string id = "";
		int val = 0;
		it->decompose_access_fsm (id, val);
		if (!id.empty ())
			access_id.push_back (id);
	}
	vector<string>::iterator it = iters.begin();
	unsigned int confirming_size = 0;
	for (auto ac : access_id) {
		for (; it!=iters.end(); it++) {
			if (ac.compare (*it) == 0) {
				it++;
				confirming_size++;
				break;
			}
		}
	}
	confirms = (confirming_size == access_id.size ());
	// Check if this is central access, i.e., val component along other non-streaming dimensions is 0
	if (confirms) {
		central = true;
		int pos = 0;
		for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,pos++) {
			string id = "";
			int val = 0;
			(*it)->decompose_access_fsm (id, val);
			if (!id.empty () & (id.compare (stream_dim) != 0))
				central &= (val == 0);
			if (stream & !id.empty () & (id.compare (stream_dim) == 0)) {
				streaming_pos = pos; 
				found_streaming_iterator = true;
			}
			if (id.empty ()) {
				access_name = access_name + "@" + to_string(val) + "@";
			}
		}
	}
	if (confirms && found_streaming_iterator) {
		//Get the streaming plane
		exprNode *streaming_plane = indices[streaming_pos];
		string streaming_id = "";
		int streaming_val = 0;
		streaming_plane->decompose_access_fsm (streaming_id, streaming_val);
		assert (streaming_id.compare (stream_dim) == 0 && "Error in finding the streaming dimension");
		tuple<string,int,bool> access = make_tuple (access_name, streaming_val, true);
		//Compute the domain size
		int pos = 0;
		for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,pos++) {
			if (pos == streaming_pos) 
				continue;
			string id = "";
			int val = 0;
			(*it)->decompose_access_fsm (id, val);
			if (!id.empty ()) {
				vector<string>::iterator jt = find (iters.begin(), iters.end (), id);
				assert (jt != iters.end ());
				domsize *= blockdims[dim_string[jt-iters.begin()]];
			}
		}
		// Multiply the domain size with the data type size
		domsize *= get_data_size (type);
		if (access_map.find (access) != access_map.end ()) {
			read  += get<0>(access_map[access]);
			write += get<1>(access_map[access]);
			domsize = max (domsize, get<2>(access_map[access]));	
		}
		access_map[access] = make_tuple (read, write, domsize);
	}
	else if (confirms) {
		tuple<string,int,bool> access = make_tuple (access_name, 0, false);
		//Compute the domain size
		for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++) {
			string id = "";
			int val = 0;
			(*it)->decompose_access_fsm (id, val);
			if (!id.empty ()) {
				vector<string>::iterator jt = find (iters.begin(), iters.end (), id);
				assert (jt != iters.end ());
				domsize *= blockdims[dim_string[jt-iters.begin()]];
			}
		}
		// Multiply the domain size with the data type size
		domsize *= get_data_size (type);
		if (access_map.find (access) != access_map.end ()) {
			read  += get<0>(access_map[access]);
			write += get<1>(access_map[access]);
			domsize = max (domsize, get<2>(access_map[access]));
		}
		access_map[access] = make_tuple (read, write, domsize);
	}
}

void shiftvecNode::collect_access_stats (map<tuple<string,int,bool>,tuple<int,int,map<int,vector<string>>>> &access_map, bool is_read, vector<string> iters, bool stream, string stream_dim) {
	int read =  is_read ? 1 : 0;
	int write = is_read ? 0 : 1;

	// Create the key for access_map
	bool confirms = false, central = false, found_streaming_iterator = false;
	string access_name (name);
	int streaming_pos = 0;
	vector<string> access_id;
	for (auto it : indices) {
		string id = "";
		int val = 0;
		it->decompose_access_fsm (id, val);
		if (!id.empty ())
			access_id.push_back (id);
	}
	vector<string>::iterator it = iters.begin();
	unsigned int confirming_size = 0;
	for (auto ac : access_id) {
		for (; it!=iters.end(); it++) {
			if (ac.compare (*it) == 0) {
				it++;
				confirming_size++;
				break;
			}
		}
	}
	confirms = (confirming_size == access_id.size ());
	// Check if this is central access, i.e., val component along other non-streaming dimensions is 0
	if (confirms) {
		central = true;
		int pos = 0;
		for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,pos++) {
			string id = "";
			int val = 0;
			(*it)->decompose_access_fsm (id, val);
			if (!id.empty () & (id.compare (stream_dim) != 0))
				central &= (val == 0);
			if (stream & !id.empty () & (id.compare (stream_dim) == 0)) {
				streaming_pos = pos; 
				found_streaming_iterator = true;
			}
			if (id.empty ()) {
				access_name = access_name + "@" + to_string(val) + "@";
			}
		}
	}
	if (confirms && found_streaming_iterator) {
		//Get the streaming plane
		exprNode *streaming_plane = indices[streaming_pos];
		string streaming_id = "";
		int streaming_val = 0;
		streaming_plane->decompose_access_fsm (streaming_id, streaming_val);
		assert (streaming_id.compare (stream_dim) == 0 && "Error in finding the streaming dimension");
		tuple<string,int,bool> access = make_tuple (access_name, streaming_val, true);
		map<int,vector<string>> index_map;
		if (access_map.find (access) != access_map.end ()) {
			read  += get<0>(access_map[access]);
			write += get<1>(access_map[access]);
			index_map = get<2>(access_map[access]);
		}
		int pos = 0;
		for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,pos++) {
			//if (pos == streaming_pos) 
			//	continue;
			string id = "";
			int val = 0;
			(*it)->decompose_access_fsm (id, val);
			if (!id.empty ()) {
				vector<string>::iterator jt = find (iters.begin(), iters.end (), id);
				assert (jt != iters.end ());
				// Add to index_map
				if (index_map.find (pos) == index_map.end ()) {
					index_map[pos].push_back (id);
				}
				else {
					if (find (index_map[pos].begin(), index_map[pos].end(), id) == index_map[pos].end())
						index_map[pos].push_back (id);
				}
			}
			//else if (id.empty ()) {
			//    id = to_string (val);
			//	// Add to index_map
			//	if (index_map.find (pos) == index_map.end ()) {
			//		index_map[pos].push_back (id);
			//	}
			//	else {
			//		if (find (index_map[pos].begin(), index_map[pos].end(), id) == index_map[pos].end())
			//			index_map[pos].push_back (id);
			//	}
			//}
		}
		access_map[access] = make_tuple (read, write, index_map);
	}
	else if (confirms) {
		tuple<string,int,bool> access = make_tuple (access_name, 0, false);
		map<int,vector<string>> index_map;
		if (access_map.find (access) != access_map.end ()) {
			read  += get<0>(access_map[access]);
			write += get<1>(access_map[access]);
			index_map = get<2>(access_map[access]);
		}
		int pos = 0;
		for (vector<exprNode*>::iterator it=indices.begin(); it!=indices.end(); it++,pos++) {
			string id = "";
			int val = 0;
			(*it)->decompose_access_fsm (id, val);
			if (!id.empty ()) {
				vector<string>::iterator jt = find (iters.begin(), iters.end (), id);
				assert (jt != iters.end ());
				// Add to index_map
				if (index_map.find (pos) == index_map.end ()) {
					index_map[pos].push_back (id);
				}
				else {
					if (find (index_map[pos].begin(), index_map[pos].end(), id) == index_map[pos].end())
						index_map[pos].push_back (id);
				}
			}
		}
		access_map[access] = make_tuple (read, write, index_map);
	}
}

exprNode *shiftvecNode::unroll_expr (string iter, int unroll_instance, map<string,int> &scalar_version_map, bool is_lhs) {
	shiftvecNode *ret = new shiftvecNode (name, type, nested);
	for (auto ind : indices) {
		exprNode *temp = ind->unroll_expr (iter, unroll_instance, scalar_version_map, false);
		ret->push_index (temp);
	}
	return ret;
}

int functionNode::get_retiming_offset (string stream_dim) {
	int ret = INT_MIN;
	for (auto a : arg_list) {
		ret = max (ret, a->get_retiming_offset (stream_dim));
	}
	return ret; 
}

exprNode *functionNode::retime_node (string stream_dim, int offset) {
	functionNode *ret = new functionNode (*this);
	unsigned pos = 0;
	for (auto a : arg_list) {
		ret->set_arg (a->retime_node (stream_dim, offset), pos);
		pos++;
	}
	return ret;
}

bool functionNode::retiming_feasible (string stream_dim, vector<int> &offset) {
	bool ret = true;
	for (auto a : arg_list) {
		ret &= a->retiming_feasible (stream_dim, offset);
	}
	return ret; 
}

int functionNode::get_flop_count (void) {
	int ret = flops_per_function (name);
	for (auto a : arg_list) {
		ret += a->get_flop_count ();
	}
	return ret;
}

bool functionNode::is_scalar_expr (void) {
	bool ret = true;
	for (auto a : arg_list) {
		ret &= a->is_scalar_expr ();
	}
	return ret;
}

void functionNode::collect_accesses (set<string> &accesses) {
	//	accesses.insert (name);
	for (auto a : arg_list) {
		a->collect_accesses (accesses);	
	}
}

void functionNode::infer_expr_type (DATA_TYPE &ret) {
	for (auto a : arg_list) {
		a->infer_expr_type (ret);
	}
}

void functionNode::print_node (stringstream &out) {
	if (nested)
		out << "(";
	out << name << "(";
	for (auto l : arg_list) 		
		l->print_node (out);
	out << ")";
	if (nested)
		out << ")";
}

void functionNode::print_node (stringstream &out, map<tuple<string,int,bool>,RESOURCE> rmap, bool is_lhs, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays, map<string,int> unroll_instance, bool blocked_loads, map<string,int> udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, string &cstream_shift) {
	if (nested)
		out << "(";
	out << name << "(";
	for (auto a : arg_list) {
		a->print_node (out, rmap, is_lhs, full_stream, stream, stream_dim, temp_arrays, unroll_instance, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, cstream_shift);
	}
	out << ")";
	if (nested) 
		out << ")";
}

void functionNode::print_node (stringstream &out, map<tuple<string,int,bool>,RESOURCE> rmap, bool is_lhs, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays, bool blocked_loads, map<string,int> udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, map<string,tuple<int,int>> halo_dims, int reg_count, string &cstream_shift) {
	if (nested)
		out << "(";
	out << name << "(";
	for (auto a : arg_list) {
		a->print_node (out, rmap, is_lhs, full_stream, stream, stream_dim, temp_arrays, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, halo_dims, reg_count, cstream_shift);
	}
	out << ")";
	if (nested) 
		out << ")";
}

void functionNode::print_last_write (stringstream &out, map<tuple<string,int,bool>,RESOURCE> rmap, map<string,exprNode*> last_writes, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays, map<string,int> unroll_instance, bool blocked_loads, map<string,int> udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, string &cstream_shift) {
	for (auto a : arg_list) {
		a->print_last_write(out, rmap, last_writes, full_stream, stream, stream_dim, temp_arrays, unroll_instance, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, cstream_shift);
	}
}

void functionNode::print_last_write (stringstream &out, map<tuple<string,int,bool>,RESOURCE> rmap, map<string,exprNode*> last_writes, bool full_stream, bool stream, string stream_dim, vector<string> temp_arrays, bool blocked_loads, map<string,int> udecls, vector<string> params, vector<string> iters, bool linearize_accesses, bool code_diff, map<string,tuple<int,int>> halo_dims, int reg_count, string &cstream_shift) {
	for (auto a : arg_list) {
		a->print_last_write(out, rmap, last_writes, full_stream, stream, stream_dim, temp_arrays, blocked_loads, udecls, params, iters, linearize_accesses, code_diff, halo_dims, reg_count, cstream_shift);
	}
}

bool functionNode::waw_dependence (exprNode *node) {
	bool waw_dependence = false;
	for (auto a : arg_list) {
		waw_dependence |= a->waw_dependence (node);
	}
	return waw_dependence;
}

bool functionNode::raw_dependence (exprNode *node) {
	bool raw_dependence = false;
	for (auto a : arg_list) {
		raw_dependence |= a->raw_dependence (node);
	}
	return raw_dependence;
}

bool functionNode::war_dependence (exprNode *node) {
	bool war_dependence = false;
	for (auto a : arg_list) {
		war_dependence |= a->war_dependence (node);
	}
	return war_dependence;
}

bool functionNode::pointwise_occurrence (exprNode *node) {
	bool pointwise_occurrence = true;
	for (auto a : arg_list) {
		pointwise_occurrence &= a->pointwise_occurrence (node);
	}
	return pointwise_occurrence;
}

void functionNode:: compute_rbw (set<string> &reads, set<string> &writes, bool is_read) {
	for (auto a : arg_list) {
		a->compute_rbw (reads, writes, is_read);
	}
}

void functionNode:: compute_wbr (set<string> &writes, set<string> &reads, bool is_write) {
	for (auto a : arg_list) {
		a->compute_wbr (writes, reads, is_write);
	}
}

int functionNode::compute_dimensionality (vector<string> iters) {
	int dim = 0;
	for (auto a : arg_list) {
		int arg_dim = a->compute_dimensionality (iters);
		dim = max (dim, arg_dim);
	}
	return dim;
}

void functionNode::determine_stmt_resource_mapping (map<tuple<string,int,bool>,RESOURCE> &rmap, vector<string> iters, bool stream, string stream_dim, bool use_shmem) {
	for (auto l : arg_list) {
		l->determine_stmt_resource_mapping (rmap, iters, stream, stream_dim, use_shmem); 
	}
}

bool functionNode::verify_immutable_expr (vector<string> writes, bool start_verification) {
	bool result = true;
	for (auto a : arg_list) {
		result &= a->verify_immutable_expr (writes, start_verification);
	}
	return result;
}

int functionNode::minimum_streaming_offset (string stream_dim) {
	int result = INT_MAX;
	for (auto a : arg_list) {
		result = min (result, a->minimum_streaming_offset (stream_dim));
	}
	return result;
}

int functionNode::maximum_streaming_offset (exprNode *src_lhs, string stream_dim) {
	int result = 0;
	for (auto a : arg_list) {
		result = max (result, a->maximum_streaming_offset (src_lhs, stream_dim));
	}
	return result;
}

void functionNode::offset_expr (int offset, string stream_dim) {
	for (auto a : arg_list) {
		a->offset_expr (offset, stream_dim);
	}
}

void functionNode::set_data_type (map<string, DATA_TYPE> data_types) {
	for (auto a : arg_list) {
		a->set_data_type (data_types);
	}
}

void functionNode::compute_hull (map<string, vector<Range*>> &hull_map, vector<Range*> &hull, vector<Range*> &initial_hull, bool stream, string stream_dim, int offset) {
	for (auto a : arg_list) {
		a->compute_hull (hull_map, hull, initial_hull, stream, stream_dim, offset);
	}
}

void functionNode::compute_hull (map<string, vector<Range*>> &hull_map, vector<Range*> &hull, vector<Range*> &initial_hull, map<tuple<string,int,bool>,RESOURCE> rmap, bool stream, string stream_dim, int offset) {
	for (auto a : arg_list) {
		a->compute_hull (hull_map, hull, initial_hull, rmap, stream, stream_dim, offset);
	}
}

void functionNode::shift_hull (vector<Range*> &hull) {
	for (auto a : arg_list) {
		a->shift_hull (hull);
	}
}

void functionNode::shift_hull (vector<Range*> &hull, bool stream, string stream_dim) {
	for (auto a : arg_list) {
		a->shift_hull (hull, stream, stream_dim);
	}
}

void functionNode::collect_access_stats (map<tuple<string,int,bool>,tuple<int,int,int>> &access_map, bool is_read, map<string,int> blockdims, vector<string> iters, bool stream, string stream_dim) {
	for (auto a : arg_list) {
		a->collect_access_stats (access_map, is_read, blockdims, iters, stream, stream_dim);
	}
}

void functionNode::collect_access_stats (map<tuple<string,int,bool>, tuple<int,int,map<int,vector<string>>>> &access_map, bool is_read, vector<string> iters, bool stream, string stream_dim) {
	for (auto a : arg_list) {
		a->collect_access_stats (access_map, is_read, iters, stream, stream_dim);
	}
}

void functionNode::accessed_arrays (set<string> &arrays) {
	for (auto a : arg_list) {
		a->accessed_arrays (arrays);
	}
}

exprNode *functionNode::unroll_expr (string iter, int unroll_instance, map<string,int> &scalar_version_map, bool is_lhs) {
	std::vector<exprNode*> *new_arg = new std::vector<exprNode*> ();
	for (auto a : arg_list) {
		exprNode *temp = a->unroll_expr (iter, unroll_instance, scalar_version_map, false);
		new_arg->push_back (temp);
	}
	return new functionNode (name, new_arg, type, nested);
}

bool functionNode::same_subscripts_for_array (string arr_name, vector<string> &subscripts, DATA_TYPE &arr_type) {
        bool ret = true;
	for (auto a : arg_list) {
        	ret &= a->same_subscripts_for_array (arr_name, subscripts, arr_type);
        }
	return ret;
}

void functionNode::replace_array_name (string old_name, string new_name) {
	for (auto a : arg_list) {
        	a->replace_array_name (old_name, new_name);
	}
}
