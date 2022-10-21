#include "Filter.hpp"
#include "Node.hpp"
#include "ScheduledGate.hpp"
#include <iostream>

//Warning: this filter may be non-optimal
class ReqLPhase : public Filter {
  private:
	int numFiltered = 0;
	int cycle = -99999;
	int q1 = 0;
	int q2 = 0;

  public:
	Filter * createEmptyCopy() {
		ReqLPhase * f = new ReqLPhase();
		f->numFiltered = this->numFiltered;
		f->cycle = this->cycle;
		f->q1 = this->q1;
		f->q2 = this->q2;
		return f;
	}
	
	bool filter(Node * newNode) {
		if(newNode->cycle == cycle) {
			ScheduledGate * sg = newNode->lastNonSwapGate[q1];
			if(!sg || sg->cycle != cycle) {
				numFiltered++;
				return true;
			}
			if(sg->gate->control != q2 && sg->gate->target != q2) {
				numFiltered++;
				return true;
			}
		}
		
		return false;
	}
	
	virtual void printStatistics(std::ostream & stream) {
		stream << "//ReqLPhase filtered " << numFiltered << " total nodes.\n";
	}
	
	virtual int setArgs(char** argv) {
		cycle = atoi(argv[0]);
		q1 = atoi(argv[1]);
		q2 = atoi(argv[2]);
		
		return 3;
	}
	
	virtual int setArgs() {
		cin >> cycle;
		cin >> q1;
		cin >> q2;
		
		return 3;
	}
};
