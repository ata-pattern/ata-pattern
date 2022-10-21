//#include "QASMparser.h"
#include "myParser.hpp"
#include "Node.hpp"
#include "Expander/Meta.hpp"
#include "CostFunc/Meta.hpp"
#include "Latency/Meta.hpp"
#include "NodeMod/Meta.hpp"
#include "Filter/Meta.hpp"
#include "Queue/Meta.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stack>
#include <vector>
using namespace std;

bool _verbose = false;

//set each node's distance to furthest leaf node
//while we're at it, record the next 2-bit gate (cnot) from each gate node
int setCriticality(GateNode ** lastGatePerQubit, int numQubits) {
	GateNode ** gates = new GateNode*[numQubits];
	for(int x = 0; x < numQubits; x++) {
		gates[x] = lastGatePerQubit[x];
		if(gates[x]) {
			gates[x]->nextTargetCNOT = NULL;
			gates[x]->nextControlCNOT = NULL;
			gates[x]->criticality = 0;
		}
	}
	
	int maxCrit = 0;
	
	bool done = false;
	while(!done) {
		done = true;
		for(int x = 0; x < numQubits; x++) {
			GateNode * g = gates[x];
			if(g) {
				done = false;
			} else {
				continue;
			}
			
			//mark ready if gate is 1-qubit or appears twice in gates array
			bool ready = (g->control < 0) || (gates[g->control] == gates[g->target]);
			
			if(ready) {
				int crit = g->criticality + g->optimisticLatency;
				if(crit > maxCrit) {
					maxCrit = crit;
				}
				
				GateNode * parentT = g->targetParent;
				GateNode * parentC = g->controlParent;
				if(parentT) {
					//set parent's criticality
					if(crit > parentT->criticality) {
						parentT->criticality = crit;
					}
					
					//set parent's next 2-bit gate
					GateNode * nextCX;
					if(g->control >= 0) {
						nextCX = g;
					} else {
						nextCX = g->nextTargetCNOT;
					}
					if(parentT->target == g->target) {
						parentT->nextTargetCNOT = nextCX;
					} else if(g->control >= 0 && parentT->target == g->control) {
						parentT->nextTargetCNOT = nextCX;
					}
					if(parentT->control == g->target) {
						parentT->nextControlCNOT = nextCX;
					} else if(g->control >= 0 && parentT->control == g->control) {
						parentT->nextControlCNOT = nextCX;
					}
				}
				if(parentC) {
					//set parent's criticality
					if(crit > parentC->criticality) {
						parentC->criticality = crit;
					}
					
					//set parent's next 2-bit gate
					GateNode * nextCX;
					if(g->control >= 0) {
						nextCX = g;
					} else {
						nextCX = g->nextTargetCNOT;
					}
					if(parentC->target == g->target) {
						parentC->nextTargetCNOT = nextCX;
					} else if(g->control >= 0 && parentC->target == g->control) {
						parentC->nextTargetCNOT = nextCX;
					}
					if(parentC->control == g->target) {
						parentC->nextControlCNOT = nextCX;
					} else if(g->control >= 0 && parentC->control == g->control) {
						parentC->nextControlCNOT = nextCX;
					}
				}
				
				//adjust gates array
				assert(gates[g->target] == g);
				gates[g->target] = parentT;
				if(g->control >= 0) {
					assert(gates[g->control] == g);
					gates[g->control] = parentC;
				}
			}
		}
	}
	
	delete [] gates;
	
	return maxCrit;
}

//parse qasm, build dependence graph, put root gates into firstGates:
void buildDependencyGraph(string filename, Latency * lat, set<GateNode*> & firstGates, int & numQubits, Environment * env) {
	numQubits = 0;
	
	/*
	//parse qasm
	std::vector<ParsedGate> gates = parse(env, filename.c_str());
	env->numGates = gates.size();
	int maxQubits = 0;
	for(unsigned int x = 0; x < env->qregName.size(); x++) {
		maxQubits += env->qregSize[x];
	}
	*/
	
	std::vector<ParsedGate> gates;
	std::fstream myfile(filename, std::ios_base::in);
	unsigned int numEdges;
	int maxQubits;
	myfile >> maxQubits;
	myfile >> numEdges;
	env->numGates = numEdges;
	for(unsigned int x = 0; x < numEdges; x++) {
		int a, b;
		myfile >> a;
		myfile >> b;
		char * gateName = (char*) malloc(3);
		gateName[0] = 'c';
		gateName[1] = 'z';
		gateName[2] = 0;
		gates.push_back({gateName, a, b});
	}
	
	env->firstCXPerQubit = new GateNode*[maxQubits];
	for(int x = 0; x < maxQubits; x++) {
		env->firstCXPerQubit[x] = 0;
	}
	
	//build dependence graph
	GateNode ** lastGatePerQubit = new GateNode*[maxQubits];
	for(int x = 0; x < maxQubits; x++) {
		lastGatePerQubit[x] = 0;
	}
	for(unsigned int x = 0; x < gates.size(); x++) {
		GateNode * v = new GateNode;
		v->control = gates.at(x).control;
		v->target = gates.at(x).target;
		v->name = gates.at(x).type;
		v->criticality = 0;
		v->optimisticLatency = lat->getLatency(v->name, (v->control >= 0 ? 2 : 1), -1, -1);
		v->controlChild = 0;
		v->targetChild = 0;
		v->controlParent = 0;
		v->targetParent = 0;
		
		if(v->control >= 0) {
			if(!env->firstCXPerQubit[v->control]) {
				env->firstCXPerQubit[v->control] = v;
			}
			if(!env->firstCXPerQubit[v->target]) {
				env->firstCXPerQubit[v->target] = v;
			}
		}
		
		if(v->control >= numQubits) {
			numQubits = v->control + 1;
		}
		if(v->target >= numQubits) {
			numQubits = v->target + 1;
		}
		
		assert(v->control != v->target);
		
		//if v is a root gate, add it to firstGates
		if(!v->controlParent && !v->targetParent) {
			firstGates.insert(v);
		}
	}
	
	assert(numQubits <= maxQubits);
	
	//set critical path lengths starting from each gate
	setCriticality(lastGatePerQubit, numQubits);
	
	delete [] lastGatePerQubit;
}

//parse coupling map, producing a list of edges and number of physical qubits
void buildCouplingMap(string filename, set<pair<int, int> > & edges, int & numPhysicalQubits) {
	std::fstream myfile(filename, std::ios_base::in);
	unsigned int numEdges;
	
	myfile >> numPhysicalQubits;
	myfile >> numEdges;
	for(unsigned int x = 0; x < numEdges; x++) {
		int a, b;
		myfile >> a;
		myfile >> b;
		pair<int, int> edge = make_pair(a,b);
		edges.insert(edge);
	}
}

//Calculate minimum distance between each pair of physical qubits
//ToDo replace this with something more efficient?
void calcDistances(int * distances, int numQubits) {
	bool done = false;
	while(!done) {
		done = true;
		for(int x = 0; x < numQubits; x++) {
			for(int y = 0; y < numQubits; y++) {
				if(x==y) {
					continue;
				}
				for(int z = 0; z < numQubits; z++) {
					if(x == z || y == z) {
						continue;
					}
					
					if(distances[x*numQubits + y] + distances[y*numQubits + z] < distances[x*numQubits + z]) {
						done = false;
						distances[x*numQubits + z] = distances[x*numQubits + y] + distances[y*numQubits + z];
						distances[z*numQubits + x] = distances[x*numQubits + z];
					}
				}
			}
		}
	}
}

//Print a node's scheduled gates
//returns how many cycles the node takes to complete all its gates
int printNode(std::ostream & stream, LinkedStack<ScheduledGate*> * gates) {
	int cycles = 0;
	std::stack<ScheduledGate*> gateStack;
	while(gates->size > 0) {
		gateStack.push(gates->value);
		gates = gates->next;
	}
	
	while(!gateStack.empty()) {
		ScheduledGate * sg = gateStack.top();
		gateStack.pop();
		int target = sg->physicalTarget;
		int control = sg->physicalControl;
		if(sg->gate->name.compare("swp") && sg->gate->name.compare("SWP")) {
			stream << "c(";
		} else {
			stream << "s(";
		}
		if(control >= 0) {
			stream << control << ",";
		}
		stream << target;
		stream << ")";
		stream << " //cycle " << sg->cycle;
		if(sg->gate->name.compare("swp") && sg->gate->name.compare("SWP")) {
			int target = sg->gate->target;
			int control = sg->gate->control;
			stream << " //originally ";
			if(sg->gate->name.compare("swp") && sg->gate->name.compare("SWP")) {
				stream << "c(";
			} else {
				stream << "s(";
			}
			if(control >= 0) {
				stream << control << ",";
			}
			stream << target << ")";
		}
		stream << "\n";
		
		if(sg->cycle + sg->latency > cycles) {
			cycles = sg->cycle + sg->latency;
		}
	}
	
	return cycles;
}

unsigned nChoosek(unsigned n, unsigned k) {
    if (k > n) return 0;
    if (k * 2 > n) k = n-k;
    if (k == 0) return 1;

    int result = n;
    for(unsigned int i = 2; i <= k; ++i ) {
        result *= (n-i+1);
        result /= i;
    }
    return result;
}

//string comparison
int caseInsensitiveCompare(const char * c1, const char * c2) {
	for(int x = 0;; x++) {
		if(toupper(c1[x]) == toupper(c2[x])) {
			if(!c1[x]) {
				return 0;
			}
		} else {
			return toupper(c1[x]) - toupper(c2[x]);
		}
	}
}

//string comparison
int caseInsensitiveCompare(string str, const char * c2) {
	const char * c1 = str.c_str();
	return caseInsensitiveCompare(c1, c2);
}
int caseInsensitiveCompare(const char * c1, string str) {
	const char * c2 = str.c_str();
	return caseInsensitiveCompare(c1, c2);
}

int main(int argc, char** argv) {
	char * qasmFileName = NULL;
	char * couplingMapFileName = NULL;
	char** latencyC = NULL;
	char** latencyS = NULL;
	
	Expander * ex = NULL;
	CostFunc * cf = NULL;
	Latency * lat = NULL;
	Queue * nodes = NULL;
	Environment * env = new Environment;
	
	unsigned int retainPopped = 0;
	
	bool skipFinalCleanup = false;
	
	int choice = -1;
	//bool printNumQubitsAndQuit = false;
	
	//variables used to indicate we'll spend some cycles searching for initial mapping:
	int initialSearchCycles = 0;
	
	//variables for user-specified initial mapping:
	char init_qal[MAX_QUBITS];
	char init_laq[MAX_QUBITS];
	int use_specified_init_mapping = 0;
	
	//If true, generate enough initial mappings to guarantee optimal result for clique
	bool cliqueMap = false;
	
	if(!ex) ex = std::get<0>(expanders[0]);
	if(!cf) cf = std::get<0>(costFunctions[0]);
	if(!lat) lat = std::get<0>(latencies[0]);
	if(!nodes) nodes = std::get<0>(queues[0]);
	
	//Parse command-line arguments:
	for(int iter = 1; iter < argc; iter++) {
		if(!caseInsensitiveCompare(argv[iter], "-retain") || !caseInsensitiveCompare(argv[iter], "-retainPopped")) {
			retainPopped = atoi(argv[++iter]);
		} else if(!caseInsensitiveCompare(argv[iter], "-qal")) {
			++iter;
			use_specified_init_mapping = 1;
			int c = 0;
			int i = 0;
			while(argv[iter][c]) {
				if(argv[iter][c] < '0' || argv[iter][c] > '9') {
					if(argv[iter][c] != '-') {
						c++;
						continue;
					}
				}
				init_qal[i++] = atoi(&(argv[iter][c]));
				
				while(argv[iter][c] != ',' && argv[iter][c] != 0) {
					c++;
				}
			}
			for(; i < MAX_QUBITS; i++) {
				init_qal[i] = -1;
			}
		} else if(!caseInsensitiveCompare(argv[iter], "-laq")) {
			++iter;
			use_specified_init_mapping = 2;
			int c = 0;
			int i = 0;
			while(argv[iter][c]) {
				if(argv[iter][c] < '0' || argv[iter][c] > '9') {
					if(argv[iter][c] != '-') {
						c++;
						continue;
					}
				}
				init_laq[i++] = atoi(&(argv[iter][c]));
				
				while(argv[iter][c] != ',' && argv[iter][c] != 0) {
					c++;
				}
			}
			for(; i < MAX_QUBITS; i++) {
				init_laq[i] = -1;
			}
		} else if(!caseInsensitiveCompare(argv[iter], "-expander")) {
			char * choiceStr = argv[++iter];
			bool found = false;
			for(int x = 0; x < NUMEXPANDERS; x++) {
				if(!caseInsensitiveCompare(std::get<1>(expanders[x]), choiceStr)) {
					found = true;
					ex = std::get<0>(expanders[x]);
					iter += ex->setArgs(argv + (iter+1));
					break;
				}
			}
			assert(found);
		} else if(!caseInsensitiveCompare(argv[iter], "-pureSwapDiameter") || !caseInsensitiveCompare(argv[iter], "-rewindD")) {
			initialSearchCycles = -1;//a later part of the code detects the nonsensical -1 value and sets this appropriately
		} else if(!caseInsensitiveCompare(argv[iter], "-pureSwaps") || !caseInsensitiveCompare(argv[iter], "-rewindCycles")) {
			char * choiceStr = argv[++iter];
			initialSearchCycles = atoi(choiceStr);
		} else if(!caseInsensitiveCompare(argv[iter], "-nodemod")) {
			char * choiceStr = argv[++iter];
			bool found = false;
			for(int x = 0; x < NUMNODEMODS; x++) {
				if(!caseInsensitiveCompare(std::get<1>(nodeMods[x]), choiceStr)) {
					found = true;
					NodeMod * nm = std::get<0>(nodeMods[x]);
					env->nodeMods.push_back(nm);
					iter += nm->setArgs(argv + (iter+1));
					break;
				}
			}
			assert(found);
		} else if(!caseInsensitiveCompare(argv[iter], "-costfunction") || !caseInsensitiveCompare(argv[iter], "-costfunc") || !caseInsensitiveCompare(argv[iter], "-cost")) {
			char * choiceStr = argv[++iter];
			bool found = false;
			for(int x = 0; x < NUMCOSTFUNCTIONS; x++) {
				if(!caseInsensitiveCompare(std::get<1>(costFunctions[x]), choiceStr)) {
					found = true;
					cf = std::get<0>(costFunctions[x]);
					iter += cf->setArgs(argv + (iter+1));
					break;
				}
			}
			assert(found);
		} else if(!caseInsensitiveCompare(argv[iter], "-filter")) {
			char * choiceStr = argv[++iter];
			bool found = false;
			for(int x = 0; x < NUMFILTERS; x++) {
				if(!caseInsensitiveCompare(std::get<1>(FILTERS[x]), choiceStr)) {
					found = true;
					Filter * fil = std::get<0>(FILTERS[x])->createEmptyCopy();
					env->filters.push_back(fil);
					iter += fil->setArgs(argv + (iter+1));
					break;
				}
			}
			assert(found);
		} else if(!caseInsensitiveCompare(argv[iter], "-queue")) {
			char * choiceStr = argv[++iter];
			bool found = false;
			for(int x = 0; x < NUMQUEUES; x++) {
				if(!caseInsensitiveCompare(std::get<1>(queues[x]), choiceStr)) {
					found = true;
					nodes = std::get<0>(queues[x]);
					iter += nodes->setArgs(argv + (iter+1));
					break;
				}
			}
			assert(found);
		} else if(!caseInsensitiveCompare(argv[iter], "-v")) {
			_verbose = true;
		} else if(!caseInsensitiveCompare(argv[iter], "-cliquemap")) {
			cliqueMap = true;
		} else if(!caseInsensitiveCompare(argv[iter], "-skipcleanup")) {
			skipFinalCleanup = true;
		} else if(!qasmFileName) {
			qasmFileName = argv[iter];
		} else if(!couplingMapFileName) {
			couplingMapFileName = argv[iter];
		} else if(latencyC == NULL) {
			latencyC = &argv[iter];
		} else if(latencyS == NULL) {
			latencyS = &argv[iter];
		} else {
			assert(false);
		}
	}
	
	if(latencyC != NULL) {
		assert(latencyC + 1 == latencyS);
		lat = std::get<0>(latencies[1]);
		lat->setArgs(latencyC);
	}
	
	bool userChoices = false;
	
	if(!ex) {
		userChoices = true;
		choice = -1;
		cerr << "Select an expander.\n";
		for(int x = 0; x < NUMEXPANDERS; x++) {
			cerr << " " << x << ": " << std::get<1>(expanders[x]) << ": " << std::get<2>(expanders[x]) << "\n";
		}
		cin >> choice;
		assert(choice >= 0 && choice < NUMEXPANDERS);
		ex = std::get<0>(expanders[choice]);
		ex->setArgs();
	}
	
	if(!cf) {
		userChoices = true;
		choice = -1;
		cerr << "Select a cost function.\n";
		for(int x = 0; x < NUMCOSTFUNCTIONS; x++) {
			cerr << " " << x << ": " << std::get<1>(costFunctions[x]) << ": " << std::get<2>(costFunctions[x]) << "\n";
		}
		cin >> choice;
		assert(choice >= 0 && choice < NUMCOSTFUNCTIONS);
		cf = std::get<0>(costFunctions[choice]);
		cf->setArgs();
	}
	
	if(!lat) {
		userChoices = true;
		choice = -1;
		cerr << "Select a latency setting.\n";
		for(int x = 0; x < NUMLATENCIES; x++) {
			cerr << " " << x << ": " << std::get<1>(latencies[x]) << ": " << std::get<2>(latencies[x]) << "\n";
		}
		cin >> choice;
		assert(choice >= 0 && choice < NUMLATENCIES);
		lat = std::get<0>(latencies[choice]);
		lat->setArgs();
	}
	
	if(!nodes) {
		userChoices = true;
		choice = -1;
		cerr << "Select a queue structure.\n";
		for(int x = 0; x < NUMQUEUES; x++) {
			cerr << " " << x << ": " << std::get<1>(queues[x]) << ": " << std::get<2>(queues[x]) << "\n";
		}
		cin >> choice;
		assert(choice >= 0 && choice < NUMQUEUES);
		nodes = std::get<0>(queues[choice]);
		nodes->setArgs();
	}
	
	if(userChoices) {
		int numselected;
		
		bool filtersOn[NUMFILTERS];
		for(int x = 0; x < NUMFILTERS; x++) {
			filtersOn[x] = false;
		}
		numselected = 0;
		while(numselected < NUMFILTERS) {
			choice = -1;
			cerr << "Select a filter, or -1 for no additional filters.\n";
			cerr << " " << -1 << ": " << "no more filters\n";
			for(int x = 0; x < NUMFILTERS; x++) {
				if(!filtersOn[x]) {
					if(x < 10) cerr << " ";
					cerr << " " << x << ": " << std::get<1>(FILTERS[x]) << ": " << std::get<2>(FILTERS[x]) << "\n";
				}
			}
			cin >> choice;
			if(choice < 0 || choice >= NUMFILTERS) {
				break;
			}
			if(!filtersOn[choice]) {
				numselected++;
				filtersOn[choice] = true;
				Filter * fil = std::get<0>(FILTERS[choice])->createEmptyCopy();
				env->filters.push_back(fil);
				fil->setArgs();
			}
		}
		
		numselected = 0;
		bool nodeModsOn[NUMNODEMODS];
		for(int x = 0; x < NUMNODEMODS; x++) {
			nodeModsOn[x] = false;
		}
		while(numselected < NUMNODEMODS) {
			choice = -1;
			cerr << "Select a node modifier, or -1 for no additional mods.\n";
			cerr << " " << -1 << ": " << "no more node mods\n";
			for(int x = 0; x < NUMNODEMODS; x++) {
				if(!nodeModsOn[x]) {
					if(x < 10) cerr << " ";
					cerr << " " << x << ": " << std::get<1>(nodeMods[x]) << ": " << std::get<2>(nodeMods[x]) << "\n";
				}
			}
			cin >> choice;
			if(choice < 0 || choice >= NUMNODEMODS) {
				break;
			}
			if(!nodeModsOn[choice]) {
				numselected++;
				nodeModsOn[choice] = true;
				NodeMod * nm = std::get<0>(nodeMods[choice]);
				env->nodeMods.push_back(nm);
				nm->setArgs();
			}
		}
	}
	
	env->latency = lat;
	env->swapCost = lat->getLatency("swp", 2, -1, -1);
	env->cost = cf;
	
	set<GateNode*> firstGates;
	buildDependencyGraph(qasmFileName, lat, firstGates, env->numLogicalQubits, env);
	
	//Parse coupling map
	buildCouplingMap(couplingMapFileName, env->couplings, env->numPhysicalQubits);
	assert(env->numPhysicalQubits >= env->numLogicalQubits);
	
	//Calculate distances between physical qubits in coupling map (min 1, max numPhysicalQubits-1)
	env->couplingDistances = new int[env->numPhysicalQubits*env->numPhysicalQubits];
	for(int x = 0; x < env->numPhysicalQubits * env->numPhysicalQubits; x++) {
		env->couplingDistances[x] = env->numPhysicalQubits - 1;
	}
	for(auto iter = env->couplings.begin(); iter != env->couplings.end(); iter++) {
		int x = (*iter).first;
		int y = (*iter).second;
		env->couplingDistances[x*env->numPhysicalQubits + y] = 1;
		env->couplingDistances[y*env->numPhysicalQubits + x] = 1;
	}
	calcDistances(env->couplingDistances, env->numPhysicalQubits);
	
	if(initialSearchCycles < 0) {
		int diameter = 0;
		for(int x = 0; x < env->numPhysicalQubits - 1; x++) {
			for(int y = x + 1; y < env->numPhysicalQubits; y++) {
				if(env->couplingDistances[x*env->numPhysicalQubits + y] > diameter) {
					diameter = env->couplingDistances[x*env->numPhysicalQubits + y];
				}
			}
		}
		initialSearchCycles = diameter;
	}
	
	//Prepare list of gates corresponding to possible swaps
	//ToDo: make it so this won't cause redundancies when given directed coupling map
		//might need to adjust parts of code that infer its size from coupling's size
	env->possibleSwaps = new GateNode*[env->couplings.size()];
	auto iter = env->couplings.begin();
	int x = 0;
	while(iter != env->couplings.end()) {
		GateNode * g = new GateNode();
		g->control = (*iter).first;
		g->target = (*iter).second;
		g->name = "swp";
		g->optimisticLatency = lat->getLatency("swp", 2, g->target, g->control);
		env->possibleSwaps[x] = g;
		x++;
		iter++;
	}
	
	//Set up root node (for cycle -1, before any gates are scheduled):
	if(!cliqueMap || env->numPhysicalQubits == env->numLogicalQubits) {
		Node * root = new Node();
		if(use_specified_init_mapping > 0) {
			for(int x = 0; x < env->numPhysicalQubits; x++) {
				root->laq[x] = -1;
				root->qal[x] = -1;
			}
		} else {
			for(int x = env->numLogicalQubits; x < env->numPhysicalQubits; x++) {
				root->laq[x] = -1;
				root->qal[x] = -1;
			}
		}
		if(use_specified_init_mapping == 1) {
			for(int x = 0; x < env->numPhysicalQubits; x++) {
				root->qal[x] = init_qal[x];
				if(init_qal[x] >= 0) {
					root->laq[(int) init_qal[x]] = x;
				}
			}
		} else if(use_specified_init_mapping == 2) {
			for(int x = 0; x < env->numPhysicalQubits; x++) {
				root->laq[x] = init_laq[x];
				if(init_laq[x] >= 0) {
					root->qal[(int) init_laq[x]] = x;
				}
			}
		}
		root->parent = NULL;
		root->numUnscheduledGates = env->numGates;
		root->env = env;
		root->cycle = -1;
		if(initialSearchCycles) {
			//std::cerr << "//Note: making attempt to find better initial mapping.\n";
			root->cycle -= initialSearchCycles;
		}
		root->readyGates = firstGates;
		root->scheduled = new LinkedStack<ScheduledGate*>;
		root->cost = cf->getCost(root);
		nodes->push(root);
	} else {
		int phys = env->numPhysicalQubits;
		int log = env->numLogicalQubits;
		int possibilities = nChoosek(phys, log);
		assert(possibilities > 0);
		
		if(phys-log > 1) {
			std::cerr << "ERROR: current implementation can't handle mapping > 1 empty physical qubit.\n";
			assert(phys - log <= 1);
		}
		
		for(int x = 0; x < phys; x++) {
			Node * root = new Node();
			for(int y = 0; y < phys; y++) {
				root->laq[y] = -1;
				root->qal[y] = -1;
			}
			for(int y = 0; y < phys; y++) {
				if(y < x) {
					root->laq[y] = y;
					root->qal[y] = y;
				} else if(y > x) {
					root->laq[y-1] = y;
					root->qal[y] = y-1;
				}
			}
			root->parent = NULL;
			root->numUnscheduledGates = env->numGates;
			root->env = env;
			root->cycle = -1;
			root->readyGates = firstGates;
			root->scheduled = new LinkedStack<ScheduledGate*>;
			root->cost = cf->getCost(root);
			nodes->push(root);
		}
	}
	
	//Cleanup filters before I start messing things up:
	for(int x = 0; x < NUMFILTERS; x++) {
		bool del = true;
		for(unsigned int y = 0; y < env->filters.size(); y++) {
			if(env->filters[y] == std::get<0>(FILTERS[x])) {
				del = false;
				break;
			}
		}
		if(del) {
			delete std::get<0>(FILTERS[x]);
		}
	}
	env->resetFilters();
	
	//Pop nodes from the queue until we're done:
	bool notDone = true;
	std::vector<Node*> tempNodes;
	int numPopped = 0;
	int counter = 0;
	std::deque<Node*> oldNodes;
	while(notDone) {
		assert(nodes->size() > 0);
		
		while(retainPopped && oldNodes.size() > retainPopped) {
			Node * pop = oldNodes.front();
			oldNodes.pop_front();
			if(pop == nodes->getBestFinalNode()) {
				oldNodes.push_back(pop);
			} else {
				env->deleteRecord(pop);
				delete pop;
			}
		}
		
		Node * n = nodes->pop();
		n->expanded = true;
		
		if(n->dead) {
			if(n == nodes->getBestFinalNode()) {
				oldNodes.push_back(n);
			} else {
				env->deleteRecord(n);
				delete n;
			}
			continue;
		}
		
		oldNodes.push_back(n);
		
		/*
		if(n->parent && n->parent->dead) {
			std::cerr << "skipping child of dead node:\n";
			printNode(std::cerr, lastNode->scheduled);
			n->dead = true;
			continue;
		}
		*/
		
		numPopped++;
		
		//In verbose mode, we pause after popping some number of nodes:
		if(_verbose && counter <= 0) {
			cerr << "cycle " << n->cycle << "\n";
			cerr << "cost " << n->cost << "\n";
			cerr << "unscheduled " << n->numUnscheduledGates << " from this node\n";
			std::cerr << "mapping (logical qubit at each location): ";
			for(int x = 0; x < env->numPhysicalQubits; x++) {
				std::cerr << (int)n->qal[x] << ", ";
			}
			std::cerr << "\n";
			std::cerr << "mapping (location of each logical qubit): ";
			for(int x = 0; x < env->numPhysicalQubits; x++) {
				std::cerr << (int)n->laq[x] << ", ";
			}
			std::cerr << "\n";
			std::cerr << "//" << (numPopped-1) << " nodes popped from queue so far.\n";
			std::cerr << "//" << nodes->size() << " nodes remain in queue.\n";
			env->printFilterStats(std::cerr);
			printNode(std::cerr, n->scheduled);
			//cf->getCost(n);
			for(GateNode * ready : n->readyGates) {
				std::cerr << "ready: ";
				int control = (ready->control >= 0) ? n->laq[ready->control] : -1;
				int target = (ready->target >= 0) ? n->laq[ready->target] : -1;
				std::cerr << ready->name << " ";
				if(ready->control >= 0) {
					std::cerr << "q[" << control << "],";
				}
				std::cerr << "q[" << target << "]";
				std::cerr << ";";
				
				target = ready->target;
				control = ready->control;
				std::cerr << " //" << ready->name << " ";
				if(control >= 0) {
					std::cerr << "q[" << control << "],";
				}
				std::cerr << "q[" << target << "]";
				std::cerr << "\n";
			}
			
			cin >> counter;//pause the program after (counter) steps
			if(counter < 0) exit(1);
		}
		
		notDone = ex->expand(nodes, n);
		
		counter--;
	}
	
	Node * finalNode = nodes->getBestFinalNode();
	
	
	//Figure out what the initial mapping must have been
	LinkedStack<ScheduledGate*> * sg = finalNode->scheduled;
	char inferredQal[env->numPhysicalQubits];
	char inferredLaq[env->numPhysicalQubits];
	for(int x = 0; x < env->numPhysicalQubits; x++) {
		inferredQal[x] = finalNode->qal[x];
		inferredLaq[x] = finalNode->laq[x];
	}
	/*
	std::cerr << "//Note: qubit mapping at end (location of each logical qubit): ";
	for(int x = 0; x < env->numLogicalQubits; x++) {
		std::cerr << (int)inferredLaq[x] << ", ";
	}
	std::cerr << "\n";
	std::cerr << "//Note: qubit mapping at end (logical qubit at each location): ";
	for(int x = 0; x < env->numPhysicalQubits; x++) {
		std::cerr << (int)inferredQal[x] << ", ";
	}
	std::cerr << "\n";
	*/
	while(sg->size > 0) {
		if(sg->value->gate->control >= 0) {
			if((!sg->value->gate->name.compare("swp")) || (!sg->value->gate->name.compare("SWP"))) {
				
				if(inferredQal[sg->value->physicalControl] >= 0 && inferredQal[sg->value->physicalTarget] >= 0) {
					std::swap(inferredLaq[(int)inferredQal[sg->value->physicalControl]], inferredLaq[(int)inferredQal[sg->value->physicalTarget]]);
				} else if(inferredQal[sg->value->physicalControl] >= 0) {
					inferredLaq[(int)inferredQal[sg->value->physicalControl]] = sg->value->physicalTarget;
				} else if(inferredQal[sg->value->physicalTarget] >= 0) {
					inferredLaq[(int)inferredQal[sg->value->physicalTarget]] = sg->value->physicalControl;
				}
				
				std::swap(inferredQal[sg->value->physicalTarget], inferredQal[sg->value->physicalControl]);
			}
		} else {
			if(sg->value->physicalTarget < 0) {
				sg->value->physicalTarget = inferredLaq[sg->value->gate->target];
			}
			
			//in case this qubit's assignment is arbitrary:
			if(sg->value->physicalTarget < 0) {
				for(int x = 0; x < env->numPhysicalQubits; x++) {
					if(inferredQal[x] < 0) {
						inferredQal[x] = sg->value->gate->target;
						inferredLaq[sg->value->gate->target] = x;
						sg->value->physicalTarget = x;
						break;
					}
				}
			}
		}
		sg = sg->next;
	}
	
	//Print out the initial mapping:
	std::cout << "//Note: initial mapping (logical qubit at each location): ";
	for(int x = 0; x < env->numPhysicalQubits; x++) {
		std::cout << (int)inferredQal[x] << ", ";
	}
	std::cout << "\n";
	std::cout << "//Note: initial mapping (location of each logical qubit): ";
	for(int x = 0; x < env->numLogicalQubits; x++) {
		std::cout << (int)inferredLaq[x] << ", ";
	}
	std::cout << "\n";
	
	//Print the output:
	/*std::cout << "OPENQASM " << env->QASM_version << ";\n";
	for(unsigned int x = 0; x < env->includes.size(); x++) {
		std::cout << "include " << env->includes[x] << ";\n";
	}
	for(unsigned int x = 0; x < env->customGates.size(); x++) {
		std::cout << "gate " << env->customGates[x] << "\n";
	}
	for(unsigned int x = 0; x < env->opaqueGates.size(); x++) {
		std::cout << "opaque " << env->opaqueGates[x] << "\n";
	}
	std::cout << "qreg q[" << env->numPhysicalQubits << "];\n";
	std::cout << "creg c[" << env->numPhysicalQubits << "];\n";
	*/
	int numCycles = printNode(std::cout, finalNode->scheduled);
	for(unsigned int x = 0; x < env->measures.size(); x++) {
		std::cout << "measure q[" << (int) finalNode->laq[env->measures[x].first] << "] -> c[" << env->measures[x].second << "];\n";
	}
	
	//if(_verbose) {
		//Print some metadata about the input & output:
		std::cout << "//" << env->numGates << " original gates\n";
		std::cout << "//" << finalNode->scheduled->size << " gates in generated circuit\n";
		std::cout << "//" << numCycles << " depth of generated circuit\n"; //" (and costFunc reports " << finalNode->cost << ")\n";
		std::cout << "//" << (numPopped-1) << " nodes popped from queue for processing.\n";
		std::cout << "//" << nodes->size() << " nodes remain in queue.\n";
		env->printFilterStats(std::cout);
	//}
	
	//Cleanup
	if(!skipFinalCleanup) {
		while(nodes->size()) {
			Node * n = nodes->pop();
			delete n;
		}
		while(oldNodes.size() > 0) {
			Node * n = oldNodes.front();
			oldNodes.pop_front();
			delete n;
		}
		for(int x = 0; x < NUMEXPANDERS; x++) {
			delete std::get<0>(expanders[x]);
		}
		for(unsigned int y = 0; y < env->filters.size(); y++) {
			delete env->filters[y];
		}
		for(int x = 0; x < NUMCOSTFUNCTIONS; x++) {
			delete std::get<0>(costFunctions[x]);
		}
		for(int x = 0; x < NUMLATENCIES; x++) {
			delete std::get<0>(latencies[x]);
		}
		for(int x = 0; x < NUMQUEUES; x++) {
			delete std::get<0>(queues[x]);
		}
		for(unsigned int x = 0; x < env->couplings.size(); x++) {
			delete env->possibleSwaps[x];
		}
		delete [] env->possibleSwaps;
		delete [] env->firstCXPerQubit;
		delete [] env->couplingDistances;
		delete env;
	}
	
	return 0;
}