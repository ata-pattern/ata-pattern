#include "NodeMod.hpp"
#include "Node.hpp"
#include <iostream>

class OptMapper : public NodeMod {
  private:
	bool findPerfectMap(GateNode ** cxs, Node * node) {
		Environment * env = node->env;
		GateNode ** possibleSwaps = env->possibleSwaps;
		int numQubits = env->numPhysicalQubits;
		auto * laq = node->laq;
		auto * qal = node->qal;
		
		for(int z = 0; z < numQubits; z++) {
			GateNode * currentGate = cxs[z];
			if(currentGate) {
				int physC = laq[currentGate->control];
				int physT = laq[currentGate->target];
				bool iteratorIsControl = (z == currentGate->control);
				if(iteratorIsControl) {
					cxs[z] = currentGate->nextControlCNOT;
				} else {
					cxs[z] = currentGate->nextTargetCNOT;
				}
				
				if(physC < 0 && physT < 0) {
					for(int x = 0; x < env->numPhysicalQubits - 1; x++) {
						if(qal[x] < 0) {
							for(int y = x + 1; y < env->numPhysicalQubits; y++) {
								if(qal[y] < 0) {
									int dist = env->couplingDistances[x * env->numPhysicalQubits + y];
									if(dist == 1) {
										//try one way
										node->laq[currentGate->control] = x;
										node->laq[currentGate->target] = y;
										node->qal[x] = currentGate->control;
										node->qal[y] = currentGate->target;
										bool good = findPerfectMap(cxs, node);
										if(good) {
											return good;
										}
										
										//try the other way
										node->laq[currentGate->control] = y;
										node->laq[currentGate->target] = x;
										node->qal[y] = currentGate->control;
										node->qal[x] = currentGate->target;
										good = findPerfectMap(cxs, node);
										if(good) {
											return good;
										}
										
										//reset on failure
										node->laq[currentGate->control] = -1;
										node->laq[currentGate->target] = -1;
										node->qal[y] = -1;
										node->qal[x] = -1;
									}
								}
							}
						}
					}
				} else if(physC < 0) {
					for(int x = 0; x < env->numPhysicalQubits - 1; x++) {
						if(x == currentGate->target) {
							continue;
						}
						if(node->qal[x] < 0) {
							int dist = env->couplingDistances[x * env->numPhysicalQubits + physT];
							if(dist == 1) {
								node->laq[currentGate->control] = x;
								node->qal[x] = currentGate->control;
								bool good = findPerfectMap(cxs, node);
								if(good) {
									return good;
								}
								
								//reset on failure
								node->laq[currentGate->control] = -1;
								node->qal[x] = -1;
							}
						}
					}
				} else if(physT < 0) {
					for(int x = 0; x < env->numPhysicalQubits - 1; x++) {
						if(x == currentGate->control) {
							continue;
						}
						if(node->qal[x] < 0) {
							int dist = env->couplingDistances[x * env->numPhysicalQubits + physT];
							if(dist == 1) {
								node->laq[currentGate->target] = x;
								node->qal[x] = currentGate->target;
								bool good = findPerfectMap(cxs, node);
								if(good) {
									return good;
								}
								
								//reset on failure
								node->laq[currentGate->target] = -1;
								node->qal[x] = -1;
							}
						}
					}
				}
				
				//this path failed; let's reset the gate in the CX array
				cxs[z] = currentGate;
			}
		}
		
		return false;
	}
	
	bool findPerfectMap2(GateNode ** cxs, Node * node) {
		Environment * env = node->env;
		GateNode ** possibleSwaps = env->possibleSwaps;
		int numPossibleSwaps = env->couplings.size();
		int numPhysical = env->numPhysicalQubits;
		int numLogical = env->numLogicalQubits;
		auto * laq = node->laq;
		auto * qal = node->qal;
		
		int nextSwapToTry[numPhysical];
		nextSwapToTry[0] = 0;
		bool flip[numPhysical];
		flip[0] = false;
		int bit = 0;
		
		bool triedBit[numLogical*numLogical];
		for(int x = 0; x < numLogical; x++) {
			triedBit[x] = 0;
		}
		
		GateNode * cxs2[numPhysical];
		
		bool good = false;
		bool nomore = false;
		while(!good) {
			good = true;
			
			//reset mapping for current bit and later bits
			for(int b = bit; b < numPhysical; b++) {
				int q = laq[b];
				if(q >= 0) {
					qal[q] = -1;
					laq[b] = -1;
				}
				if(b > bit) {
					nextSwapToTry[b] = 0;
					flip[b] = false;
				}
				if(b > bit && b < numLogical) {
					for(int c = 0; c < numLogical; c++) {
						triedBit[b * numLogical + c] = false;
					}
				}
			}
			
			//find sane mapping based on couplings and initial CXs
			for(int b = bit; b < numLogical; b++) {
				GateNode * swp = possibleSwaps[nextSwapToTry[b]];
				//std::cerr << "trying " << b << " : " << nextSwapToTry[b] << " : " << flip[b] << "\n";
				int pc = swp->control;
				int pt = swp->target;
				if(flip[b]) {
					std::swap(pc, pt);
				}
				
				bool badSwap = false;
				if(qal[pc] >= 0 || triedBit[b * numLogical + pc]) {
					badSwap = true;
				}
				
				if(cxs[b]) {
					int lt = cxs[b]->target;
					if(lt == b) {
						lt = cxs[b]->control;
					}
					if(qal[pt] >= 0) {
						if(qal[pt] != lt) {
							badSwap = true;
						}
					}
				}
				
				for(int c = b; c >= -1; c--) {
					if(c < 0) {
						nomore = true;
					}
					
					bit = c;
					nextSwapToTry[c]++;
					if(nextSwapToTry[c] == numPossibleSwaps) {
						nextSwapToTry[c] = 0;
						flip[c] = !flip[c];
						if(flip[c]) {
							break;
						} else {
							continue;
						}
					} else {
						break;
					}
				}
				//std::cerr << "bit:" << bit << ", nextSwp:" << nextSwapToTry[bit] << ", flip:" << flip[bit] << "\n";
				
				if(badSwap) {
					good = false;
					//std::cerr << b << ":" << pc << " bad\n";
					break;
				} else {
					//std::cerr << b << ":" << pc << "(" << nextSwapToTry[b] << ")\n";
					triedBit[b * numLogical + pc] = true;
					qal[pc] = b;
					laq[b] = pc;
				}
			}
			
			//test current mapping on all CX gates
			if(good) {
				for(int x = 0; x < numPhysical; x++) {
					cxs2[x] = cxs[x];
				}
				/*std::cerr << bit << ": ";
				for(int x = 0; x < numPhysical; x++) {
					std::cerr << (int)qal[x] << ", ";
				}
				std::cerr << "\n";*/
				
				for(int x = 0; good && x < numPhysical; x++) {
					while(cxs2[x]) {
						GateNode * currentGate = cxs2[x];
						int physC = laq[currentGate->control];
						int physT = laq[currentGate->target];
						bool iteratorIsControl = (x == currentGate->control);
						if(iteratorIsControl) {
							cxs2[x] = currentGate->nextControlCNOT;
						} else {
							cxs2[x] = currentGate->nextTargetCNOT;
						}
						
						if(env->couplingDistances[physC * numPhysical + physT] != 1) {
							good = false;
						}
					}
				}
			}
		}
	}
  public:
  
	void mod(Node * node, int flag) {
		//Return unless this was called before calculating node's cost
		if(flag != MOD_TYPE_BEFORECOST) {
			return;
		}
		
		Environment * env = node->env;
		
		//If this is not the root node, return
		if(node->cycle >= 0 || node->parent != NULL) {
			return;
		}
		
		//Try to find mapping that satisfies all CNOTs
		//ToDo
		GateNode * cxs[env->numPhysicalQubits];
		for(int x = 0; x < env->numPhysicalQubits; x++) {
			cxs[x] = env->firstCXPerQubit[x];
		}
		for(int x = 0; x < env->numPhysicalQubits; x++) {
			node->laq[x] = -1;
			node->qal[x] = -1;
		}
		bool good = findPerfectMap2(cxs, node);
		
		if(good) {
			return;
		}
		
		cerr << "perfect map fail\n";
		
		//Set initial initial mapping back to default
		for(int x = 0; x < env->numPhysicalQubits; x++) {
			node->laq[x] = x;
			node->qal[x] = x;
		}
		
		//Couldn't find perfect mapping; let's find diameter of coupling map and spend that number of cycles shuffling qubits
		int diameter = 0;
		for(int x = 0; x < env->numPhysicalQubits - 1; x++) {
			for(int y = x + 1; y < env->numPhysicalQubits; y++) {
				if(env->couplingDistances[x*env->numPhysicalQubits + y] > diameter) {
					diameter = env->couplingDistances[x*env->numPhysicalQubits + y];
				}
			}
		}
		node->cycle -= diameter;
	}
};
