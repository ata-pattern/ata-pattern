#include "Filter.hpp"
#include "Node.hpp"
#include <iostream>

//Warning: this filter might make us miss the optimal solution
class MinGatesPerCycle : public Filter {
  private:
	int numFiltered = 0;
	int minGates = 1;

  public:
	Filter * createEmptyCopy() {
		MinGatesPerCycle * f = new MinGatesPerCycle();
		f->numFiltered = this->numFiltered;
		f->minGates = this->minGates;
		return f;
	}
	
	bool filter(Node * newNode) {
		//don't filter out a complete node, even if final cycle has too few gates:
		if(newNode->numUnscheduledGates == 0) {
			return false;
		}
		
		//don't filter out root nodes:
		if(newNode->cycle < 0) {
			return false;
		}
		
		int justScheduledGates = 0;
		for(int x = 0; x < newNode->env->numPhysicalQubits; x++) {
			if(newNode->lastGate[x]) {
				if(newNode->lastGate[x]->cycle == newNode->cycle) {
					if(newNode->lastGate[x]->physicalTarget == x) {
						justScheduledGates++;
						if(justScheduledGates >= minGates) {
							return false;
						}
					}
				}
			}
		}
		
		numFiltered++;
		return true;
	}
	
	virtual void printStatistics(std::ostream & stream) {
		stream << "//MinGates filtered " << numFiltered << " total nodes.\n";
	}
	
	virtual int setArgs(char** argv) {
		minGates = atoi(argv[0]);
		
		return 1;
	}
	
	virtual int setArgs() {
		cin >> minGates;
		
		return 1;
	}
};
