#include "Filter.hpp"
#include "Node.hpp"
#include <iostream>

//Warning: this filter will skip (optimal) solutions
class SkipSol : public Filter {
  private:
	int numFiltered = 0;
	int skip = 0;

  public:
	Filter * createEmptyCopy() {
		SkipSol * f = new SkipSol();
		f->numFiltered = this->numFiltered;
		f->skip = this->skip;
		return f;
	}
	
	bool filter(Node * newNode) {
		if(newNode->numUnscheduledGates == 0) {
			if(skip) {
				skip--;
				numFiltered++;
				return true;
			}
		}
		
		return false;
	}
	
	virtual void printStatistics(std::ostream & stream) {
		stream << "//SkipSol filtered " << numFiltered << " total nodes.\n";
	}
	
	virtual int setArgs(char** argv) {
		skip = atoi(argv[0]);
		
		return 1;
	}
	
	virtual int setArgs() {
		cin >> skip;
		
		return 1;
	}
};
