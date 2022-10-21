#include "Filter.hpp"
#include "Node.hpp"
#include "LinkedStack.hpp"
#include "ScheduledGate.hpp"
#include <iostream>
#include <functional>
#include <unordered_map>
#include <vector>
#include <stack>
#include <bits/stdc++.h>

//WARNING: AT TIME OF WRITING, THIS FILTER DOESN'T WORK
//I still like the idea of a clique-specific filter, but I'm not sure what it should really look like.

#ifndef HASH_COMBINE_FUNCTION
#define HASH_COMBINE_FUNCTION
	//based on hash_combine from Boost libraries
	template <class T>
	inline void hash_combine(std::size_t& seed, const T& v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
	}
#endif

class CliqueFilter : public Filter {
  private:
	int numFiltered = 0;
	std::unordered_map<std::size_t, vector<Node*> > hashmap;
	
	//Print a node's scheduled gates
	//returns how many cycles the node takes to complete all its gates
	int printNode(std::ostream & stream, LinkedStack<ScheduledGate*> * gates) {
		int cycles = 0;
		std::stack<ScheduledGate*> gateStack;
		while(gates->size > 0) {
			gateStack.push(gates->value);
			gates = gates->next;
		}
		
		while(!gateStack.empty()) {
			ScheduledGate * sg = gateStack.top();
			gateStack.pop();
			int target = sg->physicalTarget;
			int control = sg->physicalControl;
			stream << sg->gate->name << " ";
			if(control >= 0) {
				stream << "q[" << control << "],";
			}
			stream << "q[" << target << "]";
			stream << ";";
			stream << " //cycle: " << sg->cycle;
			if(sg->gate->name.compare("swp") && sg->gate->name.compare("SWP")) {
				int target = sg->gate->target;
				int control = sg->gate->control;
				stream << " //" << sg->gate->name << " ";
				if(control >= 0) {
					stream << "q[" << control << "],";
				}
				stream << "q[" << target << "]";
			}
			stream << "\n";
			
			if(sg->cycle + sg->latency > cycles) {
				cycles = sg->cycle + sg->latency;
			}
		}
		
		return cycles;
	}

	
	inline std::size_t hashFunc(Node * n) {
		std::size_t hash_result = 0;
		int numQubits = n->env->numPhysicalQubits;
		
		//combine into hash: unassigned physical qubits
		for(int x = 0; x < numQubits; x++) {
			if(n->qal[x] < 0) {
				hash_combine(hash_result, x);
			}
		}
		
		//combine into hash: number of ready gates
		hash_combine(hash_result, n->readyGates.size());
		
		//combine into hash: cycle number
		hash_combine(hash_result, n->cycle);
		
		return hash_result;
	}	
	
  public:
	Filter * createEmptyCopy() {
		CliqueFilter * f = new CliqueFilter();
		f->numFiltered = this->numFiltered;
		return f;
	}
	
	void deleteRecord(Node * n) {
		std::size_t hash_result = hashFunc(n);
		vector<Node*> * mapValue = &this->hashmap[hash_result];//Note: I'm terrified of accidentally making an actual copy of the vector here
		for(unsigned int blah = 0; blah <  mapValue->size(); blah++) {
			Node * n2 = (*mapValue)[blah];
			if(n2 == n) {
				if(mapValue->size() > 1 && blah < mapValue->size() - 1) {
					std::swap((*mapValue)[blah],(*mapValue)[mapValue->size()-1]);
				}
				mapValue->pop_back();
				return;
			}
		}
	}
	
	bool filter(Node * newNode) {
		if(newNode->cycle > 0) return false;
		
		int numQubits = newNode->env->numPhysicalQubits;
		std::size_t hash_result = hashFunc(newNode);
		
		for(Node * candidate : this->hashmap[hash_result]) {
			//Node * candidate = findNode->second;
			bool willFilter = true;
			
			if(newNode->cycle != candidate->cycle) {//can't filter it
				willFilter = false;
			}
			
			for(int x = 0; willFilter && x < numQubits - 1; x++) {
				if((candidate->qal[x] < 0) != (newNode->qal[x] < 0)) {
					//std::cerr << "Warning: duplicate hash values.\n";
					willFilter = false;
					break;
				}
			}
			
			for(int x = 0; willFilter && x < numQubits; x++) {
				int newBusy = newNode->busyCycles(x) + newNode->cycle;
				int canBusy = candidate->busyCycles(x) + candidate->cycle;
				if(newBusy < 2) newBusy = 0;
				if(canBusy < 2) canBusy = 0;
				if(newBusy != canBusy) {
					willFilter = false;
					break;
				}
			}
			
			if(newNode->readyGates.size() != candidate->readyGates.size()) {
				willFilter = false;
			} else if(willFilter) {
				int counts1[newNode->env->numPhysicalQubits];
				int counts2[newNode->env->numPhysicalQubits];
				for(int x = 0; x < newNode->env->numPhysicalQubits; x++) {
					counts1[x] = 0;
					counts2[x] = 0;
				}
				
				for(auto x = candidate->readyGates.begin(); x != candidate->readyGates.end(); x++) {
					GateNode * g = *x;
					counts1[g->target]++;
					counts1[g->control]++;
				}
				for(auto x = newNode->readyGates.begin(); x != newNode->readyGates.end(); x++) {
					GateNode * g = *x;
					counts2[g->target]++;
					counts2[g->control]++;
				}
				
				sort(counts1, counts1 + newNode->env->numPhysicalQubits);
				sort(counts2, counts2 + newNode->env->numPhysicalQubits);
				for(int x = 0; x < newNode->env->numPhysicalQubits; x++) {
					if(counts1[x] != counts2[x]) {
						willFilter = false;
					}
				}
			}
			
			if(willFilter && newNode->cycle >= candidate->cycle) {
				/*std::cerr<< "//filter out following node @cycle " << newNode->cycle << " :\n";
				printNode(cerr, newNode->scheduled);
				std::cerr<< "//based on following node @cycle " << candidate->cycle << " :\n";
				printNode(cerr, candidate->scheduled);
				std::cerr << "\n";
				*/
				
				numFiltered++;
				return true;
			}
		}
		this->hashmap[hash_result].push_back(newNode);
		
		return false;
	}
	
	virtual void printStatistics(std::ostream & stream) {
		stream << "//CliqueFilter filtered " << numFiltered << " total nodes.\n";
	}
};
