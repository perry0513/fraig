/****************************************************************************
  FileName     [ myHashSet.h ]
  PackageName  [ util ]
  Synopsis     [ Define HashSet ADT ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2014-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef MY_HASH_SET_H
#define MY_HASH_SET_H

#include <vector>

using namespace std;

//---------------------
// Define HashSet class
//---------------------
// To use HashSet ADT,
// the class "Data" should at least overload the "()" and "==" operators.
//
// "operator ()" is to generate the hash key (size_t)
// that will be % by _numBuckets to get the bucket number.
// ==> See "bucketNum()"
//
// "operator ==" is to check whether there has already been
// an equivalent "Data" object in the HashSet.
// Note that HashSet does not allow equivalent nodes to be inserted
//
template <class Data>
class HashSet
{
public:
   HashSet(size_t b = 0) : _numBuckets(0), _buckets(0) { if (b != 0) init(b); }
   ~HashSet() { reset(); }

   // TODO: implement the HashSet<Data>::iterator
   // o An iterator should be able to go through all the valid Data
   //   in the Hash
   // o Functions to be implemented:
   //   - constructor(s), destructor
   //   - operator '*': return the HashNode
   //   - ++/--iterator, iterator++/--
   //   - operators '=', '==', !="
   //
   class iterator
   {
      friend class HashSet<Data>;

   public:
      iterator(size_t n, size_t cb, vector<Data>* v, typename vector<Data>::iterator it)
         :_numBuckets(n), _cB(cb), _buckets(v), _it(it) {}
      const Data& operator * () const { return *_it; }
      Data& operator * () { return *_it; }
      iterator& operator ++ () {
         if (_buckets[_cB].end() != ++_it) return *this;
         for (++_cB; _cB < _numBuckets; ++_cB)
            if (!_buckets[_cB].empty()) { _it = _buckets[_cB].begin(); break; }
         if (_cB == _numBuckets) { _cB = _numBuckets - 1; _it = _buckets[_cB].end(); }
         return *this;
      }
      iterator operator ++ (int) { iterator temp = *this; ++(*this); return temp; }
      iterator& operator -- () {
         if (_it != _buckets[_cB].begin()) { --_it; return *this; }
         for (size_t i; i >= 0; --i)
            if (!_buckets[i].empty()) { _cB = i; _it = _buckets[_cB].end(); break; }
         return *this;
      }
      iterator operator -- (int) { iterator temp = *this; --(*this); return temp; }
      bool operator == (const iterator& i) const { return _cB == i._cB && _it == i._it; }
      bool operator != (const iterator& i) const { return _cB != i._cB || _it != i._it; }
   private:
      size_t         _numBuckets;
      size_t         _cB;
      vector<Data>*  _buckets;
      typename vector<Data>::iterator _it;
   };

   void init(size_t b) { _numBuckets = b; _buckets = new vector<Data>[b]; }
   void reset() {
      _numBuckets = 0;
      if (_buckets) { delete [] _buckets; _buckets = 0; }
   }
   void clear() {
      for (size_t i = 0; i < _numBuckets; ++i) _buckets[i].clear();
   }
   size_t numBuckets() const { return _numBuckets; }

   vector<Data>& operator [] (size_t i) { return _buckets[i]; }
   const vector<Data>& operator [](size_t i) const { return _buckets[i]; }

   // TODO: implement these functions
   //
   // Point to the first valid data
   iterator begin() const {
      size_t b = 0;
      for (; b < _numBuckets; b++)
         if (!_buckets[b].empty()) break;
      if (b == _numBuckets) return end();
      return iterator(_numBuckets, b, _buckets, _buckets[b].begin());
   }
   // Pass the end
   iterator end() const {
      return iterator(_numBuckets, _numBuckets - 1, _buckets, _buckets[_numBuckets - 1].end()); }
   // return true if no valid data
   bool empty() const { return size() == 0; }
   // number of valid data
   size_t size() const {
      size_t size = 0;
      for (size_t i = 0; i < _numBuckets; ++i)
         size += _buckets[i].size();
      return size;
   }

   // check if d is in the hash...
   // if yes, return true;
   // else return false;
   bool check(const Data& d) const {
      size_t b = bucketNum(d);
      for (size_t i = 0; i < _buckets[b].size(); ++i)
         if (d == _buckets[b][i]) return true;
      return false;
   }

   // query if d is in the hash...
   // if yes, replace d with the data in the hash and return true;
   // else return false;
   bool query(Data& d) const {
      size_t b = bucketNum(d);
      for (size_t i = 0; i < _buckets[b].size(); ++i){
         if (d == _buckets[b][i]){
            d = _buckets[b][i];
            return true;
         }
      }
      return false;
   }

   // update the entry in hash that is equal to d (i.e. == return true)
   // if found, update that entry with d and return true;
   // else insert d into hash as a new entry and return false;
   bool update(const Data& d) {
      size_t b = bucketNum(d);
      for (size_t i = 0; i < _buckets[b].size(); ++i){
         if (d == _buckets[b][i]){
            _buckets[b][i] = d;
            return true;
         }
      }
      insert(d);
      return false;
   }

   // return true if inserted successfully (i.e. d is not in the hash)
   // return false if d is already in the hash ==> will not insert
   bool insert(const Data& d) {
      if (check(d)) return false;
      _buckets[bucketNum(d)].push_back(d);
      return true;
   }

   // return true if removed successfully (i.e. d is in the hash)
   // return false otherwise (i.e. nothing is removed)
   bool remove(const Data& d) {
      size_t b = bucketNum(d);
      for (size_t i = 0; i < _buckets[b].size(); ++i){
         if (d == _buckets[b][i]){
            _buckets[b][i] = _buckets[b].back();
            _buckets[b].pop_back();
            return true;
         }
      }
      return false;
   }

private:
   // Do not add any extra data member
   size_t            _numBuckets;
   vector<Data>*     _buckets;

   size_t bucketNum(const Data& d) const {
      return (d() % _numBuckets); }
};

#endif // MY_HASH_SET_H
