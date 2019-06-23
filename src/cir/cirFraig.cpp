/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include <queue>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/
vector<CirGate*> newinput;

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
// _unusedList and _undefList won't be changed

size_t genKey(unsigned x, unsigned y)
{
	if (x < y) return ((size_t)x << 32) + y;
	else return ((size_t)y << 32) + x;
}

void
CirMgr::strash()
{
	unordered_map<size_t, CirGate*> hash;
	for (size_t i = 0; i < _DFSList.size(); ++i){
		CirGate *gate = _gateList[_DFSList[i]];
		size_t key = genKey(gate->getFanin()[0], gate->getFanin()[1]);
		if (hash.find(key) != hash.end()) gate->strashMerge(hash[key]);
		else hash[key] = gate;
	}
	setDFS();
	setFU();
}

void
CirMgr::fraig()
{
	SatSolver s;
	for (int i = 0; i < (int)_DFSList.size(); ++i){
		if (_fecFriends.find(_DFSList[i]) == _fecFriends.end()) continue;
		FecGrp& grp = *(_fecFriends[_DFSList[i]]);
		if (grp.getBase() / 2 == _DFSList[i]) continue;
		CirGate *base = _gateList[grp.getBase() / 2], *g = _gateList[_DFSList[i]];
		initSat(s, base, g);
		Var newV = s.newVar();
		bool same = base->getSimVal() != g->getSimVal();	// false if same
		s.addXorCNF(newV, base->getVar(), false, g->getVar(), same);
		s.assumeRelease();  // Clear assumptions
		s.assumeProperty(_gateList[0]->getVar(), false);
		s.assumeProperty(newV, true);  // k = 1
		if (!s.assumpSolve()){
			#ifdef LOG_DEBUG
			cout << "Fraig: " << base->getId() << " merging "
				 << (same?"!":"") << g->getId() << "...\n";
			#endif // LOG_DEBUG
			g->fraigMerge(base);
			i = base->getPos() - 1;
		}
		else reSim(s);
		newinput.clear();
	}
	setDFS();
	setFU();
	endFraig();
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
void
CirMgr::initSat(SatSolver& s, CirGate* base, CirGate* gate)
{
	s.initialize();
	_gateList[0]->setVar(s.newVar());
	_gateList[0]->setRef();
	genProofModel(s, base);
	genProofModel(s, gate);
	CirGate::setGlobalRef();
}

void
CirMgr::genProofModel(SatSolver& s, CirGate* gate)
{
	if (gate->isActive()) return;
	if (gate->isPI()){
		gate->setVar(s.newVar());
		newinput.push_back(gate);
		gate->setRef(); return;
	}
	assert(gate->isAig());
	unsigned g[2] = { gate->getFanin()[0], gate->getFanin()[1] };
	for (int i = 0; i < 2; ++i)
		genProofModel(s, gate->getGate(g[i]));
	gate->setVar(s.newVar());
	s.addAigCNF(gate->getVar(), _gateList[g[0] / 2]->getVar(),
				g[0] % 2, _gateList[g[1] / 2]->getVar(), g[1] % 2);
	gate->setRef();
}

void
CirMgr::reSim(const SatSolver& s)
{
	for (size_t i = 0; i < _PIList.size(); ++i)
		_gateList[_PIList[i] / 2]->setSimVal(rnGenSize_t());
	for (size_t i = 0; i < newinput.size(); ++i)
		newinput[i]->setSimVal(s.getValue(newinput[i]->getVar()));
	simulate(true);
	setFecFriends();
}

void
CirMgr::endFraig()
{
	for (size_t i = 0; i < _PIList.size(); ++i)
		_gateList[_PIList[i] / 2]->setSimVal(0);
	for (size_t i = 0; i < _DFSList.size(); ++i)
		_gateList[_DFSList[i]]->setSimVal(0);
	for (size_t i = 0; i < _POList.size(); ++i)
		_gateList[_M + 1 + i]->setSimVal(0);
	for (size_t i = 0; i < _fecGrps.size(); ++i) delete _fecGrps[i];
	_fecGrps.clear();
	_fecFriends.clear();
	_isSimulated = false;
	_isFraiged = true;
}
