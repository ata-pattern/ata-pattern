#include "Filter.hpp"
#include "Node.hpp"
#include <iostream>

//Warning: this filter might make us miss the optimal solution (by terminating early)
class MaxCost : public Filter {
  private:
	int numFiltered = 0;
	int maxCost = 999999;

  public:
	Filter * createEmptyCopy() {
		MaxCost * f = new MaxCost();
		f->numFiltered = this->numFiltered;
		f->maxCost = this->maxCost;
		return f;
	}
	
	bool filter(Node * newNode) {
		if(newNode->cost <= maxCost) {
			return false;
		}
		
		numFiltered++;
		return true;
	}
	
	virtual void printStatistics(std::ostream & stream) {
		stream << "//MaxCost filtered " << numFiltered << " total nodes.\n";
	}
	
	virtual int setArgs(char** argv) {
		maxCost = atoi(argv[0]);
		
		return 1;
	}
	
	virtual int setArgs() {
		cin >> maxCost;
		
		return 1;
	}
};
