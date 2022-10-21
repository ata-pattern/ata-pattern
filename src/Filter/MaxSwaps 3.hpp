#include "Filter.hpp"
#include "Node.hpp"
#include <iostream>

//Warning: this filter might make us miss the optimal solution (by terminating early?)
class MaxSwaps : public Filter {
  private:
	int numFiltered = 0;
	int maxSwaps = 999999;

  public:
	Filter * createEmptyCopy() {
		MaxSwaps * f = new MaxSwaps();
		f->numFiltered = this->numFiltered;
		f->maxSwaps = this->maxSwaps;
		return f;
	}
	
	bool filter(Node * newNode) {
		//calculate number of swaps that have already been scheduled:
		int runningTotal = newNode->scheduled->size + newNode->numUnscheduledGates;
		int numOriginal = newNode->env->numGates;
		int currentswaps = runningTotal - numOriginal;
		
		//calculate lower bound on how many more swapas we'll need:
		int addlswaps = 0;
		auto iter = newNode->readyGates.begin();
		while(iter != newNode->readyGates.end()) {
			GateNode * g = *iter;
			
			//calculate min swaps needed to make this particular gate executable
			if(g->target >= 0 && g->control >= 0) {
				int physicalControl = newNode->laq[g->control];
				int physicalTarget = newNode->laq[g->target];
				int minSwaps = newNode->env->couplingDistances[physicalControl*newNode->env->numPhysicalQubits + physicalTarget] - 1;
				if(addlswaps < minSwaps) {
					addlswaps = minSwaps;
				}
			}
			
			iter++;
		}
		
		if(currentswaps + addlswaps <= maxSwaps) {
			return false;
		}
		
		numFiltered++;
		return true;
	}
	
	virtual void printStatistics(std::ostream & stream) {
		stream << "//MaxSwaps filtered " << numFiltered << " total nodes.\n";
	}
	
	virtual int setArgs(char** argv) {
		maxSwaps = atoi(argv[0]);
		
		return 1;
	}
	
	virtual int setArgs() {
		cin >> maxSwaps;
		
		return 1;
	}
};
