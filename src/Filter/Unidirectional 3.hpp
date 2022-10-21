#include "Filter.hpp"
#include "Node.hpp"
#include <iostream>

//Warning: this filter might make us miss the optimal solution
class Unidirectional : public Filter {
  private:
	int numFiltered = 0;

  public:
	Filter * createEmptyCopy() {
		Unidirectional * f = new Unidirectional();
		f->numFiltered = this->numFiltered;
		return f;
	}
	
	bool filter(Node * newNode) {
		//don't waste time looking at root nodes:
		if(newNode->cycle < 0) {
			return false;
		}
		
		int direction = -1;
		
		for(int x = 0; x < newNode->env->numPhysicalQubits - 1; x++) {
			if(newNode->lastGate[x]) {
				if(newNode->lastGate[x]->cycle != newNode->cycle) {
					continue;
				}
				
				//get this gate's physical qubits:
				int i = newNode->lastGate[x]->physicalControl;
				int j = newNode->lastGate[x]->physicalTarget;
				if(i > j) {
					std::swap(i, j);
				}
				
				//if it doesn't use both qubits, skip
				if(i < 0) {
					continue;
				}
				
				int newDirection = j - i;
				if(direction < 0) {
					direction = newDirection;
				} else if (direction != newDirection) {
					numFiltered++;
					return true;
				}
			}
		}
		
		return false;
	}
	
	virtual void printStatistics(std::ostream & stream) {
		stream << "//Unidirectional filtered " << numFiltered << " total nodes.\n";
	}
};
