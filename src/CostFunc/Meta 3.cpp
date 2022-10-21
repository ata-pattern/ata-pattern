#ifndef META_COSTFUNC_CPP
#define META_COSTFUNC_CPP

#include "Meta.hpp"
#include "CostFunc.hpp"
#include "qaoa.hpp"
#include "qaoa2.hpp"
#include <string>
#include <tuple>
using namespace std;

const int NUMCOSTFUNCTIONS = 2;
tuple<CostFunc*, string, string> costFunctions[NUMCOSTFUNCTIONS] = {
	make_tuple(new CostQAOA(),
				"QAOA",
				"Cost function which assumes gates have no dependencies"),
	make_tuple(new CostQAOA2(),
				"QAOA2",
				"Cost function which assumes gates have no dependencies"),
};

#endif