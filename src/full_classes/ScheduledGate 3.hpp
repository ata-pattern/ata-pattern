#ifndef SCHEDULEDGATE_HPP
#define SCHEDULEDGATE_HPP

#include "GateNode.hpp"
using namespace std;

class ScheduledGate {
  public:
	GateNode * gate;
	int cycle;//cycle when this gate started
	int physicalControl;
	int physicalTarget;
	
	int latency;
	
	ScheduledGate(GateNode * gate, int cycle) {
		this->gate = gate;
		this->cycle = cycle;
		this->physicalControl = -1;
		this->physicalTarget = -1;
		this->latency = 99999999;
	}
};

#endif
