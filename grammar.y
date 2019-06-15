%{
	#include <cstdio>
	#include <string>
	#include <vector>
	#include "grammar.hpp"

	extern FILE *yyin;
	extern int yylineno;
	int yylex (void);
	int yyparse (void);
	void yyerror(const char *str) {
		fprintf(stderr,"error: %s at line %d\n", str, yylineno);
	}
	void grammar::set_input (FILE *in) {
	    yyin = in;
	}
	void grammar::parse () {
	    do {
	        yyparse ();
	    } while (!feof (yyin));
	}
%}

%union{
	int ival;
	double dval;
	float fval;
	bool bval;
	char *str;
	class startNode *startnode;
	class funcDefn *funcdefn;
	class stencilDefn *stencildefn;
	class stencilCall *stencilcall;
	class stmtList *stmtlist;
	class exprNode *exprnode;
	class shiftvecNode *shiftvecnode;
	class argList *arglist;
	class Range *iterrange;
	class arrayDecl *arraydecl;
	std::vector<std::string> *stringlist;
	std::vector<exprNode*> *exprlist;
	std::vector<arrayDecl*> *arraylist;
	std::vector<Range*> *rangelist;
}

/*
%locations
%define parse.lac full
%define parse.error verbose
*/

%token <str> ID
%token <ival> DATATYPE
%token <ival> TRUE
%token <ival> FALSE
%token <ival> T_INT
%token <fval> T_FLOAT
%token <dval> T_DOUBLE
%token PARAMETER FUNCTION STENCIL COEFFICIENT ITERATOR LEQ GEQ EQ NEQ PLUSEQ MINUSEQ MULTEQ DIVEQ ANDEQ OREQ COLON DDOTS COMMENT ALLOCIN COPYIN CONSTANT COPYOUT SHMEM NOSHMEM GMEM ITERATE INOUT

/* Associativity */
%left '|'
%left '&'
%left EQ NEQ
%left '<' LEQ '>' GEQ
%left '+' '-'
%left '*' '/' '%'
%left UMINUS UPLUS
%left '^'

%type <startnode> program
%type <stmtlist> pointstmts 
%type <exprnode> lhs 
%type <exprnode> rhs 
%type <exprnode> scalar_rhs
%type <shiftvecnode> offsetvar
%type <shiftvecnode> offsetlist
%type <exprnode> arrayaccess 
%type <stringlist> copyinlist
%type <stringlist> allocinlist
%type <stringlist> copyoutlist
%type <stringlist> constantlist
%type <stringlist> idlist
%type <arglist> arglist
%type <arraylist> arraylist
%type <rangelist> rangelist
%type <exprnode> range
%type <iterrange> stencilloop 
%type <exprlist> rhslist
%type <exprlist> scalar_rhslist
%type <stencilcall> stencilcall
%%

start : {grammar::start = new startNode ();} program {}
	;

program : program functiondefn {}
	| functiondefn {} 
	;

functiondefn : {grammar::start->create_func_defn ();} paramlist iterlist declist allocinlist copyinlist constantlist coeflist stencildefs stencilcalls copyoutlist {grammar::start->increment_func_defn_count();}
	;

/* Function definition */
stencildefs : stencildefs stencildef {}
	| stencildef {}
	;

stencildef : STENCIL ID '(' arglist ')' '{' tempdeclist shmemlist noshmemlist gmemlist pointstmts '}' {stencilDefn *node = new stencilDefn ($11, $4);
							grammar::start->push_stencil_defn ($2, node);} 
	| FUNCTION ID '(' arglist ')' '{' tempdeclist shmemlist noshmemlist gmemlist pointstmts '}' {stencilDefn *node = new stencilDefn ($11, $4);
                            grammar::start->push_stencil_defn ($2, node);}
	;

/* Parameter list */
paramlist : PARAMETER idlist ';'{grammar::start->push_parameter ($2);}
	| {}
	;

/* Iterator list */
iterlist : ITERATOR idlist ';' {grammar::start->push_iterator ($2);}
	| {}
	;

/* Declaration of variables or arrays */
declist : declist decl 
	| {}
	;

decl : DATATYPE idlist ';' {grammar::start->push_var_decl ($2, get_data_type ($1));}
	| DATATYPE arraylist ';' {grammar::start->push_array_decl ($2, get_data_type ($1));} 

/* Shared memory declarations */
shmemlist : shmemlist shmemdecl
	| {}
	;

shmemdecl : SHMEM idlist ';' {grammar::start->push_shmem_decl ($2);}

/* No shared memory declarations */
noshmemlist : noshmemlist noshmemdecl
	| {}
	;
	
noshmemdecl : NOSHMEM idlist ';' {grammar::start->push_noshmem_decl ($2);}

/* Global memory declarations */
gmemlist : gmemlist gmemdecl
	| {}
	;

gmemdecl : GMEM idlist ';' {grammar::start->push_gmem_decl ($2);}

/* Declaration of temporary variables or arrays */
tempdeclist : tempdeclist tempdecl 
	| {}
	;

tempdecl : DATATYPE idlist ';' {grammar::start->push_temp_var_decl ($2, get_data_type ($1));}
	| DATATYPE arraylist ';' {grammar::start->push_temp_array_decl ($2, get_data_type ($1));} 
	| DATATYPE ID '=' scalar_rhs ';' {grammar::start->push_temp_var_init_decl (get_data_type ($1), $2, $4);} 

/* The range of array declarations. Currently, they are simple */
range : ID '+' T_INT {$$ = new binaryNode (T_PLUS, new idNode ($1), new datatypeNode<int> ($3, INT), false);}
 	| ID '-' T_INT {$$ = new binaryNode (T_MINUS, new idNode ($1), new datatypeNode<int> ($3, INT), false);}
	| ID {$$ = new idNode ($1);}
	| T_INT {$$ = new datatypeNode<int> ($1, INT);}
	;

rangelist : rangelist ',' range {$1->push_back (new Range (new datatypeNode<int> (0, INT), $3));
					$$ = $1;}
	| rangelist ']''[' range {$1->push_back (new Range (new datatypeNode<int> (0, INT), $4));
                    $$ = $1;}
	| rangelist ',' range COLON range {$1->push_back (new Range ($3, $5));
					$$ = $1;}
	| rangelist ']''[' range COLON range {$1->push_back (new Range ($4, $6));
					$$ = $1;}
	| rangelist ',' range DDOTS range {$1->push_back (new Range ($3, $5));
					$$ = $1;}
	| rangelist ']''[' range DDOTS range {$1->push_back (new Range ($4, $6));
					$$ = $1;}
	| '(' range ')' {std::vector<Range*> *node = new std::vector<Range*> (); 
			node->push_back (new Range (new datatypeNode<int> (0, INT), $2));
			$$ = node;}
	| range {std::vector<Range*> *node = new std::vector<Range*> ();
			node->push_back (new Range (new datatypeNode<int> (0, INT), $1));
			$$ = node;}
	| range COLON range {std::vector<Range*> *node = new std::vector<Range*> ();
			node->push_back (new Range ($1, $3));
			$$ = node;}
	| range DDOTS range {std::vector<Range*> *node = new std::vector<Range*> ();
			node->push_back (new Range ($1, $3));
			$$ = node;}
	;

/* Coefficient list */
coeflist : COEFFICIENT idlist ';'{grammar::start->push_coefficient ($2);}
	| {}
	;

/* A list of statements describing accesses at generic point, of the form LHS = RHS */
pointstmts : 
	  pointstmts lhs '=' rhs ';' {stmtNode *node = new stmtNode (ST_EQ, $2, $4);
					$1->push_stmt (node);
					$$ = $1;}
	| pointstmts lhs PLUSEQ rhs ';' {stmtNode *node = new stmtNode (ST_PLUSEQ, $2, $4);
					$1->push_stmt (node);
					$$ = $1;}
	| pointstmts lhs MINUSEQ rhs ';' {stmtNode *node = new stmtNode (ST_MINUSEQ, $2, $4);
					$1->push_stmt (node);
					$$ = $1;}
	| pointstmts lhs MULTEQ rhs ';' {stmtNode *node = new stmtNode (ST_MULTEQ, $2, $4);
					$1->push_stmt (node);
					$$ = $1;}
	| pointstmts lhs DIVEQ rhs ';' {stmtNode *node = new stmtNode (ST_DIVEQ, $2, $4);
					$1->push_stmt (node);
					$$ = $1;}
	| pointstmts lhs ANDEQ rhs ';' {stmtNode *node = new stmtNode (ST_ANDEQ, $2, $4);
					$1->push_stmt (node);
					$$ = $1;}
	| pointstmts lhs OREQ rhs ';' {stmtNode *node = new stmtNode (ST_OREQ, $2, $4);
					$1->push_stmt (node);
					$$ = $1;}
	| pointstmts '[' rangelist ']' COLON lhs '=' rhs ';' {stmtNode *node = new stmtNode (ST_EQ, $6, $8, *($3));
					$1->push_stmt (node);
					$$ = $1;}
 	| pointstmts '[' rangelist ']' COLON lhs PLUSEQ rhs ';' {stmtNode *node = new stmtNode (ST_PLUSEQ, $6, $8, *($3));
					$1->push_stmt (node);
					$$ = $1;}
	| pointstmts '[' rangelist ']' COLON lhs MINUSEQ rhs ';' {stmtNode *node = new stmtNode (ST_MINUSEQ, $6, $8, *($3));
					$1->push_stmt (node);
					$$ = $1;}
	| pointstmts '[' rangelist ']' COLON lhs MULTEQ rhs ';' {stmtNode *node = new stmtNode (ST_MULTEQ, $6, $8, *($3));
					$1->push_stmt (node);
					$$ = $1;}
	| pointstmts '[' rangelist ']' COLON lhs DIVEQ rhs ';' {stmtNode *node = new stmtNode (ST_DIVEQ, $6, $8, *($3));
					$1->push_stmt (node);
					$$ = $1;}
	| pointstmts '[' rangelist ']' COLON lhs ANDEQ rhs ';' {stmtNode *node = new stmtNode (ST_ANDEQ, $6, $8, *($3));
					$1->push_stmt (node);
					$$ = $1;}
	| pointstmts '[' rangelist ']' COLON lhs OREQ rhs ';' {stmtNode *node = new stmtNode (ST_OREQ, $6, $8, *($3));
					$1->push_stmt (node);
					$$ = $1;}
	/* Add support for comments */
	| pointstmts COMMENT lhs '=' rhs ';' {$$ = $1;}
	| pointstmts COMMENT lhs PLUSEQ rhs ';' {$$ = $1;}
	| pointstmts COMMENT lhs MINUSEQ rhs ';' {$$ = $1;}
	| pointstmts COMMENT lhs MULTEQ rhs ';' {$$ = $1;}
	| pointstmts COMMENT lhs DIVEQ rhs ';' {$$ = $1;}
	| pointstmts COMMENT lhs ANDEQ rhs ';' {$$ = $1;}
	| pointstmts COMMENT lhs OREQ rhs ';' {$$ = $1;}
	| pointstmts COMMENT '[' rangelist ']' COLON lhs '=' rhs ';' {$$ = $1;}
	| pointstmts COMMENT '[' rangelist ']' COLON lhs PLUSEQ rhs ';' {$$ = $1;}
	| pointstmts COMMENT '[' rangelist ']' COLON lhs MINUSEQ rhs ';' {$$ = $1;}
	| pointstmts COMMENT '[' rangelist ']' COLON lhs MULTEQ rhs ';' {$$ = $1;}
	| pointstmts COMMENT '[' rangelist ']' COLON lhs DIVEQ rhs ';' {$$ = $1;}
	| pointstmts COMMENT '[' rangelist ']' COLON lhs ANDEQ rhs ';' {$$ = $1;}
	| pointstmts COMMENT '[' rangelist ']' COLON lhs OREQ rhs ';' {$$ = $1;}
	| {stmtList *node = new stmtList (); 
		$$ = node;}
	;

/* A single statement of the form (B[k+2,j+1,i+3] + 2.0f*C[k+1,j+3,i+2])*/
lhs : ID {$$ = new idNode ($1);}
	| offsetvar {$$ = $1;}
	| '(' lhs ')' {$$ = $2;}
	;

/* A single statement of the form (B[k+2,j+1,i+3] + 2.0f*C[k+1,j+3,i+2])*/
rhs : ID {$$ = new idNode ($1);}
	| offsetvar {$$ = $1;}
	| TRUE {$$ = new datatypeNode<bool> ($1, BOOL);}
	| FALSE {$$ = new datatypeNode<bool> ($1, BOOL);}
	| T_INT {$$ = new datatypeNode<int> ($1, INT);}
	| T_DOUBLE {$$ = new datatypeNode<double> ($1, DOUBLE);}
	| T_FLOAT {$$ = new datatypeNode<float> ($1, FLOAT);}
	| rhs '|' rhs {$$ = new binaryNode (T_OR, $1, $3);} 
	| rhs '&' rhs {$$ = new binaryNode (T_AND, $1, $3);} 
	| rhs EQ rhs {$$ = new binaryNode (T_EQ, $1, $3);}
	| rhs NEQ rhs {$$ = new binaryNode (T_NEQ, $1, $3);}
	| rhs '<' rhs {$$ = new binaryNode (T_LT, $1, $3);}
	| rhs LEQ rhs {$$ = new binaryNode (T_LEQ, $1, $3);}
	| rhs '>' rhs {$$ = new binaryNode (T_GT, $1, $3);}
	| rhs GEQ rhs {$$ = new binaryNode (T_GEQ, $1, $3);}
	| rhs '+' rhs {$$ = new binaryNode (T_PLUS, $1, $3);}  
	| rhs '-' rhs {$$ = new binaryNode (T_MINUS, $1, $3);}
	| rhs '*' rhs {$$ = new binaryNode (T_MULT, $1, $3);}
	| rhs '/' rhs {$$ = new binaryNode (T_DIV, $1, $3);}
	| rhs '%' rhs {$$ = new binaryNode (T_MOD, $1, $3);}
	| '-' rhs %prec UMINUS {$$ = new uminusNode ($2);} 
	| rhs '^' rhs {$$ = new binaryNode (T_EXP, $1, $3);}
	| '(' rhs ')' {$2->set_nested (); 
			$$ = $2;}
	| ID '(' rhslist ')' {$$ = new functionNode ($1, $3);}
	;

rhslist : rhslist ',' rhs {$1->push_back ($3);
		$$ = $1;}
	| rhs {std::vector<exprNode*> *node = new std::vector<exprNode*> ();
		node->push_back ($1);
		$$ = node;}
	;

scalar_rhs : ID {$$ = new idNode ($1);}
	| TRUE {$$ = new datatypeNode<bool> ($1, BOOL);}
	| FALSE {$$ = new datatypeNode<bool> ($1, BOOL);}
	| T_INT {$$ = new datatypeNode<int> ($1, INT);}
	| T_DOUBLE {$$ = new datatypeNode<double> ($1, DOUBLE);}
	| T_FLOAT {$$ = new datatypeNode<float> ($1, FLOAT);}
	| scalar_rhs '|' scalar_rhs {$$ = new binaryNode (T_OR, $1, $3);} 
	| scalar_rhs '&' scalar_rhs {$$ = new binaryNode (T_AND, $1, $3);} 
	| scalar_rhs EQ scalar_rhs {$$ = new binaryNode (T_EQ, $1, $3);}
	| scalar_rhs NEQ scalar_rhs {$$ = new binaryNode (T_NEQ, $1, $3);}
	| scalar_rhs '<' scalar_rhs {$$ = new binaryNode (T_LT, $1, $3);}
	| scalar_rhs LEQ scalar_rhs {$$ = new binaryNode (T_LEQ, $1, $3);}
	| scalar_rhs '>' scalar_rhs {$$ = new binaryNode (T_GT, $1, $3);}
	| scalar_rhs GEQ scalar_rhs {$$ = new binaryNode (T_GEQ, $1, $3);}
	| scalar_rhs '+' scalar_rhs {$$ = new binaryNode (T_PLUS, $1, $3);}  
	| scalar_rhs '-' scalar_rhs {$$ = new binaryNode (T_MINUS, $1, $3);}
	| scalar_rhs '*' scalar_rhs {$$ = new binaryNode (T_MULT, $1, $3);}
	| scalar_rhs '/' scalar_rhs {$$ = new binaryNode (T_DIV, $1, $3);}
	| scalar_rhs '%' scalar_rhs {$$ = new binaryNode (T_MOD, $1, $3);}
	| '-' scalar_rhs %prec UMINUS {$$ = new uminusNode ($2);} 
	| scalar_rhs '^' scalar_rhs {$$ = new binaryNode (T_EXP, $1, $3);}
	| '(' scalar_rhs ')' {$2->set_nested (); 
			$$ = $2;}
	| ID '(' scalar_rhslist ')' {$$ = new functionNode ($1, $3);}

scalar_rhslist : scalar_rhslist ',' scalar_rhs {$1->push_back ($3);
		$$ = $1;}
	| scalar_rhs {std::vector<exprNode*> *node = new std::vector<exprNode*> ();
		node->push_back ($1);
		$$ = node;}
	;

/* Array access, of the form A[k+1][j+2][i+1] or A[k+1,j+2,i+1] */
offsetvar : ID '[' offsetlist ']' {$3->set_name ($1); 
				$$ = $3;}
	;

arrayaccess : ID {$$ = new idNode ($1);}
	| T_INT {$$ = new datatypeNode<int> ($1, INT);}
	| arrayaccess '+' arrayaccess {$$ = new binaryNode (T_PLUS, $1, $3);}  
	| arrayaccess '-' arrayaccess {$$ = new binaryNode (T_MINUS, $1, $3);}
	| arrayaccess '*' arrayaccess {$$ = new binaryNode (T_MULT, $1, $3);}
	| arrayaccess '/' arrayaccess {$$ = new binaryNode (T_DIV, $1, $3);}
	| '-' arrayaccess %prec UMINUS {$$ = new uminusNode ($2);}
	;

offsetlist : offsetlist ',' arrayaccess {$1->push_index ($3); 
					$$ = $1;} 
	| offsetlist ']''[' arrayaccess {$1->push_index ($4); 
					$$ = $1;} 
	| arrayaccess {shiftvecNode *node = new shiftvecNode ();
		node->push_index ($1); 
		$$ = node;}
	;

/* List of inputs and outputs */
allocinlist : ALLOCIN idlist ';' {grammar::start->push_allocin ($2);}
	| {}
	;

copyinlist : COPYIN idlist ';' {grammar::start->push_copyin ($2);}
	| {} 
	;

/* List of input that are to be put in constant memory */
constantlist : CONSTANT idlist ';' {grammar::start->push_constant ($2);}
	| {}
	;

copyoutlist : COPYOUT idlist ';' {grammar::start->push_copyout ($2);}
	| {} 
	;

arraylist : arraylist ',' ID '[' rangelist ']' {$1->push_back (new arrayDecl (std::string($3), *($5)));
						$$ = $1;}
	| ID '[' rangelist ']' {std::vector<arrayDecl*> *node = new std::vector<arrayDecl*> ();
				node->push_back (new arrayDecl (std::string($1), *($3)));
				$$ = node;}
	;

arglist : arglist ',' INOUT COLON ID {$1->push_back (std::string ($5));
			$1->push_inout (std::string ($5));
			$$ = $1;}
	| arglist ',' ID {$1->push_back (std::string ($3));
			$$ = $1;}
	| INOUT COLON ID {argList * node = new argList ();
		node->push_back (std::string ($3));
		node->push_inout (std::string ($3));
		$$ = node;}
	| ID {argList * node = new argList ();
		node->push_back (std::string ($1));
		$$ = node;}
	;

idlist : idlist ',' ID {$1->push_back (std::string ($3));
			$$ = $1;} 
	| ID {std::vector<std::string> * node = new std::vector<std::string> ();
		node->push_back (std::string ($1));
		$$ = node;}
	;

/* List of stencil calls */
stencilcalls : stencilcalls stencilcall {grammar::start->push_stencil_call ($2);}
	| stencilcall {grammar::start->push_stencil_call ($1);}
	| stencilloop {grammar::start->set_stencil_call_iterations($1);} '{' stencilcalls '}' {grammar::start->reset_stencil_call_iterations();} 
	;

stencilcall : ID '(' idlist ')' ';' {stencilCall *node = new stencilCall (std::string($1), *($3));
					$$ = node;}
	| '[' rangelist ']' COLON ID '(' idlist ')' ';' {stencilCall *node = new stencilCall (std::string($5), *($7), *($2));
					$$ = node;}
	;

stencilloop : ITERATE range {$$ = new Range (new datatypeNode<int> (0, INT), $2);}
	| ITERATE '(' range ')' {$$ = new Range (new datatypeNode<int> (0, INT), $3);}
	| ITERATE '[' range ']' {$$ = new Range (new datatypeNode<int> (0, INT), $3);}
	| ITERATE range COLON range {$$ = new Range ($2, $4);}
	| ITERATE '(' range COLON range ')' {$$ = new Range ($3, $5);}
	| ITERATE '[' range COLON range ']' {$$ = new Range ($3, $5);}

%%
