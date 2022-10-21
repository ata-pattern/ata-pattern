1. Command to run 2x4  grid architecture

./mapper circuits/biclique4x4.txt coupling/2x4.txt 1 1 -skipcleanup -expander qaoa2 -filter unidirectional -filter qaoa

2. Command to run sycamore with one row having 3 nodes and the other row has 4 nodes.
./mapper circuits/biclique3x4.txt coupling/syc2x3h.txt 1 1 -skipcleanup -expander qaoa2  -filter unidirectional -filter qaoa

3. Command to run linear architecture 1x6 
./mapper circuits/clique6.txt coupling/1x6.txt 1 1 -skipcleanup -expander qaoa2 -filter unidirectional -filter qaoa

4. Command to run hexagon architecture 
./mapper circuits/biclique4x4.txt coupling/hex2x4.txt 1 1 -skipcleanup -expander qaoa2  -filter unidirectional -filter qaoa

Input files: (1) one is the architecture coupling, and (2) is the circuit as cliques or bicliques. In either input file, the first line indicates the number of vertices and the number of edges. The following lines indicate the edges. 

Output: The circuit’s cycle by cycle schedule. “S” represents a SWAP. “C” represents a CPHASE gate. The circuit is pretty self-explanatory. 
