#ifndef META_FILTERS_CPP
#define META_FILTERS_CPP

#include "Meta.hpp"
#include "Filter.hpp"
#include "QAOAFilter.hpp"
//#include "CliqueFilter.hpp"
#include "AlternatingFilter.hpp"
#include "MinGatesPerCycle.hpp"
#include "FullCycles.hpp"
#include "Unidirectional.hpp"
#include "MaxCost.hpp"
#include "MaxSwaps.hpp"
#include "SkipSol.hpp"
#include "NoSwapsOn.hpp"
#include "OnlySwapsOn.hpp"
#include "ReqLPhase.hpp"
#include <string>
#include <tuple>
using namespace std;

const int NUMFILTERS = 11;
tuple<Filter*, string, string> FILTERS[NUMFILTERS] = {
	make_tuple(new QAOAFilter(),
				"qaoa",
				"Filter specialized for case where there are no dependencies between gates."),
	//make_tuple(new CliqueFilter(),
	//			"clique",
	//			"Filter specialized for qaoa clique (probably non-optimal!)."),
	make_tuple(new AlternatingFilter(),
				"AlternatingFilter",
				"disallows swaps and non-swaps starting in same cycle (non-optimal!)"),
	make_tuple(new MaxCost(),
				"maxCost",
				"disallows nodes with cost > N (non-optimal, technically)"),
	make_tuple(new MaxSwaps(),
				"maxswaps",
				"disallows nodes with total swapas > N (non-optimal, technically)"),
	make_tuple(new MinGatesPerCycle(),
				"MinGatesPerCycle",
				"disallows nodes with a cycle that schedules < N gates (non-optimal!)"),
	make_tuple(new FullCycles(),
				"FullCycles",
				"disallows nodes with a cycle that has unused edges (non-optimal!)"),
	make_tuple(new Unidirectional(),
				"Unidirectional",
				"disallows nodes with a cycle whose edges don't go in the same direction -- designed for 2xN architecture on the assumption vertical edges have a consistent difference in qubit ID and horizontal edges gave a consistent difference in qubit ID. (non-optimal!)"),
	make_tuple(new SkipSol(),
				"SkipSol",
				"skips the first N completely-scheduled circuits, thus printing an alternate solution (non-optimal!)"),
	make_tuple(new NoSwapsOn(),
				"noswapson",
				"rejects nodes that schedule a swap on cycle [k] (non-optimal!)"),
	make_tuple(new OnlySwapsOn(),
				"onlyswapson",
				"rejects nodes that schedule a non-swap gate on cycle [k] (non-optimal!)"),
	make_tuple(new ReqLPhase(),
				"ReqLPhase",
				"if node is at cycle [k] and does not contain fresh phase gate with Logical qubits [i,j], filter it out"),
	
};

#endif