#ifndef LATENCY_SPEC2_HPP
#define LATENCY_SPEC2_HPP

#include "Latency.hpp"
#include <cstring>
#include <iostream>
#include <cassert>
using namespace std;

/**
 * Allows user to SPECify latencies.
 * Two arguments: cphase latency, swap latency
 */
class spec2 : public Latency {
	int latC = -1;
	int latS = -1;
	
  public:
	int getLatency(string gateName, int numQubits, int target, int control) {
		if(numQubits != 2) {
			assert(false && "Invalid gate. (Error detected in Latency module.)\n");
		}
		if(latC < 0 || latS < 0) {
			assert(false && "Invalid latency specifications.\n");
		}
		
		if(!gateName.compare("swp") || !gateName.compare("SWP")) {
			return latS;
		} else {
			return latC;
		}
	}
	
	int setArgs(char** argv) {
		latC = atoi(argv[0]);
		latS = atoi(argv[1]);
		
		return 2;
	}
	
	int setArgs() {
		std::cin >> latC;
		std::cin >> latS;
		
		return 2;
	}
};

#endif
