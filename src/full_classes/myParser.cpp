#include "myParser.hpp"
#include "Environment.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <fstream>
using namespace std;

///Gets next token in the OPENQASM file we're parsing
char * getToken(std::ifstream & infile, bool & sawSemicolon) {
	char c;
	int MAXBUFFERSIZE = 256;
	char buffer[MAXBUFFERSIZE];
	int bufferLoc = 0;
	bool paren = false;//true if inside parentheses, e.g. partway through reading U3(...) gate
	bool bracket = false;//true if inside brackets
	bool quote = false;//true if inside quotation marks
	bool comment = false;//true if between "//" and end-of-line
	
	if(sawSemicolon) {//saw semicolon in previous call, but had to return a different token
		sawSemicolon = false;
		char * token = new char[2];
		token[0]=';';
		token[1]=0;
		return token;
	}
	
	while(infile.get(c)) {
		assert(bufferLoc < MAXBUFFERSIZE);
		
		if(comment) {//currently parsing a single-line comment
			if(c == '\n') {
				comment = false;
			}
		} else if(quote) {
			buffer[bufferLoc++] = c;
			if(c == '"') {
				quote = false;
				buffer[bufferLoc++] = 0;
				char * token = new char[bufferLoc];
				strcpy(token, buffer);
				return token;
			}
		} else if(c == '"') {
			assert(!bufferLoc);
			quote = true;
			buffer[bufferLoc++] = c;
		} else if(c == '\r') {
		} else if(c == '/') {//probably parsing the start of a single-line comment
			if(bufferLoc && buffer[bufferLoc-1] == '/') {
				bufferLoc--;//remove '/' from buffer
				comment = true;
			} else {
				buffer[bufferLoc++] = c;
			}
		} else if(c == ';') {
			assert(!paren);
			assert(!bracket);
			
			if(bufferLoc == 0) {
				buffer[bufferLoc++] = c;
			} else {
				sawSemicolon = true;
			}
			
			buffer[bufferLoc++] = 0;
			char * token = new char[bufferLoc];
			strcpy(token, buffer);
			return token;
		} else if(c == ' ' || c == '\n' || c == '\t' || c == ',') {
			if(paren || bracket) {
				buffer[bufferLoc++] = c;
			} else if(bufferLoc) { //this whitespace is a token separator
				buffer[bufferLoc++] = 0;
				char * token = new char[bufferLoc];
				strcpy(token,buffer);
				return token;
			}
		} else if(c == '(') {
			assert(!paren);
			assert(!bracket);
			paren = true;
			buffer[bufferLoc++] = c;
		} else if(c == ')') {
			assert(paren);
			assert(!bracket);
			paren = false;
			buffer[bufferLoc++] = c;
		} else if(c == '[') {
			assert(!paren);
			assert(!bracket);
			bracket = true;
			buffer[bufferLoc++] = c;
		} else if(c == ']') {
			assert(!paren);
			assert(bracket);
			bracket = false;
			buffer[bufferLoc++] = c;
		} else {
			buffer[bufferLoc++] = c;
		}
	}
	
	if(bufferLoc) {
		buffer[bufferLoc++] = 0;
		char * token = new char[bufferLoc];
		strcpy(token,buffer);
		return token;
	} else {
		return 0;
	}
}

//returns entire gate definition (except the 'gate' keyword) as a string.
char * getCustomGate(std::ifstream & infile) {
	char c;
	int MAXBUFFERSIZE = 1024;
	char buffer[MAXBUFFERSIZE];
	int bufferLoc = 0;
	bool curlybrace = false;
	bool comment = false;//true if between "//" and end-of-line
	
	while(infile.get(c)) {
		assert(bufferLoc < MAXBUFFERSIZE);
		
		if(comment) {//currently parsing a single-line comment
			if(c == '\n') {
				comment = false;
			}
		} else if(c == '/') {//probably parsing the start of a single-line comment
			if(bufferLoc && buffer[bufferLoc-1] == '/') {
				bufferLoc--;//remove '/' from buffer
				comment = true;
			}
		} else if(c == '{') {
			assert(!curlybrace);
			curlybrace = true;
			buffer[bufferLoc++] = c;
		} else if(c == '}') {
			assert(curlybrace);
			buffer[bufferLoc++] = c;
			
			buffer[bufferLoc++] = 0;
			char * token = new char[bufferLoc];
			strcpy(token, buffer);
			return token;
		} else {
			buffer[bufferLoc++] = c;
		}
	}
	
	assert(false);
	return 0;
}

//returns the rest of the statement up to and including the semicolon at its end
//I use this for saving opaque gate statements
char * getRestOfStatement(std::ifstream & infile) {
	char c;
	int MAXBUFFERSIZE = 1024;
	char buffer[MAXBUFFERSIZE];
	int bufferLoc = 0;
	bool comment = false;//true if between "//" and end-of-line
	
	while(infile.get(c)) {
		assert(bufferLoc < MAXBUFFERSIZE);
		
		if(comment) {//currently parsing a single-line comment
			if(c == '\n') {
				comment = false;
			}
		} else if(c == '/') {//probably parsing the start of a single-line comment
			if(bufferLoc && buffer[bufferLoc-1] == '/') {
				bufferLoc--;//remove '/' from buffer
				comment = true;
			}
		} else if(c == ';') {
			buffer[bufferLoc++] = c;
			
			buffer[bufferLoc++] = 0;
			char * token = new char[bufferLoc];
			strcpy(token, buffer);
			return token;
		} else {
			buffer[bufferLoc++] = c;
		}
	}
	
	assert(false);
	return 0;
}

///Parses the specified OPENQASM file
std::vector<ParsedGate> parse(Environment * env, const char * fileName) {
	std::ifstream infile(fileName);
	vector<ParsedGate> gates;
	
	char * token = 0;
	bool b = false;
	while((token = getToken(infile, b))) {//Reminder: the single = instead of double == here is intentional.
		
		if(!strcmp(token,"OPENQASM")) {
			token = getToken(infile,b);
			if(strcmp(token,"2.0")) {
				std::cerr << "WARNING: unexpected OPENQASM version. This may fail.\n";
			}
			
			assert(env->QASM_version == NULL);
			env->QASM_version = token;
			
			token = getToken(infile,b);
			assert(!strcmp(token,";"));
		} else if(!strcmp(token,"if")) {
			std::cerr << "FATAL ERROR: if-statements not supported.\n";
			exit(1);
		} else if(!strcmp(token,"gate")) {
			token = getCustomGate(infile);
			env->customGates.push_back(token);
		} else if(!strcmp(token,"opaque")) {
			token = getRestOfStatement(infile);
			env->opaqueGates.push_back(token);
		} else if(!strcmp(token, "include")) {
			token = getToken(infile,b);
			assert(token[0] == '"');
			env->includes.push_back(token);
			
			token = getToken(infile,b);
			assert(!strcmp(token,";"));
		} else if(!strcmp(token, "qreg")) {
			char * bitArray = getToken(infile, b);
			token = getToken(infile,b);
			assert(!strcmp(token,";"));
			
			char * temp = bitArray;
			int size = 0;
			while(*temp != '[' && *temp != 0) {
				temp++;
			}
			if(*temp == 0) {
				size = 1;
			} else {
				size = std::atoi(temp+1);
			}
			
			*temp = 0;
			env->qregName.push_back(bitArray);
			env->qregSize.push_back(size);
		} else if(!strcmp(token, "creg")) {
			char * bitArray = getToken(infile, b);
			token = getToken(infile,b);
			assert(!strcmp(token,";"));
			
			char * temp = bitArray;
			int size = 0;
			while(*temp != '[' && *temp != 0) {
				temp++;
			}
			if(*temp == 0) {
				size = 1;
			} else {
				size = std::atoi(temp+1);
			}
			
			*temp = 0;
			env->cregName.push_back(bitArray);
			env->cregSize.push_back(size);
		} else if(!strcmp(token, "measure")) {
			char * qbit = getToken(infile, b);
			
			token = getToken(infile, b);
			assert(!strcmp(token,"->"));
			
			char * cbit = getToken(infile, b);
			
			token = getToken(infile,b);
			assert(!strcmp(token,";"));
			
			char * temp = qbit;
			int qdx = 0;
			while(*temp != '[' && *temp != 0) {
				temp++;
			}
			if(*temp == 0) {
				qdx = 0;
			} else {
				qdx = std::atoi(temp+1);
			}
			*temp = 0;
			
			temp = cbit;
			int cdx = 0;
			while(*temp != '[' && *temp != 0) {
				temp++;
			}
			if(*temp == 0) {
				cdx = 0;
			} else {
				cdx = std::atoi(temp+1);
			}
			*temp = 0;
			
			env->measures.push_back(std::make_pair(qdx + env->getQregOffset(qbit), cdx + env->getCregOffset(cbit)));
		} else if(!strcmp(token, ";")) {
			std::cerr << "Warning: unexpected semicolon.\n";
		} else {
			char * gateName = token;
			char * qubit1Token = getToken(infile, b);
			if(strcmp(qubit1Token, ";")) {
				assert(qubit1Token && qubit1Token[0] != 0);
				int temp = 1;
				while(qubit1Token[temp] != '[' && qubit1Token[temp] != 0) {
					temp++;
				}
				
				//Get flat array index corresponding to this qubit
				int originalOffset = 0;
				if(qubit1Token[temp] != 0) {
					originalOffset = atoi(qubit1Token + temp + 1);
				}
				qubit1Token[temp] = 0;
				int qubit1FlatOffset = env->getQregOffset(qubit1Token) + originalOffset;
				
				char * qubit2Token = getToken(infile, b);
				if(strcmp(qubit2Token, ";")) {
					assert(qubit2Token && qubit2Token[0] != 0);
					int temp = 1;
					while(qubit2Token[temp] != '[' && qubit2Token[temp] != 0) {
						temp++;
					}
					
					//Get flat array index corresponding to this qubit
					int originalOffset = 0;
					if(qubit2Token[temp] != 0) {
						originalOffset = atoi(qubit2Token + temp + 1);
					}
					qubit2Token[temp] = 0;
					int qubit2FlatOffset = env->getQregOffset(qubit2Token) + originalOffset;
					
					//We do not accept gates with three (or more) qubits:
					token = getToken(infile,b);
					assert(!strcmp(token,";"));
					
					//Push this 2-qubit gate onto gate list
					gates.push_back({gateName, qubit2FlatOffset, qubit1FlatOffset});
					
				} else {
					//Push this 1-qubit gate onto gate list
					gates.push_back({gateName, qubit1FlatOffset, -1});
				}
			} else {
				//Push this 0-qubit gate onto gate list
				gates.push_back({gateName, -1, -1});
			}
		}
	}
	
	return gates;
}
