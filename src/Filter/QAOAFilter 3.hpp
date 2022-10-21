#include "Filter.hpp"
#include "Node.hpp"
#include "LinkedStack.hpp"
#include "ScheduledGate.hpp"
#include <iostream>
#include <functional>
#include <unordered_map>
#include <vector>
#include <stack>

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

class QAOAFilter : public Filter {
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
		
		//combine into hash: qubit map (array of integers)
		for(int x = 0; x < numQubits; x++) {
			hash_combine(hash_result, n->laq[x]);
		}
		
		//combine into hash: ready gate (set of pointers)
		for(auto x = n->readyGates.begin(); x != n->readyGates.end(); x++) {
			hash_combine(hash_result, (std::uintptr_t) (*x));
		}
		
		return hash_result;
	}	
	
  public:
	Filter * createEmptyCopy() {
		QAOAFilter * f = new QAOAFilter();
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
		int numQubits = newNode->env->numPhysicalQubits;
		std::size_t hash_result = hashFunc(newNode);
		
		for(Node * candidate : this->hashmap[hash_result]) {
			//Node * candidate = findNode->second;
			bool willFilter = true;
			
			if(newNode->cycle < candidate->cycle) {//can't filter it
				willFilter = false;
			}
			if(newNode->cycle > candidate->cycle + 1) {
				willFilter = false;//otherwise I need a more complicated check to see if they're directly descended
			}
			if(newNode->parent == candidate) {//bad comparison; can't filter it
				willFilter = false;
			}
			
			for(int x = 0; willFilter && x < numQubits - 1; x++) {
				if(candidate->laq[x] != newNode->laq[x]) {
					//std::cerr << "Warning: duplicate hash values.\n";
					willFilter = false;
					break;
				}
			}
			if(newNode->readyGates.size() < candidate->readyGates.size()) {
				willFilter = false;
			} else if(willFilter) {
				for(auto x = candidate->readyGates.begin(); x != candidate->readyGates.end(); x++) {
					if(newNode->readyGates.count(*x) == 0) {//newNode has already scheduled a gate that the candidate superior node has not; therefore candidate is not actually superior
						//std::cerr<<"\nhey!!!\n";
						willFilter = false;
						break;
					}
				}
			}
			
			for(int x = 0; willFilter && x < numQubits; x++) {
				int newBusy = newNode->busyCycles(x) + newNode->cycle;
				int canBusy = candidate->busyCycles(x) + candidate->cycle;
				if(newBusy < canBusy) {
					willFilter = false;
					break;
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
		stream << "//QAOAFilter filtered " << numFiltered << " total nodes.\n";
	}
};
