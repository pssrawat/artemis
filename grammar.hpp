#ifndef __GRAMMAR_HPP__
#define __GRAMMAR_HPP__
#include <stdio.h>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include "funcdefn.hpp"

class grammar {
	public:
		static startNode *start;
		static void set_input (FILE *);
		static void parse ();
};

#endif
