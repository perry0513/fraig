/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/
vector<FecGrp*> newGrps;
/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static size_t simCount;
static size_t simLen;
/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
#define BREAK_COUNT 		15
#define INTERVAL 			100
#define DIFFER_PERCENTAGE 	0.0001

void
CirMgr::randomSim()
{
	::simCount = 0;
	::simLen = 64;
	if (_isFraiged) {
		#ifdef LOG_DEBUG
		cout << "0 patterns simulated\n";
		#endif // LOG_DEBUG
		return; 
	}
	if (!_isSimulated) resetFEC();
	int failCount = 0, lastFecCount = 1;
	size_t max = size_t(1) << _PIList.size();
	while (1) {
		for (size_t i = 0; i < _PIList.size(); ++i)
			_gateList[_PIList[i] / 2]->setSimVal(rnGenSize_t());
		simulate(true);
		++::simCount;
		if (max < ::simCount * 64 || (::simCount % INTERVAL == 0 && 
			abs(lastFecCount - (int)_fecGrps.size()) / 
			(double)lastFecCount < DIFFER_PERCENTAGE)) break;
		if (lastFecCount == (int)_fecGrps.size()){
			if (++failCount == BREAK_COUNT || _fecGrps.empty()) break; }
		else { lastFecCount = (int)_fecGrps.size(); failCount = 0; }
		
	}
	setFecFriends();
	#ifdef LOG_DEBUG
	cout << ::simCount * 64 << " patterns simulated\n";
	#endif // LOG_DEBUG
}

void
CirMgr::fileSim(ifstream& patternFile)
{
	string temp, pattern;
	::simCount = 0;
	::simLen = 64;
	bool isValid = true;
	if (!_isSimulated) resetFEC();
	memset(&_inputs[0], 0, sizeof(_inputs[0]) * _inputs.size());
	while (getline(patternFile, temp)){
		myStrGetTok(temp, pattern);
		if (pattern.length() == 0) continue;
		if (pattern.length() != _PIList.size()){
			cerr << "Error: Pattern(" << pattern << ") length(" << pattern.length()
				 << ") does not match the number of inputs(" << _PIList.size()
				 << ") in a circuit!!\n";
			isValid = false;
			break;
		}
		for (size_t i = 0; i < pattern.length(); ++i){
			if (pattern[i] != '0' && pattern[i] != '1'){
				cerr << "Error: Pattern(" << pattern 
					 << ") contains a non-0/1 character('" << pattern[i] << "').\n";
				isValid = false;
				break;
			}
		}
		if (!isValid) break;
		for (size_t i = 0; i < _inputs.size(); ++i){
			_inputs[i] <<= 1;
			if (pattern[i] == '1') ++_inputs[i];
		}		
		++::simCount;
		if (::simCount % 64 == 0){
			simulate(false);
			memset(&_inputs[0], 0, sizeof(_inputs[0]) * _inputs.size());
		}
	}
	for (size_t i = 0; i < _inputs.size(); ++i) _inputs[i] <<= (64 - ::simCount % 64);
	if ((::simLen = ::simCount % 64) != 0 && isValid) simulate(false);
	if (::simCount < 64 && !isValid) _fecGrps.clear();
	setFecFriends();
	#ifdef LOG_DEBUG
	cout << (::simCount < 64 && !isValid? 0: ::simCount) << " patterns simulated\n";
	#endif // LOG_DEBUG
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/
void
CirMgr::resetFEC()
{
	_fecGrps.clear();
	FecGrp* fec = new FecGrp(0);
	for (size_t i = 0; i < _DFSList.size(); ++i)
		fec->add(_DFSList[i] * 2);
	_fecGrps.push_back(fec);
}

void
CirMgr::simulate(bool inputSet)
{
	simulateCircuit(inputSet);
	findFECs();
}

void
CirMgr::simulateCircuit(bool inputSet)
{
	if (!inputSet)
		for (size_t i = 0; i < _PIList.size(); ++i)
			_gateList[_PIList[i] / 2]->setSimVal(_inputs[i]);
	for (size_t i = 0; i < _DFSList.size(); ++i)
		_gateList[_DFSList[i]]->simulate();
	for (size_t i = 0; i < _POList.size(); ++i)
		_gateList[_M + 1 + i]->simulate();
	if (_simLog) writeLog();
}

void
CirMgr::writeLog(){
	ofstream& log = *_simLog;
	for (size_t i = 0; i < _POList.size(); ++i)
		_outputs[i] = _gateList[_M + 1 + i]->getSimVal()();
	size_t m = 0x8000000000000000;
	for (size_t i = 0; i < ::simLen; ++i){
		for (size_t j = 0; j < _inputs.size(); ++j)
			log << (_inputs[j] & m? 1:0);
		log << ' ';
		for (size_t j = 0; j < _POList.size(); ++j)
			log << (_outputs[j] & m? 1:0);
		log << endl;
		m >>= 1;
	}
}

void
CirMgr::findFECs()
{	
	if (!_isSimulated){
		// for the first time
		// divide all into grps by directly differing SimValue
		// regardless of inverse
		SimHash newFecGrps;
		for (size_t i = 0;i < _fecGrps[0]->size(); ++i){
			unsigned x = (*_fecGrps[0])[i];
			SimValue sv = _gateList[x / 2]->getSimVal();
			if (newFecGrps.find(sv) != newFecGrps.end())
				newFecGrps[sv]->add(x);
			else if (newFecGrps.find(!sv) != newFecGrps.end())
				newFecGrps[!sv]->add(x + 1);
			else newFecGrps[sv] = new FecGrp(x);
		}
		collectGrps(0, newFecGrps);
		addNewGrps();
		_isSimulated = true;
		return;
	}
	for (size_t i = 0; i < _fecGrps.size(); ++i){
		SimHash newFecGrps;
		for (int j = 0; j < (int)_fecGrps[i]->size(); ++j){
			unsigned x = (*_fecGrps[i])[j];
			SimValue sv = _gateList[x / 2]->getSimVal();
			// x is odd, find inverse only
			// if found, add x with x being odd (change x at collect phase)
			// if not found, create new entry with simvalue inverted
			if (x % 2){
				if (newFecGrps.find(!sv) != newFecGrps.end())
					newFecGrps[!sv]->add(x);
				else newFecGrps[!sv] = new FecGrp(x);
			}
			// x is even, find directly
			// if found, add x with x being even (no need to change)
			// if not found, create new entry with simvalue un-inverted
			else{
				if (newFecGrps.find(sv) != newFecGrps.end())
					newFecGrps[sv]->add(x);
				else newFecGrps[sv] = new FecGrp(x);
			}
		}
		collectGrps(i, newFecGrps);
	}
	addNewGrps();
}

void
CirMgr::collectGrps(size_t i, SimHash& h)
{
	for (auto it = h.begin(); it != h.end(); ++it){
		FecGrp* cand = it->second;
		if (cand->isSingleton())
		{ delete cand; continue; }
		newGrps.push_back(cand);
	}
	delete _fecGrps[i];
}

void
CirMgr::addNewGrps()
{
	_fecGrps.clear();
	for (size_t i = 0; i < newGrps.size(); ++i)
		_fecGrps.push_back(newGrps[i]);
	newGrps.clear();
}

void
CirMgr::setFecFriends()
{
	_fecFriends.clear();
	for (size_t i = 0; i < _fecGrps.size(); ++i)
		_fecGrps[i]->setFECs(_fecFriends);
}
