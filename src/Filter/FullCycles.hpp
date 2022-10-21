#include "Filter.hpp"
#include "Node.hpp"
#include <iostream>

//Warning: this filter might make us miss the optimal solution
class FullCycles : public Filter {
  private:
	int numFiltered = 0;

  public:
	Filter * createEmptyCopy() {
		FullCycles * f = new FullCycles();
		f->numFiltered = this->numFiltered;
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
		
		for(int x = 0; x < newNode->env->numPhysicalQubits - 1; x++) {
			if(newNode->lastGate[x]) {
				if(newNode->lastGate[x]->cycle == newNode->cycle) {
					continue;
				}
			}
			for(int y = x + 1; y < newNode->env->numPhysicalQubits; y++) {
				if(newNode->lastGate[y]) {
					if(newNode->lastGate[y]->cycle == newNode->cycle) {
						continue;
					}
				}
				
				if(newNode->env->couplings.count(make_pair(x,y)) > 0) {
					numFiltered++;
					return true;
				} else if(newNode->env->couplings.count(make_pair(y,x)) > 0) {
					numFiltered++;
					return true;
				}
			}
		}
		
		return false;
	}
	
	virtual void printStatistics(std::ostream & stream) {
		stream << "//FullCycles filtered " << numFiltered << " total nodes.\n";
	}
};
