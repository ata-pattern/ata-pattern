#ifndef topk_expander_hpp
#define topk_expander_hpp

#include <cassert>
#include <vector>
#include <iostream>
#include <queue>
#include "Expander.hpp"
#include "Queue.hpp"
#include "CostFunc.hpp"

class topk : public Expander {
  private:
	int K = 0;
	
	struct CmpNodeCostR
	{
		//Reminder: I reversed cost function here so I can delete extra nodes as soon as priority queue is overfilled
		bool operator()(const Node* lhs, const Node* rhs) const
		{
			//tiebreaker:
			if(lhs->cost == rhs->cost) {
				return lhs->cost2 <= rhs->cost2;
			}
			
			//lower cost is better
			return lhs->cost < rhs->cost;
		}
	};
	
	void genChildren(Node * parent, std::priority_queue<Node*, std::vector<Node*>, CmpNodeCostR> & nodes, vector<GateNode*> * gates, int q, vector<GateNode*> & subset, bool * used) {
		Environment * env = parent->env;
		int phys = env->numPhysicalQubits;
		if(q == phys) {
			Node * child = parent->prepChild();
			for(unsigned int y = 0; y < subset.size(); y++) {
				if(parent->cycle >= -1) {
					if(!child->scheduleGate(subset[y])) {
						std::cerr << "SANITY CHECK ERROR q2~45\n";
					}
				} else {
					if(!child->swapQubits(subset[y]->target, subset[y]->control)) {
						std::cerr << "SANITY CHECK ERROR q2~50\n";
					}
				}
			}
			
			int cycleMod = (child->cycle < 0) ? child->cycle : 0;
			child->cycle -= cycleMod;
			child->cost = env->cost->getCost(child);
			child->cycle += cycleMod;
			
			nodes.push(child);
			if((signed)nodes.size() > this->K) {
				Node * worstNode = nodes.top();
				nodes.pop();
				delete worstNode;
			}
			
			return;
		} else if(used[q]) {
			genChildren(parent, nodes, gates, q+1, subset, used);
			return;
		}
		
		genChildren(parent, nodes, gates, q+1, subset, used);
		
		for(unsigned int x = 0; x < gates[q].size(); x++) {
			GateNode * g = gates[q][x];
			int target = (g->target < 0) ? -1 : parent->laq[g->target];
			int control = (g->control < 0) ? -1 : parent->laq[g->control];
			if(!g->name.compare("swp") || !g->name.compare("SWP")) {
				target = g->target;
				control = g->control;
			}
			
			if(!used[control] && !used[target]) {
				used[target] = true;
				used[control] = true;
				subset.push_back(g);
				genChildren(parent, nodes, gates, q+1, subset, used);
				subset.pop_back();
				used[control] = false;
				used[target] = false;
			}
		}
	}
  
  public:
	int setArgs(char** argv) {
		K = atoi(argv[0]);
		return 1;
	}
	
	int setArgs() {
		std::cerr << "Enter value of K for top-k expander: K=";
		std::cin >> K;
		return 1;
	}
	
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
		
		vector<GateNode*> bins[node->env->numPhysicalQubits];
		
		//generate list of valid gates, based on ready list
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
				//good = false;
			}
			
			if(good && control >= 0 && target >= 0) {//gate has 2 qubits
				if(node->env->couplings.count(make_pair(target,control)) <= 0) {
					if(node->env->couplings.count(make_pair(control,target)) <= 0) {
						good = false;
					}
				}
			}
			
			/*
			bool lastGateForItsBits = true;
			if(target >= 0 && numGatesPerQubit[target] > 1) {
				lastGateForItsBits = false;
			}
			if(control >= 0 && numGatesPerQubit[control] > 1) {
				lastGateForItsBits = false;
			}
			*/
			
			if(good) {
				//int latency = node->env->latency->getLatency(g->name, (control >= 0 ? 2 : 1), target, control);
				if(target < control) {
					bins[target].push_back(g);
				} else {
					bins[control].push_back(g);
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
			if(good) {
				numDependentGates++;
				if(target < control) {
					bins[target].push_back(g);
				} else {
					bins[control].push_back(g);
				}
			}
		}
		
		//Generate children nodes:
		std::priority_queue<Node*, std::vector<Node*>, CmpNodeCostR> tempNodes;
		vector<GateNode*> subset;
		bool used[node->env->numPhysicalQubits];
		for(int x = 0; x < node->env->numPhysicalQubits; x++) {
			used[x] = false;
		}
		genChildren(node, tempNodes, bins, 0, subset, used);
		
		//Push top K into main priority queue
		int counter = this->K;
		while(counter > 0 && tempNodes.size() > 0) {
			Node * child = tempNodes.top();
			tempNodes.pop();
			if(nodes->push(child)) {
				counter--;
			} else {
				std::cerr << "SANITY CHECK ERROR: i guess not all pushed nodes will go through for top-k after all; therefore I need to redo the optimization handling the overfill\n";
				delete child;
			}
		}
		
		//cleanup the discarded children
		while(tempNodes.size() > 0) {
			Node * child = tempNodes.top();
			tempNodes.pop();
			delete child;
		}		
		
		return true;
	}
};

#endif
