#include "Filter.hpp"
#include "Node.hpp"
#include <iostream>

//Warning: this filter might make us miss the optimal solution
class AlternatingFilter : public Filter {
  private:
	int numFiltered = 0;

  public:
	Filter * createEmptyCopy() {
		AlternatingFilter * f = new AlternatingFilter();
		f->numFiltered = this->numFiltered;
		return f;
	}
	
	bool filter(Node * newNode) {
		bool justScheduledSwaps = false;
		bool justScheduledNonSwaps = false;
		for(int x = 0; x < newNode->env->numPhysicalQubits; x++) {
			if(newNode->lastGate[x]) {
				bool isSwap = !newNode->lastGate[x]->gate->name.compare("swp") || !newNode->lastGate[x]->gate->name.compare("SWP");
				if(newNode->lastGate[x]->cycle == newNode->cycle) {
					if(isSwap) {
						justScheduledSwaps = true;
						if(justScheduledNonSwaps) {
							numFiltered++;
							return true;
						}
					} else {
						justScheduledNonSwaps = true;
						if(justScheduledSwaps) {
							numFiltered++;
							return true;
						}
					}
				}
			}
		}
		
		return false;
	}
	
	virtual void printStatistics(std::ostream & stream) {
		stream << "//AlternatingFilter filtered " << numFiltered << " total nodes.\n";
	}
};
