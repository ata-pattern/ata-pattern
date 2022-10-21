#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

class GateNode;
class CostFunc;
#include "Latency.hpp"
#include "Filter.hpp"
#include "NodeMod.hpp"
#include <set>
#include <vector>
#include <cassert>
#include <cstring>
using namespace std;

class Environment {//for data shared across all nodes
  public:
	//Important variables for outputting an OPENQASM file:
	char * QASM_version = NULL;//string representation of OPENQASM version number (i.e. "2.0")
	vector<char*> includes;//list of include statements we need to reproduce in output
	vector<char*> customGates;//list of gate definitions we need to reproduce in output
	vector<char*> opaqueGates;//list of opaque gate definitions we need to reproduce in output
	vector<std::pair<int, int> > measures;//list of measurement gates; first is qbit, second is cbit
	
	vector<NodeMod*> nodeMods;
	vector<Filter*> filters;
	CostFunc * cost;//contains function to calculate a node's cost
	Latency * latency;//contains function to calculate a gate's latency
	
	set<pair<int, int> > couplings; //the coupling map (as a list of qubit-pairs)
	GateNode ** possibleSwaps; //list of swaps implied by the coupling map
	int * couplingDistances;//array of size (numPhysicalQubits*numPhysicalQubits), containing the minimal number of hops between each pair of qubits in the coupling graph
	
	int numLogicalQubits;//number of logical qubits in circuit; if there's a gap then this includes unused qubits
	int numPhysicalQubits;//number of physical qubits in the coupling map
	int swapCost; //best possible swap cost; this should be set by main using the latency function
	int numGates; //the number of gates in the original circuit
	
	GateNode ** firstCXPerQubit = 0;//the first 2-qubit gate that uses each logical qubit
	
	//necessary info for mapping original qubit IDs to flat array (and back again, if necessary)
	vector<char*> qregName;
	vector<int> qregSize;
	
	//necessary info for mapping original cbit IDs to flat array (and back again, if necessary)
	vector<char*> cregName;
	vector<int> cregSize;
	
	///Gives the flat-array index of the first bit in the specified qreg
	int getQregOffset(char * name) {
		int offset = 0;
		for(unsigned int x = 0; x < qregName.size(); x++) {
			if(!std::strcmp(name, qregName[x])) {
				return offset;
			} else {
				offset += qregSize[x];
			}
		}
		
		std::cerr << "FATAL ERROR: couldn't recognize qreg name " << name << "\n";
		
		assert(false);
		return -1;
	}
	
	///Gives the flat-array index of the first bit in the specified creg
	int getCregOffset(char * name) {
		int offset = 0;
		for(unsigned int x = 0; x < cregName.size(); x++) {
			if(!std::strcmp(name, cregName[x])) {
				return offset;
			} else {
				offset += cregSize[x];
			}
		}
		
		assert(false);
		return -1;
	}
	
	///Invoke all node mods, using the specified node and specified flag
	void runNodeModifiers(Node * node, int flag) {
		for(unsigned int x = 0; x < this->nodeMods.size(); x++) {
			this->nodeMods[x]->mod(node, flag);
		}
	}
	
	///Invoke the active filters; returns true if we should delete the node.
	bool filter(Node * newNode) {
		for(unsigned int x = 0; x < this->filters.size(); x++) {
			if(this->filters[x]->filter(newNode)) {
				for(unsigned int y = 0; y < x; y++) {
					this->filters[y]->deleteRecord(newNode);
				}
				return true;
			}
		}
		
		return false;
	}
	
	///Instructs all active filters to delete all pointers to the specified node.
	void deleteRecord(Node * oldNode) {
		for(unsigned int x = 0; x < this->filters.size(); x++) {
			this->filters[x]->deleteRecord(oldNode);
		}
	}
	
	///Recreates the filters, forcibly erasing any data they've gathered.
	void resetFilters() {
		for(unsigned int x = 0; x < this->filters.size(); x++) {
			Filter * old = this->filters[x];
			this->filters[x] = old->createEmptyCopy();
			delete old;
		}
	}
	
	///Invokes the printStatistics function for every active filter.
	void printFilterStats(std::ostream & stream) {
		for(unsigned int x = 0; x < this->filters.size(); x++) {
			this->filters[x]->printStatistics(stream);
		}
	}
};

#endif
