/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirGate::reportGate()", "CirGate::reportFanin()" and
//       "CirGate::reportFanout()" for cir cmds. Feel free to define
//       your own variables and functions.

extern CirMgr *cirMgr;

/**************************************/
/*   class CirGate member functions   */
/**************************************/
CirMgr* CirGate::_mgr = cirMgr;
size_t CirGate::_globalRef = 1;

bool CirGate::isActive() const { return _ref == _globalRef; }
void CirGate::setRef() const { _ref = _globalRef; }
void CirGate::setGlobalRef() { ++_globalRef; }
CirGate* CirGate::getGate(size_t x) const { return _mgr->_gateList[x / 2]; }

CirGate*
CirGate::changeToAIG(size_t ln) {
	CirGate* newGate = new CirAIGGate(_gid, ln);
//	newGate->_faninList.swap(_faninList);   // faninLIst should be empty
	newGate->_fanoutList.swap(_fanoutList);
  	return newGate;
}

void
CirGate::printGate(bool isNeg) const
{
	
}

void
CirGate::reportGate() const
{
	string g = getTypeStr(), s = getSymbol();
	cout << "================================================================================" << endl;
	cout << "= " << g << '(' << _gid << ')' << (s.empty()? "":"\""+s+"\"") << ", line " << lineNum << endl;
//	cout << "Fanins: "; for (size_t i = 0; i < _faninList.size(); ++i) cout << _faninList[i] << ' ';
//	cout << "\nFanouts: "; for (size_t i = 0; i < _fanoutList.size(); ++i) cout << _fanoutList[i] << ' ';
	cout << "= FECs:"; printFECs(); cout << endl;
	cout << "= Value: " << _val << endl;
	cout << "================================================================================" << endl;
}

void
CirGate::printFECs() const
{
	if (_mgr->_fecFriends.find(_gid) != _mgr->_fecFriends.end())
		_mgr->_fecFriends[_gid]->printFECs(_gid);
}

void
CirGate::reportFanin(int level) const
{
	assert (level >= 0);
	DFSFanin(0, level, false);
	setGlobalRef();
}

void
CirGate::DFSFanin(int cl, int level, bool isNeg) const
{
	for (int i = 0; i < 2 * cl; ++i) cout << ' ';
	cout << (isNeg? "!":"") << getTypeStr() << ' ' << _gid 
		 << (isActive() && cl < level? " (*)":"") << endl;

	if (cl < level && !isActive()){
		for (size_t i = 0; i < _faninList.size(); ++i)
			getGate(_faninList[i])->DFSFanin(cl + 1, level, _faninList[i] % 2);
	}
	if (cl < level && !_faninList.empty()) setRef();
}

void
CirGate::reportFanout(int level) const
{
	assert (level >= 0);
	DFSFanout(0, level, false);
	setGlobalRef();
}

void
CirGate::DFSFanout(int cl, int level, bool isNeg) const
{
	for (int i = 0; i < 2 * cl; ++i) cout << ' ';
	cout << (isNeg? "!":"") << getTypeStr() << ' ' << _gid 
		 << (isActive() && cl < level? " (*)":"") << endl;

	if (cl < level && !isActive()){
		for (size_t i = 0; i < _fanoutList.size(); ++i)
			getGate(_fanoutList[i])->DFSFanout(cl + 1, level, _fanoutList[i] % 2);
	}
	if (cl < level && !_fanoutList.empty()) setRef();
}

void deleteId(IdList& v, size_t id){
	int x = -1;
	for (size_t i = 0; i < v.size(); ++i)
		if (v[i] / 2 == id) { x = i; break; }
	assert(x != -1);
	v[x] = v.back();
	v.pop_back();
}

void
CirGate::sweep() {
	if (isActive()) return;
	for (size_t i = 0; i < _faninList.size(); ++i){
		CirGate* gate = getGate(_faninList[i]);
		if (!gate) continue;
		deleteId(gate->_fanoutList, _gid);
		if (!gate->isActive()) gate->sweep();
	}
	#ifdef LOG_DEBUG
	cout << "Sweeping: " << getTypeStr() << "(" << _gid << ") removed...\n";
	#endif // LOG_DEBUG
	_mgr->_gateList[_gid] = 0;
	if (isAig()) --_mgr->_A;
	delete this;
}

void
CirGate::checkPO(CirGate* gate) {
	if (gate->isPO()) _mgr->_POList[gate->_gid - _mgr->_M - 1] = gate->_faninList[0];
}

void
CirGate::optimize() {
	CirGate* in[2] = {getGate(_faninList[0]), getGate(_faninList[1])};
	// combine identical fanins
	if (_faninList[0] == _faninList[1]){// cout << _gid << " case x x\n";
		deleteId(in[0]->_fanoutList, _gid);
		deleteId(in[0]->_fanoutList, _gid);
		for (size_t i = 0; i < _fanoutList.size(); ++i){
			IdList& outfanin = getGate(_fanoutList[i])->_faninList;
			size_t newfanin = in[0]->_gid * 2 + (_faninList[0] + _fanoutList[i]) % 2;
			outfanin.push_back(newfanin);
			deleteId(outfanin, _gid);
			in[0]->addFanout(_fanoutList[i] / 2 * 2 + newfanin % 2);
			checkPO(getGate(_fanoutList[i]));
		}
	}
	// combine inverted fanins to CONST0
	else if (_faninList[0] / 2 == _faninList[1] / 2){// cout << _gid << " case x !x\n";
		deleteId(in[0]->_fanoutList, _gid);
		deleteId(in[0]->_fanoutList, _gid);
		for (size_t i = 0; i < _fanoutList.size(); ++i){
			IdList& outfanin = getGate(_fanoutList[i])->_faninList;
			outfanin.push_back(_fanoutList[i] % 2);
			deleteId(outfanin, _gid);
			getGate(0)->addFanout(_fanoutList[i]);
			checkPO(getGate(_fanoutList[i]));
		}
	}
	// convert to CONST0
	else if (_faninList[0] == 0 || _faninList[1] == 0){// cout << _gid << " case 0 ?\n";
		CirGate* notCONST0 = _faninList[0] == 0? in[1]: in[0];
		deleteId(getGate(0)->_fanoutList, _gid);
		deleteId(notCONST0->_fanoutList, _gid);
		for (size_t i = 0; i < _fanoutList.size(); ++i){
			IdList& outfanin = getGate(_fanoutList[i])->_faninList;
			outfanin.push_back(_fanoutList[i] % 2);
			deleteId(outfanin, _gid);
			getGate(0)->addFanout(_fanoutList[i]);
			checkPO(getGate(_fanoutList[i]));
		}
	}
	// convert to the one not CONST1
	else if (_faninList[0] == 1 || _faninList[1] == 1){// cout << _gid << " case 1 ?\n";
		size_t num = _faninList[0] == 1? _faninList[1]: _faninList[0];
		CirGate* notCONST1 = getGate(num);
		deleteId(getGate(0)->_fanoutList, _gid);
		deleteId(notCONST1->_fanoutList, _gid);
		for (size_t i = 0; i < _fanoutList.size(); ++i){
			IdList& outfanin = getGate(_fanoutList[i])->_faninList;
			size_t newfanin = notCONST1->_gid * 2 + (num + _fanoutList[i]) % 2;
			outfanin.push_back(newfanin);
			deleteId(outfanin, _gid);
			notCONST1->addFanout(_fanoutList[i] / 2 * 2 + newfanin % 2);
			checkPO(getGate(_fanoutList[i]));
		}
	}
	else return;
	#ifdef LOG_DEBUG
	cout << "Simplifying: " << _faninList[0] << " merging " << _faninList[1] << "...\n";
	#endif // LOG_DEBUG
	_mgr->_gateList[_gid] = 0;
	--_mgr->_A;
	delete this;
}

void
CirGate::strashMerge(CirGate* gate)
{
	#ifdef LOG_DEBUG
	cout << "Strashing: " << gate->getId() << " merging " << _gid << "...\n";
	#endif // LOG_DEBUG
	for (size_t i = 0; i < _fanoutList.size(); ++i){
		IdList& outfanin = getGate(_fanoutList[i])->getFanin();
		gate->addFanout(_fanoutList[i]);
		outfanin.push_back(2 * gate->_gid + _fanoutList[i] % 2);
		deleteId(outfanin, _gid);
		checkPO(getGate(_fanoutList[i]));
	}
	for (size_t i = 0; i < _faninList.size(); ++i){
		IdList& infanout = getGate(_faninList[i])->getFanout();
		deleteId(infanout, _gid);
	}
	_mgr->_gateList[_gid] = 0;
	--_mgr->_A;
	delete this;
}

void
CirGate::fraigMerge(CirGate* gate)
{
	for (size_t i = 0; i < _fanoutList.size(); ++i){
		IdList& outfanin = getGate(_fanoutList[i])->getFanin();
		unsigned newfanin = 2 * gate->_gid + (_fanoutList[i] + (gate->_val != _val)) % 2;
		outfanin.push_back(newfanin);
		gate->addFanout(_fanoutList[i] / 2 * 2 + newfanin % 2);
		deleteId(outfanin, _gid);
		checkPO(getGate(_fanoutList[i]));
	}
	_mgr->removeNotInDFSfromFECs();
	for (size_t i = 0; i < _faninList.size(); ++i){
		IdList& infanout = getGate(_faninList[i])->getFanout();
		deleteId(infanout, _gid);
	}
	_mgr->_gateList[_gid] = 0;
	--_mgr->_A;
	delete this;
}
