# FRAIG: Functionally Reduced And-Inverter Graphs
## Install
```
~ $ git clone https://github.com/perry0513/fraig.git
~ $ cd fraig
~/fraig $ make-mac	// (or make-linux based on the underlying OS)
~/fraig $ make
```
- An executable `fraig` should appear after executing the above commands.

## USAGE
   - After executing `fraig`, there are multiple operations you could do:
	1. `CIRRead`: reads in benchmarks in aag format
	2. `CIRPrint`: print circuit information
	3. `CIRSWeep`: perform unused gate sweeping
	4. `CIRSTRash`: perform structural hashing
	5. `CIROPTimize`: perform trivial optimizations
	6. `CIRSIMulate`: perform Boolean logic simulation
	7. `CIRFraig`: perform functional equivalence checking and merging
	8. `CIRWrite`: write the netlist into aag file
	9. `usage`: report runtime and memory usage
	10. `help`: For more commands and information, please type `help`
