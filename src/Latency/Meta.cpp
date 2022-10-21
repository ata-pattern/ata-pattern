#ifndef META_LATENCY_CPP
#define META_LATENCY_CPP

#include "Meta.hpp"
#include "Latency.hpp"
#include "Latency_1.hpp"
#include "spec2.hpp"
#include <string>
#include <tuple>
using namespace std;

const int NUMLATENCIES = 2;
tuple<Latency*, string, string> latencies[NUMLATENCIES] = {
	make_tuple(new Latency_1(),
				"Latency_1",
				"every gate takes 1 cycle."),
	make_tuple(new spec2(),
				"spec2",
				"user specifies 2 params: C latency and S latency"),
};

#endif