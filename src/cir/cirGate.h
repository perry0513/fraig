/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <iostream>
#include "cirDef.h"
#include "sat.h"
#include "myHashMap.h"

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

class CirGate;

//------------------------------------------------------------------------
//   class CirGate and derived classes
//------------------------------------------------------------------------
class CirGate
{
public:
   friend class CirMgr;
   CirGate(int id, size_t l)
   :_gid(id), lineNum(l), _ref(0) {}
   virtual ~CirGate() {}

   // Basic access methods
   virtual string getTypeStr() const { return "UNDEF"; }
   virtual void setSymbol(string s) {}
   virtual string getSymbol() const { return ""; }
   virtual bool isUNDEF() const { return false; }
   virtual bool isPI() const { return false; }
   virtual bool isPO() const { return false; }
   virtual bool isAig() const { return false; }
   virtual bool isCONST() const { return false; }
   virtual void simulate() {}
   virtual void setVar(const Var& c) {}
   virtual const Var getVar() const { return -1; }
   int getId() const { return _gid; }
   unsigned getLineNo() const { return lineNum; }
   bool isActive() const;
   void setRef() const;
   static void setGlobalRef();
   void addFanin(size_t d) { _faninList.push_back(d); }
   void addFanout(size_t d) { _fanoutList.push_back(d); }
   IdList& getFanin() { return _faninList; }
   IdList& getFanout() { return _fanoutList; }
   CirGate* changeToAIG(size_t ln);
   CirGate* getGate(size_t x) const;
   void setPos(unsigned p) { _pos = p; }
   unsigned getPos() const { return _pos; }
   void setSimVal(size_t sv) { _val = SimValue(sv); }
   const SimValue& getSimVal() const { return _val; }
   void sweep();
   void optimize();
   void strashMerge(CirGate*);
   void fraigMerge(CirGate*);
   void checkPO(CirGate*);

   // Printing functions
   void printGate(bool) const;
   void printFECs() const;
   void reportGate() const;
   void reportFanin(int level) const;
   void reportFanout(int level) const;
   void DFSFanin(int cl, int level, bool isNeg = false) const;
   void DFSFanout(int cl, int level, bool isNeg = false) const;

protected:
   int              _gid;
   size_t           lineNum;
   unsigned         _pos;
   SimValue         _val;
   static CirMgr*   _mgr;
   mutable size_t   _ref;
   static size_t    _globalRef;
   IdList           _faninList;
   IdList           _fanoutList;
};

class CirUNDEFGate: public CirGate
{
public:
   CirUNDEFGate(int id, size_t l) :CirGate(id, l) {}
   virtual bool isUNDEF() const { return true; }
   virtual void setVar(const Var& v) { _var = v; }
   virtual const Var getVar() const { return _var; }
private:
   Var        _var;
};

class CirPIGate: public CirGate
{
public:
   CirPIGate(int id, size_t l) :CirGate(id, l), _sym("") {}
   virtual bool isPI() const { return true; }
   virtual string getTypeStr() const { return "PI"; }
   virtual void setSymbol(string s) { _sym = s; }
   virtual string getSymbol() const { return _sym; }
   virtual void setVar(const Var& v) { _var = v; }
   virtual const Var getVar() const { return _var; }
private:
   Var        _var;
   string     _sym;
};

class CirPOGate: public CirGate
{
public:
   CirPOGate(int id, size_t l) :CirGate(id, l), _sym("") {}
   virtual bool isPO() const { return true; }
   virtual string getTypeStr() const { return "PO"; }
   virtual void setSymbol(string s) { _sym = s; }
   virtual string getSymbol() const { return _sym; }
   virtual void simulate() {
      if (!_faninList.empty()){
          SimValue sv = getGate(_faninList[0])->getSimVal();
          _val = _faninList[0] % 2? !sv: sv;
      }
   }
private:
   string     _sym;
};

class CirAIGGate: public CirGate
{
public:
   CirAIGGate(int id, size_t l) :CirGate(id, l) {}
   virtual bool isAig() const { return true; }
   virtual string getTypeStr() const { return "AIG"; }
   virtual void setVar(const Var& v) { _var = v; }
   virtual const Var getVar() const { return _var; }
   virtual void simulate() {
      short opt = _faninList[0] % 2 * 10 + _faninList[1] % 2;
      SimValue sv0 = getGate(_faninList[0])->getSimVal(),
               sv1 = getGate(_faninList[1])->getSimVal();
      switch (opt){
          case 00: _val = sv0 & sv1;    break;
          case 01: _val = sv0 & !sv1;   break;
          case 10: _val = sv1 & !sv0;   break;
          case 11: _val = !(sv0 | sv1); break;
          default: break;
      }
   }
private:
   Var        _var;
};

class CirCONSTGate: public CirGate
{
public:
   CirCONSTGate(int id, size_t l) :CirGate(id, l) {}
   virtual bool isCONST() const { return true; }   
   virtual string getTypeStr() const { return "CONST"; }
   virtual void setVar(const Var& v) { _var = v; }
   virtual const Var getVar() const { return _var; }
private:
   Var        _var;
};

#endif // CIR_GATE_H
