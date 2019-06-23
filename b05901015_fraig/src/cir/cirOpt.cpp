/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed

void
CirMgr::sweep()
{	
	if (_unusedList.empty()) return;
	// markDFS
	_gateList[0]->setRef();
	for (size_t i = 0; i < _PIList.size(); ++i)
		_gateList[_PIList[i] / 2]->setRef();
	for (size_t i = 0; i < _DFSList.size(); ++i)
		_gateList[_DFSList[i]]->setRef();

	for (size_t i = 0; i < _unusedList.size(); ++i)
		_gateList[_unusedList[i]]->sweep();
	CirGate::setGlobalRef();
	setFU();
}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
	for (size_t i = 0; i < _DFSList.size(); ++i)
		_gateList[_DFSList[i]]->optimize();
	setDFS();
	setFU();
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
