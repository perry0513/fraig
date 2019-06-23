/****************************************************************************
  FileName     [ cirDef.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic data or var for cir package ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_DEF_H
#define CIR_DEF_H

#include <vector>
#include <algorithm>
#include <unordered_map>
#include "myHashMap.h"

#define LOG_DEBUG

using namespace std;

// TODO: define your own typedef or enum

class CirGate;
class CirMgr;
class SatSolver;

typedef vector<CirGate*>           GateList;
typedef vector<unsigned>           IdList;

class SimValue
{
public:
   #define INV 0xFFFFFFFFFFFFFFFF
   SimValue(size_t v = 0): _v(v) {}

   size_t operator () () const { return _v; }
   bool operator == (const SimValue& sv) const { return _v == sv._v; }
   SimValue operator & (const SimValue& sv) const { return SimValue(_v & sv._v); }
   SimValue operator | (const SimValue& sv) const { return SimValue(_v | sv._v); }
   SimValue operator ! () const { return SimValue(_v ^ INV); }

   bool isInv (const SimValue& sv) const { return (!(*this)) == sv; }

   friend ostream& operator << (ostream& out, const SimValue& sv){
      size_t mask = 1, t = sv._v;
      for (int i = 1; i <= 64; ++i){
          out << (t & mask? 1:0);
          mask <<= 1;
          if (i % 8 == 0 && i != 64) out << '_';
      }
      return out;
   }
private:
   size_t _v;
};

struct SimHasher
{
   size_t operator () (const SimValue& sv) const { return sv(); }
};

#endif // CIR_DEF_H
