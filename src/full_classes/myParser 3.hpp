#ifndef ARI_PARSER
#define ARI_PARSER

#include "Environment.hpp"
#include <vector>
using namespace std;

struct ParsedGate {
	char * type;
	int target;
	int control;
};

/**
 * Parses a quantum file, sets appropriate parts of the Environment, and returns list of quantum gates.
 * @param env The environment
 * @param fileName File path for openqasm 2.0 file
 * @return vector of ParsedGate
 */
std::vector<ParsedGate> parse(Environment * env, const char * fileName);

#endif
