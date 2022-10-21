#include "Filter.hpp"
#include "Node.hpp"
#include "ScheduledGate.hpp"
#include <iostream>

//Warning: this filter may be non-optimal
class OnlySwapsOn : public Filter {
  private:
	int numFiltered = 0;
	int cycle = -99999;

  public:
	Filter * createEmptyCopy() {
		OnlySwapsOn * f = new OnlySwapsOn();
		f->numFiltered = this->numFiltered;
		f->cycle = this->cycle;
		return f;
	}
	
	bool filter(Node * newNode) {
		if(newNode->cycle == cycle) {
			for(int q = 0; q < newNode->env->numPhysicalQubits; q++) {
				ScheduledGate * sg = newNode->lastGate[q];
				if(!sg || sg->cycle != cycle) {
					continue;
				}
				if(sg->gate->name.compare("swp") && sg->gate->name.compare("SWP")) {
					numFiltered++;
					return true;
				}
			}
		}
		
		return false;
	}
	
	virtual void printStatistics(std::ostream & stream) {
		stream << "//OnlySwapsOn" << cycle << " filtered " << numFiltered << " total nodes.\n";
	}
	
	virtual int setArgs(char** argv) {
		cycle = atoi(argv[0]);
		
		return 1;
	}
	
	virtual int setArgs() {
		cin >> cycle;
		
		return 1;
	}
};
