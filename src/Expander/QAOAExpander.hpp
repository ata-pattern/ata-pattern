#ifndef QAOAEXPANDER_HPP
#define QAOAEXPANDER_HPP

#include <cassert>
#include <vector>
#include <iostream>
#include "Expander.hpp"
#include "Queue.hpp"
#include "CostFunc.hpp"

class QAOAExpander : public Expander {
  private:
	//return true iff inserting swap g in node's child would make a useless swap cycle
	bool isCyclic(Node * node, GateNode * g) {
		int target = g->target;
		int control = g->control;
		
		if(node->lastGate[target] && node->lastGate[control]) {
			LinkedStack<ScheduledGate*> * schdule = node->scheduled;
			while(schdule->size > 0) {
				if(schdule->value->gate == g) {
					return true;
				} else {
					int c = schdule->value->physicalControl;
					int t = schdule->value->physicalTarget;
					if(c >= 0) {
						if(c == control || c == target || t == control || t == target) {
							return false;
						}
					}
				}
				schdule = schdule->next;
			}
		}
		return false;
	}
  
  public:
	bool expand(Queue * nodes, Node * node) {
		//return false if we're done expanding
		if(nodes->getBestFinalNode() && node->cost >= nodes->getBestFinalNode()->cost) {
			return false;
		}
		
		unsigned int nodesSize = nodes->size();
		
		//Calculate number of unscheduled gates per qubit, so we know which qubits will still be used:
		int numGatesPerQubit[node->env->numPhysicalQubits];
		for(int x = 0; x < node->env->numPhysicalQubits; x++) {
			numGatesPerQubit[x] = 0;
		}
		for(auto iter = node->readyGates.begin(); iter != node->readyGates.end(); iter++) {
			GateNode * g = *iter;
			int target = (g->target < 0) ? -1 : node->laq[g->target];
			int control = (g->control < 0) ? -1 : node->laq[g->control];
			if(target > -1) {
				numGatesPerQubit[target]++;
			}
			if(control > -1) {
				numGatesPerQubit[control]++;
			}
		}
		
		//generate list of valid gates, based on ready list
		vector<GateNode*> possibleGates;//possible swaps and valid 2+ cycle gates
		vector<GateNode*> singleCycleGates;//valid 1 cycle non-swap gates
		int numDependentGates = 0;
		for(auto iter = node->readyGates.begin(); iter != node->readyGates.end(); iter++) {
			GateNode * g = *iter;
			int target = (g->target < 0) ? -1 : node->laq[g->target];
			int control = (g->control < 0) ? -1 : node->laq[g->control];
			
			bool good = node->cycle >= -1;
			bool dependsOnSomething = false;
			
			if(control >= 0) {//gate has a control qubit
				int busy = node->busyCycles(control);
				if(busy) {
					dependsOnSomething = true;
					if(busy > 1) {
						good = false;
					}
				}
			}
			
			if(g->target >= 0) {//gate has a target qubit
				int busy = node->busyCycles(target);
				if(busy) {
					dependsOnSomething = true;
					if(busy > 1) {
						good = false;
					}
				}
			}
			
			if(dependsOnSomething) {
				numDependentGates++;
			}
			
			if(good && node->cycle > 0 && nodesSize > 0 && !dependsOnSomething) {
				good = false;
			}
			
			if(good && control >= 0 && target >= 0) {//gate has 2 qubits
				if(node->env->couplings.count(make_pair(target,control)) <= 0) {
					if(node->env->couplings.count(make_pair(control,target)) <= 0) {
						good = false;
					}
				}
			}
			
			bool lastGateForItsBits = true;
			if(target >= 0 && numGatesPerQubit[target] > 1) {
				lastGateForItsBits = false;
			}
			if(control >= 0 && numGatesPerQubit[control] > 1) {
				lastGateForItsBits = false;
			}
			
			if(good) {
				int latency = node->env->latency->getLatency(g->name, (control >= 0 ? 2 : 1), target, control);
				if(latency == 1 && lastGateForItsBits) {
					singleCycleGates.push_back(g);
				} else {
					possibleGates.push_back(g);
				}
			}
		}
		//generate list of valid gates, based on possible swaps
		for(unsigned int x = 0; x < node->env->couplings.size(); x++) {
			GateNode * g = node->env->possibleSwaps[x];
			int target = g->target;//note: since g is swap, this is already a physical target
			int control = g->control;//note: since g is swap, this is already a physical control
			int logicalTarget = (target >= 0) ? node->qal[target] : -1;
			int logicalControl = (control >= 0) ? node->qal[control] : -1;
			
			bool good = true;
			bool dependsOnSomething = false;
			
			bool usesLogicalQubit = false;
			if(good && logicalTarget >= 0) {
				usesLogicalQubit = true;
			}
			if(good && logicalControl >= 0) {
				usesLogicalQubit = true;
			}
			good = good && usesLogicalQubit;
			
			//make sure this swap involves a qubit that has more gates ahead
			if(good && numGatesPerQubit[target] == 0 && numGatesPerQubit[control] == 0) {
				good = false;
			}
			
			int busyT = node->busyCycles(target);
			if(good && busyT) {
				dependsOnSomething = true;
				if(busyT > 1) {
					good = false;
				}
			}
			int busyC = node->busyCycles(control);
			if(good && busyC) {
				dependsOnSomething = true;
				if(busyC > 1) {
					good = false;
				}
			}
			if(good && node->cycle > 0 && nodesSize > 0 && !dependsOnSomething) {
				good = false;
			}
			if(good && isCyclic(node, g)) {
				good = false;
			}
			if(good) {
				numDependentGates++;
				possibleGates.push_back(g);
			}
		}
		if(nodesSize > 0 && !numDependentGates) {//this node can't lead to anything optimal
			//Reminder: this line caused problems when I tried placing it before looking at swaps
			return true;
		}
		
		assert(possibleGates.size() < 64); //or else I need to do this differently
		unsigned long long numIters = 1LL << possibleGates.size();
		for(unsigned long long x = 0; x < numIters; x++) {
			Node * child = node->prepChild();
			bool good = true;
			//Schedule a unique subset of {swaps and 2-qubit gates}:
			for(unsigned int y = 0; good && y < possibleGates.size(); y++) {
				if(x & (1LL << y)) {
					if(node->cycle >= -1) {
						good = good && child->scheduleGate(possibleGates[y]);
					} else {
						good = good && child->swapQubits(possibleGates[y]->target, possibleGates[y]->control);
					}
				}
			}
			
			if(!good) {
				delete child;
			} else {
				//Schedule as many of the 1-cycle ready gates as we can:
				for(unsigned int y = 0; good && y < singleCycleGates.size(); y++) {
					child->scheduleGate(singleCycleGates[y]);
				}
				
				int cycleMod = (child->cycle < 0) ? child->cycle : 0;
				child->cycle -= cycleMod;
				child->cost = node->env->cost->getCost(child);
				child->cycle += cycleMod;
				
				if(!nodes->push(child)) {
					delete child;
				}
			}
		}
		
		return true;
	}
};

#endif
