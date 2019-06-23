/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include "cirMgr.h"
#include "cirGate.h"
#include "myHashMap.h"
#include "util.h"
//#define ERROR_HANDLE

using namespace std;

// TODO: Implement memeber functions for class CirMgr

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;
IdList writeAIG;

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
static char buf[1024];
static string errMsg;
static int errInt;
static CirGate *errGate;
static unsigned count = 0;

#ifdef ERROR_HANDLE
static bool
parseError(CirParseError err)
{
   switch (err) {
      case EXTRA_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Extra space character is detected!!" << endl;
         break;
      case MISSING_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing space character!!" << endl;
         break;
      case ILLEGAL_WSPACE: // for non-space white space character
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal white space char(" << errInt
              << ") is detected!!" << endl;
         break;
      case ILLEGAL_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
              << errMsg << "!!" << endl;
         break;
      case ILLEGAL_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
              << errMsg << "\"!!" << endl;
         break;
      case ILLEGAL_SYMBOL_TYPE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal symbol type (" << errMsg << ")!!" << endl;
         break;
      case ILLEGAL_SYMBOL_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Symbolic name contains un-printable char(" << errInt
              << ")!!" << endl;
         break;
      case MISSING_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing " << errMsg << "!!" << endl;
         break;
      case MISSING_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
              << errMsg << "\"!!" << endl;
         break;
      case MISSING_NEWLINE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": A new line is expected here!!" << endl;
         break;
      case MISSING_DEF:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
              << " definition!!" << endl;
         break;
      case CANNOT_INVERTED:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": " << errMsg << " " << errInt << "(" << errInt/2
              << ") cannot be inverted!!" << endl;
         break;
      case MAX_LIT_ID:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
              << endl;
         break;
      case REDEF_GATE:
         cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
              << "\" is redefined, previously defined as "
              << errGate->getTypeStr() << " in line " << errGate->getLineNo()
              << "!!" << endl;
         break;
      case REDEF_SYMBOLIC_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
              << errMsg << errInt << "\" is redefined!!" << endl;
         break;
      case REDEF_CONST:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Cannot redefine constant (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_SMALL:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too small (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_BIG:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too big (" << errInt << ")!!" << endl;
         break;
      default: break;
   }
   return false;
}
#endif // ERROR_HANDLE
/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
CirMgr::~CirMgr(){
   for (size_t i = 0; i < _gateList.size(); ++i)
      if (_gateList[i]) delete _gateList[i];
}

CirParseError checkNoDel(ifstream& fin, string errStr){
   int d = fin.peek();
   if (d == ' ') return EXTRA_SPACE;
   if ((errInt = d) == '\t') return ILLEGAL_WSPACE;
   if (d == '\n' || d == '\r' || d == EOF){
      errMsg = errStr;
      return MISSING_NUM;
   }
   return DUMMY_END;
}

CirParseError eatOneSpace(ifstream& fin, string errStr){
   int d = fin.peek();
   if ((errInt = d) == '\t')
      return ILLEGAL_WSPACE;
   if (d == '\n' || d == '\r'){
      errMsg = errStr;
      return MISSING_NUM;
   }
   fin.get(); ++colNo;
   return DUMMY_END;
}

CirParseError eatOneNum(ifstream& fin, string errStr, string& buff, int& d){
   CirParseError err;
   if ((err = eatOneSpace(fin, errStr)) != DUMMY_END) return err;
   if ((err = checkNoDel(fin, errStr))  != DUMMY_END) return err;
   fin >> buff;
   if (!myStr2Int(buff, d) || d < 0){
      errMsg = errStr + "(" + buff + ")";
      return ILLEGAL_NUM;
   }
   return DUMMY_END;
}

bool checkStr(string& s){
   for (size_t i = 0; i < s.length(); ++i)
      if (!isprint(s[i])){
          errInt = s[i];
          return true;
      }
   return false;
}

CirParseError
CirMgr::readFirstLine(ifstream& fin){
   CirParseError err;
   int d = fin.peek();
   string buff;
   if (d == EOF || d == '\n' || d == '\r'){
      errMsg = "aag";
      return MISSING_IDENTIFIER;
   }
   if ((err = checkNoDel(fin, "")) != DUMMY_END) return err;

   // input "aag"
   fin >> buff;
   if ((errMsg = buff) != "aag"){
      if (errMsg.length() > 3 && isdigit(errMsg[3])){
          colNo += 3;
          return MISSING_SPACE;
      }
      else return ILLEGAL_IDENTIFIER;
   }
   colNo += 3;

   // input M
   if ((err = eatOneNum(fin, "number of variables", buff, d)) != DUMMY_END) return err;
   _M = (size_t)d;
   colNo += buff.length();

   // input I
   if ((err = eatOneNum(fin, "number of PIs", buff, d)) != DUMMY_END) return err;
   _PIList.resize(d);
   _inputs.resize(d);
   colNo += buff.length();

   // input L
   if ((err = eatOneNum(fin, "number of latches", buff, d)) != DUMMY_END) return err;
   if (d != 0){
      errMsg = "latches";
      return ILLEGAL_NUM;
   }
   colNo += 1;

   // input O
   if ((err = eatOneNum(fin, "number of POs", buff, d)) != DUMMY_END) return err;
   _POList.resize(d);
   _outputs.resize(d);
   colNo += buff.length();

   // input A
   if ((err = eatOneNum(fin, "number of AIGs", buff, d)) != DUMMY_END) return err;
   if (d + (int)_PIList.size() > (errInt = _M)){
      errMsg = "number of variables";
      return NUM_TOO_SMALL;
   }
   _A = (size_t)d;
   colNo += buff.length();

   d = fin.get();
   if (d != '\n' && d != '\r') return MISSING_NEWLINE;
   ++lineNo; colNo = 0;

   _gateList.resize(_M + _POList.size() + 1);
   memset(&_gateList[0], 0, sizeof(_gateList[0]) * _gateList.size());
   _gateList[0] = new CirCONSTGate(0, 0);
   
   return DUMMY_END;
}

CirParseError
CirMgr::readPI(ifstream& fin){
   CirParseError err;
   string buff;
   
   for (size_t i = 0; i < _PIList.size(); ++i){
      int d = fin.peek();
      if (d == EOF){
          errMsg = "PI";
          return MISSING_DEF;
      }
      if ((err = checkNoDel(fin, "PI literal ID")) != DUMMY_END) return err;
      fin >> buff;
      errMsg = "PI literal ID(" + buff + ")";
      if (myStr2Int(buff, d)){
          errInt = d;
          if (d == 0 || d == 1) return REDEF_CONST;
          if (d > 0 && d % 2){
              errMsg = "PI";
              return CANNOT_INVERTED;
          }
          if (d > (int)_M * 2) return MAX_LIT_ID;
      }
      else return ILLEGAL_NUM;
      if (_gateList[d / 2]){
          errGate = _gateList[d / 2];
          return REDEF_GATE;
      }
      _gateList[d / 2] = new CirPIGate(d / 2, lineNo + 1);
      _PIList[i] = d;
      colNo += buff.length();

      d = fin.get();
      if (d != '\n' && d != '\r') return MISSING_NEWLINE;
      lineNo++; colNo = 0;
   }
   return DUMMY_END;
}

CirParseError
CirMgr::readPO(ifstream& fin){
   CirParseError err;
   string buff;

   for (size_t i = 0; i < _POList.size(); ++i){
      int d = fin.peek();
      if (d == EOF){
          errMsg = "PO";
          return MISSING_DEF;
      }
      if ((err = checkNoDel(fin, "PO literal ID")) != DUMMY_END) return err;
      fin >> buff;
      errMsg = "PO literal ID(" + buff + ")";
      if (!myStr2Int(buff, d) || d < 0) return ILLEGAL_NUM;
      if ((errInt = d) > (int)_M * 2 + 1) return MAX_LIT_ID;
      _gateList[_M + 1 + i] = new CirPOGate(_M + 1 + i, lineNo + 1);
      _POList[i] = d;
      colNo += buff.length();

      d = fin.get();
      if (d != '\n' && d != '\r') return MISSING_NEWLINE;
      lineNo++; colNo = 0;
   }
   return DUMMY_END;
}

CirParseError
CirMgr::readAIG(ifstream& fin){
   CirParseError err;
   string buff;

   for (size_t i = 0; i < _A; ++i){
      int d = fin.peek(), AIGid;
      if (d == EOF){
          errMsg = "AIG";
          return MISSING_DEF;
      }
      if ((err = checkNoDel(fin, "AIG gate literal ID")) != DUMMY_END) return err;
      fin >> buff;
      errMsg = "AIG gate literal ID(" + buff + ")";
      if (myStr2Int(buff, d)){
          errInt = d;
          if (d == 0 || d == 1) return REDEF_CONST;
          if (d > 0 && d % 2){
              errMsg = "AIG gate";
              return CANNOT_INVERTED;
          }
          if (d > (int)_M * 2) return MAX_LIT_ID;
      }
      else return ILLEGAL_NUM;

      // generate AIG
      if ((errGate = _gateList[d / 2])){
          if (_gateList[d / 2]->isUNDEF()){
              CirGate* temp = _gateList[d / 2];
              _gateList[d / 2] = _gateList[d / 2]->changeToAIG(lineNo + 1);
              delete temp;
          }
          else return REDEF_GATE;
      }
      else _gateList[d / 2] = new CirAIGGate(d / 2, lineNo + 1);
      AIGid = d;
      colNo += buff.length();

      for (int j = 0; j < 2; ++j){
          if (fin.get() != ' ') return MISSING_SPACE;
          ++colNo;

          if ((err = checkNoDel(fin, "AIG input literal ID")) != DUMMY_END) return err;
          fin >> buff;
          errMsg = "AIG input literal ID(" + buff + ")";
          if (!myStr2Int(buff, d) || d < 0) return ILLEGAL_NUM;
          if ((errInt = d) > (int)_M * 2 + 1) return MAX_LIT_ID;

          _gateList[AIGid / 2]->addFanin(d);
          if (!_gateList[d / 2]) _gateList[d / 2] = new CirUNDEFGate(d / 2, 0);
          _gateList[d / 2]->addFanout(AIGid + d % 2);
          colNo += buff.length();
      }
      d = fin.get();
      if (d != '\n' && d != '\r') return MISSING_NEWLINE;
      lineNo++; colNo = 0;
   }

   // connect PO with AIG
   for (size_t i = 0; i < _POList.size(); ++i){
      size_t po = _POList[i];
      if (!_gateList[po / 2]) _gateList[po / 2] = new CirUNDEFGate(po / 2, 0);
      _gateList[po / 2]->addFanout(2 * (_M + 1 + i) + po % 2);
      _gateList[_M + 1 + i]->addFanin(po);
   }
   return DUMMY_END;
}

CirParseError
CirMgr::readSymbol(ifstream& fin){
   CirParseError err;
   string buff;

   int d = fin.get(), e;
   char c;
   while (d != EOF && d != 'c'){
      c = d;
      if (c == ' ') return EXTRA_SPACE;
      if ((errInt = c) == '\t') return ILLEGAL_WSPACE;
      if (c != 'i' && c != 'o'){
          errMsg = c;
          return ILLEGAL_SYMBOL_TYPE;
      }
      ++colNo;

      if ((err = checkNoDel(fin, "symbol index")) != DUMMY_END) return err;
      fin >> buff;
      errMsg = "symbol index(" + buff + ")";
      if (!myStr2Int(buff, d) || d < 0) return ILLEGAL_NUM;
      if ((c == 'i' && d >= (int)_PIList.size()) ||
          (c == 'o' && d >= (int)_POList.size())){
          errInt = d;
          string p = (c == 'i'? "PI": "PO");
          errMsg = p + " index";
          return NUM_TOO_BIG;
      }
      if ((c == 'i' && !_gateList[_PIList[d] / 2]->getSymbol().empty()) ||
          (c == 'o' && !_gateList[_M + 1 + d]->getSymbol().empty())){
          errMsg = c + to_string(d);
          return REDEF_SYMBOLIC_NAME;
      }
      colNo += buff.length();

      if (fin.get() != ' ') return MISSING_SPACE;
      ++colNo;

      e = fin.peek();
      if (e == '\n' || e == '\r' || e == EOF){
          errMsg = "symbol name";
          return MISSING_IDENTIFIER;
      }
      fin.get(buf, 1024, '\n');
      buff = buf;
      if (checkStr(buff)) return ILLEGAL_SYMBOL_NAME;
      if (c == 'i') _gateList[_PIList[d] / 2]->setSymbol(buff);
      if (c == 'o') _gateList[_M + 1 + d]->setSymbol(buff);
      colNo += buff.length();

      d = fin.get();
      if (d != '\n' && d != '\r') return MISSING_NEWLINE;
      ++lineNo; colNo = 0;
      d = fin.get();
   }
   e = fin.get();
   if (d == 'c' && e != '\n' && e != '\r') return MISSING_NEWLINE;
   return DUMMY_END;
}

void
CirMgr::readNoError(ifstream& fin){
   string buff;
   size_t I, O, t;
   fin >> buff >> _M >> I >> t >> O >> _A;  // read first line
   _gateList.resize(_M + 1 + O);
   memset(&_gateList[0], 0, sizeof(_gateList[0]) * _gateList.size());
   _gateList[0] = new CirCONSTGate(0, 0);
   _PIList.resize(I);
   _inputs.resize(I);
   _POList.resize(O);
   _outputs.resize(O);
   ++lineNo;
   for (size_t i = 0; i < I; ++i){  // read PI
      fin >> t;
      _PIList[i] = t;
      _gateList[t / 2] = new CirPIGate(t / 2, lineNo + 1);
      ++lineNo;
   }
   for (size_t i = 0; i < O; ++i){  // read PO
      fin >> t;
      _POList[i] = t;
      CirGate* po = new CirPOGate(_M + 1 + i, lineNo + 1);
      po->addFanin(t);
      _gateList[_M + 1 + i] = po;
      ++lineNo;
   }
   for (size_t i = 0; i < _A; ++i){ // read AIG
      fin >> t;
      CirGate* g = new CirAIGGate(t / 2, lineNo + 1);
      _gateList[t / 2] = g;
      for (size_t j = 0; j < 2; ++j){
          fin >> t; g->addFanin(t);
      }
      ++lineNo;
   }
   for (size_t i = 0; i < _gateList.size(); ++i){ // connect all
      if (!_gateList[i]) continue;
      IdList& fanin = _gateList[i]->getFanin();
      for (size_t j = 0; j < fanin.size(); ++j){
          if (!_gateList[fanin[j] / 2])
              _gateList[fanin[j] / 2] = new CirUNDEFGate(fanin[j] / 2, 0);
          _gateList[fanin[j] / 2]->addFanout(i * 2 + fanin[j] % 2);
      }
   }
   char c;
   while (fin >> c){  // read symbol
      if (c == 'c') return;
      fin >> t >> buff;
      if (c == 'i') _gateList[_PIList[t] / 2]->setSymbol(buff);
      if (c == 'o') _gateList[_M + 1 + t]->setSymbol(buff);
   }
}

bool
CirMgr::readCircuit(const string& fileName)
{
   CirGate::_mgr = this;
   FecGrp::_mgr = this;
   lineNo = colNo = 0;
   ifstream fin(fileName.c_str());
   if (!fin){
      cerr << "Cannot open design \"" + fileName + "\"\n";
      return false;
   }
   
   #ifdef ERROR_HANDLE
   CirParseError err;
   if ((err = readFirstLine(fin)) != DUMMY_END) return parseError(err);
   if ((err = readPI(fin))        != DUMMY_END) return parseError(err);
   if ((err = readPO(fin))        != DUMMY_END) return parseError(err);
   if ((err = readAIG(fin))       != DUMMY_END) return parseError(err);
   if ((err = readSymbol(fin))    != DUMMY_END) return parseError(err);
   #else
   readNoError(fin);
   #endif // ERROR_HANDLE

   setDFS();
   setFU(); // set gates with floating fanin & unused gates

   return true;
}

void
CirMgr::setDFS(){
   _gateList[0]->setPos(0);
   ::count = 1;
   _DFSList.clear();
   for (size_t i = 0; i < _POList.size(); i++)
      findNonFloating(_gateList[_M + 1 + i]);
   CirGate::setGlobalRef();
}

void
CirMgr::findNonFloating(CirGate* gate){
   if (!gate->isActive()){
      gate->setRef();
      IdList v = gate->getFanin();
      for (size_t i = 0; i < v.size(); i++)
          findNonFloating(_gateList[v[i] / 2]);
      if (gate->isAig()){
          gate->setPos(::count++);
          _DFSList.push_back(gate->getId());
      }
   }
}

void
CirMgr::setFU(){
   _floatFaninList.clear(); _unusedList.clear();
   for (size_t i = 1; i < _gateList.size(); ++i){
      if (_gateList[i]){
        IdList fanin = _gateList[i]->getFanin(), fanout = _gateList[i]->getFanout();
        for (size_t j = 0; j < fanin.size(); ++j)
          if (_gateList[fanin[j] / 2]->isUNDEF())
            { _floatFaninList.push_back(i); break; }
        if (!_gateList[i]->isPO() && fanout.empty())
            _unusedList.push_back(i);
      }
   }
}


void
CirMgr::removeNotInDFSfromFECs()
{
   _gateList[0]->setPos(0);
   ::count = 1;
   _DFSList.clear();
   for (size_t i = 0; i < _POList.size(); i++)
      findNonFloating(_gateList[_M + 1 + i]);
   for (size_t i = 0; i < _PIList.size(); ++i)
      _gateList[_PIList[i] / 2]->setRef();
   for (size_t i = 1; i <= _M; ++i)
      if (_gateList[i] && !_gateList[i]->isActive())
          if (_fecFriends.find(i) != _fecFriends.end()){
            _fecFriends[i]->erase(i);
            _fecFriends.erase(i);
         }
   CirGate::setGlobalRef();
}

/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      162
*********************/
void
CirMgr::printSummary() const
{
   cout << endl;
   cout << "Circuit Statistics" << endl;
   cout << "==================" << endl;
   cout << "  " << setw(3) << left << "PI"  << setw(11) << right << _PIList.size() << endl;
   cout << "  " << setw(3) << left << "PO"  << setw(11) << right << _POList.size() << endl;
   cout << "  " << setw(3) << left << "AIG" << setw(11) << right << _A << endl;
   cout << "------------------" << endl;
   cout << "  Total" << setw(9) << right << _PIList.size() + _POList.size() + _A << endl;
}

void
CirMgr::printNetlist() const
{
   ::count = 0;
   cout << endl;
   for (size_t i = 0; i < _POList.size(); ++i)
      postorder(_gateList[_M + 1 + i]);
   CirGate::setGlobalRef();
}

void
CirMgr::printPIs() const
{
   cout << "PIs of the circuit:";
   for (size_t i = 0; i < _PIList.size(); ++i)
      cout << ' ' << _PIList[i] / 2;
   cout << endl;
}

void
CirMgr::printPOs() const
{
   cout << "POs of the circuit:";
   for (size_t i = 0; i < _POList.size(); ++i)
      cout << ' ' << _M + 1 + i;
   cout << endl;
}

void
CirMgr::printFloatGates() const
{
   if (!_floatFaninList.empty()){
      cout << "Gates with floating fanin(s):";
      for (size_t i = 0; i < _floatFaninList.size(); ++i)
          cout << ' ' << _floatFaninList[i];
      cout << endl;
   }
   if (!_unusedList.empty()){
      cout << "Gates defined but not used  :";
      for (size_t i = 0; i < _unusedList.size(); ++i)
          cout << ' ' << _unusedList[i];
      cout << endl;
   }
}

void
CirMgr::printFECPairs()
{
   for (size_t i = 0; i < _fecGrps.size(); ++i) _fecGrps[i]->sort();
   sort(_fecGrps.begin(), _fecGrps.end(), FecCompare());
   for (size_t i = 0; i < _fecGrps.size(); ++i)
      cout << '[' << i << ']' << (*_fecGrps[i]);
}

void
CirMgr::writeAag(ostream& outfile) const
{  
   outfile << "aag " << _M << ' ' << _PIList.size() << " 0 " 
           << _POList.size() << ' ' << _DFSList.size() << endl;
   for (size_t i = 0; i < _PIList.size(); ++i) outfile << _PIList[i] << endl;
   for (size_t i = 0; i < _POList.size(); ++i) outfile << _POList[i] << endl;
   for (size_t i = 0; i < _DFSList.size(); ++i){
      outfile << _DFSList[i] * 2;
      IdList v = _gateList[_DFSList[i]]->getFanin();
      for (size_t j = 0; j < v.size(); ++j) outfile << ' ' << v[j];
      outfile << endl;
   }
   for (size_t i = 0; i < _PIList.size(); ++i)
      if (!_gateList[_PIList[i] / 2]->getSymbol().empty())
          outfile << 'i' << i << ' ' << _gateList[_PIList[i] / 2]->getSymbol() << endl;
   for (size_t i = 0; i < _POList.size(); ++i)
      if (!_gateList[_M + i + 1]->getSymbol().empty())
          outfile << 'o' << i << ' ' << _gateList[_M + i + 1]->getSymbol() << endl;
   outfile << "c\nAAG output by Pei-Wei (Perry) Chen\n";
}

void 
CirMgr::postorder(CirGate* gate) const
{
   IdList fanin = gate->getFanin();
   for (size_t i = 0; i < fanin.size(); ++i){
      int id = fanin[i] / 2;
      if (!_gateList[id]->isActive() && 
          !_gateList[id]->isUNDEF())
          postorder(_gateList[id]);
   }

   cout << "[" << ::count << "] " << setw(4) << left << gate->getTypeStr() << gate->getId();
   for (size_t i = 0; i < fanin.size(); ++i)
      cout << ' ' << (_gateList[fanin[i] / 2]->isUNDEF()? "*":"") 
           << (fanin[i] % 2? "!":"") << fanin[i] / 2;
   if (!gate->getSymbol().empty()) cout << " (" << gate->getSymbol() << ")";
   cout << endl;
   gate->setRef();
   ++::count;
}

void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{
   writeAIG.clear();
   ::count = 0;
   errInt = 0;
   writeDFS(g);
   outfile << "aag " << errInt << ' ' << ::count 
           << " 0 1 " << writeAIG.size() << endl;
   for (size_t i = 0; i < _PIList.size(); ++i){
      if (_gateList[_PIList[i] / 2]->isActive())
          outfile << _PIList[i] << endl;
   }
   outfile << g->getId() * 2 << endl;
   for (size_t i = 0; i < writeAIG.size(); ++i){
      outfile << writeAIG[i] * 2;
      IdList v = _gateList[writeAIG[i]]->getFanin();
      for (size_t j = 0; j < v.size(); ++j) outfile << ' ' << v[j];
      outfile << endl;
   }
   ::count = 0;
   for (size_t i = 0; i < _PIList.size(); ++i){
      CirGate* g = _gateList[_PIList[i] / 2];
      if (g->isActive() && !g->getSymbol().empty())
          outfile << 'i' << ::count++ << ' ' << g->getSymbol() << endl;
   }
   outfile << "o0 " << g->getId() << "\nc\n";
   outfile << "Write gate (" << g->getId() << ") by Pei-Wei (Perry) Chen\n";
   CirGate::setGlobalRef();
}

void
CirMgr::writeDFS(CirGate* g) const
{
   if (g->isActive() || g->isCONST() || g->isUNDEF()) return;
   if (g->getId() > errInt) errInt = g->getId();
   if (g->isPI()) {
      ++::count;
      g->setRef(); return;
   }
   IdList& fanin = g->getFanin();
   for (size_t i = 0; i < fanin.size(); ++i)
      writeDFS(_gateList[fanin[i] / 2]);
   writeAIG.push_back(g->getId());
   g->setRef();
}

/*************************************/
/*   class FecGrp member functions   */
/*************************************/
CirMgr* FecGrp::_mgr = cirMgr;

void
FecGrp::setFECs(FecHash& h) {
   for (size_t i = 0; i < _data.size(); ++i)
      h[_data[i] / 2] = this;
   setBase();
}

void
FecGrp::setBase() {
   _base = _data[0];
   for (size_t i = 1; i < _data.size(); ++i)
      if (_mgr->getGate(_data[i] / 2)->getPos()
          < _mgr->getGate(_base / 2)->getPos())
          _base = _data[i];
}

void
FecGrp::erase(unsigned x) {
   for (size_t i = 0; i < _data.size(); ++i){
      if (_data[i] / 2 == x){
          bool isBase = _data[i] == _base;
          _data[i] = _data.back();
          _data.pop_back();
          if (isBase) setBase();
          break;
      }
   }
}

void
FecGrp::printFECs(unsigned t) {
   size_t r = 0;
   ::sort(_data.begin(), _data.end());
   for (size_t i = 0; i < _data.size(); ++i)
      if (_data[i] / 2 == t){ r = _data[i] % 2; break; }
   for (size_t i = 0; i < _data.size(); ++i)
      if (_data[i] / 2 != t)
          cout << ' ' << (_data[i] % 2 == r? "":"!") << _data[i] / 2;
}