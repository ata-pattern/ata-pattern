#ifndef META_EXPANDERS_CPP
#define META_EXPANDERS_CPP

#include "Meta.hpp"
#include "Expander.hpp"
#include "QAOAExpander.hpp"
#include "qaoa2.hpp"
#include "topk.hpp"
#include <string>
#include <tuple>
using namespace std;

const int NUMEXPANDERS = 3;
tuple<Expander*, string, string> expanders[NUMEXPANDERS] = {
	make_tuple(new QAOAExpander(),
				"qaoa",
				"Expander designed for special case where there are no gate dependencies."),
	make_tuple(new qaoaexpander2(),
				"qaoa2",
				"A more efficient expander, I hope."),
	make_tuple(new topk(),
				"topk",
				"A top k expander; takes value for 'k' as a parameter."),
	
};

#endif