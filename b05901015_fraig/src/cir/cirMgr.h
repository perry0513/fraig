/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <utility>
#include "util.h"

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

#include "cirDef.h"

extern CirMgr *cirMgr;

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};

class FecGrp;
typedef unordered_map<unsigned, FecGrp*> FecHash;
typedef unordered_map<SimValue, FecGrp*, SimHasher> SimHash;

// TODO: Define your own data members and member functions
class CirMgr
{
public:
   friend class CirGate;
   CirMgr(): _isSimulated(false), _isFraiged(false) {}
   ~CirMgr();

   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   CirGate* getGate(unsigned gid) const { return gid < _gateList.size()? _gateList[gid]:0; }

   // Member functions about circuit construction
   bool readCircuit(const string&);

   // Member functions about circuit optimization
   void sweep();
   void optimize();

   // Member functions about simulation
   void randomSim();
   void fileSim(ifstream&);
   void setSimLog(ofstream *logFile) { _simLog = logFile; }
   void writeLog();

   // Member functions about fraig
   void strash();
   void printFEC() const;
   void fraig();

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates() const;
   void printNonFloating (ostream&, CirGate*) const;
   void printFECPairs();
   void writeAag(ostream&) const;
   void writeGate(ostream&, CirGate*) const;

   void postorder(CirGate*) const;

private:
   size_t          _M;
   size_t          _A;
   bool            _isSimulated;
   bool            _isFraiged;
   GateList        _gateList;
   IdList          _PIList;            // stored in pos/neg
   IdList          _POList;            // stored in pos/neg
   IdList          _floatFaninList;    // stored in gateId
   IdList          _unusedList;        // stored in gateId
   IdList          _DFSList;           // stored in gateId
   vector<size_t>  _inputs;
   vector<size_t>  _outputs;
   ofstream*       _simLog;
   vector<FecGrp*> _fecGrps;
   FecHash          _fecFriends;

   void readNoError(ifstream&);
   CirParseError readFirstLine(ifstream&);
   CirParseError readPI(ifstream&);
   CirParseError readPO(ifstream&);
   CirParseError readAIG(ifstream&);
   CirParseError readSymbol(ifstream&);
   void setFU();
   void setDFS();
   void findNonFloating(CirGate*);
   void writeDFS(CirGate*) const;

   void resetFEC();
   void simulate(bool);
   void simulateCircuit(bool);
   void findFECs();
   void collectGrps(size_t, SimHash&);
   void addNewGrps();
   void setFecFriends();
   void initSat(SatSolver& s, CirGate* base, CirGate* gate);
   void genProofModel(SatSolver& s, CirGate* gate);
   void reSim(const SatSolver& s);
   void endFraig();
   void removeNotInDFSfromFECs();

   size_t rnGenSize_t() { return (size_t(rnGen(INT_MAX)) << 32) + rnGen(INT_MAX); }
};


class FecGrp
{
public:
   friend class CirMgr;
   FecGrp () {}
   FecGrp (unsigned d):_base(d) { _data.push_back(d); }
   unsigned& operator [] (size_t i) { return _data[i]; }
   const unsigned& operator [] (size_t i) const { return _data[i]; }

   void add(unsigned d) { _data.push_back(d); }
   size_t size() const { return _data.size(); }
   void sort() { ::sort(_data.begin(), _data.end()); }
   bool isSingleton() { return _data.size() == 1; }
   void printFECs (unsigned t) ;
   void setFECs(FecHash& h);
   void setBase();
   unsigned getBase() const { return _base; }
   void erase(unsigned x);

   friend ostream& operator << (ostream& out, FecGrp& fg){
      ::sort(fg._data.begin(), fg._data.end());
      size_t d = fg[0] % 2;
      for (size_t i = 0; i < fg.size(); ++i)
         out << ' ' << (fg[i] % 2 == d? "":"!") << fg[i] / 2;
      return out << endl;
   }

private:
   IdList   _data;
   unsigned _base;
   static CirMgr* _mgr;
};

class FecCompare {
public:
   bool operator () (const FecGrp* g1, const FecGrp* g2) const
   { return (*g1)[0] < (*g2)[0]; }
};

#endif // CIR_MGR_H
