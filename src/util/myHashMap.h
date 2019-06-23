/****************************************************************************
  FileName     [ myHashMap.h ]
  PackageName  [ util ]
  Synopsis     [ Define HashMap and Cache ADT ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2009-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef MY_HASH_MAP_H
#define MY_HASH_MAP_H

#include <vector>
#define fst first
#define snd second

using namespace std;

// TODO: (Optionally) Implement your own HashMap and Cache classes.

//-----------------------
// Define HashMap classes
//-----------------------
// To use HashMap ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//

template <class HashKey, class HashData>
class HashMap
{
typedef pair<HashKey, HashData> HashNode;

public:
   HashMap(size_t b=0) : _numBuckets(0), _buckets(0) { if (b != 0) init(b); }
   ~HashMap() { reset(); }

   // [Optional] TODO: implement the HashMap<HashKey, HashData>::iterator
   // o An iterator should be able to go through all the valid HashNodes
   //   in the HashMap
   // o Functions to be implemented:
   //   - constructor(s), destructor
   //   - operator '*': return the HashNode
   //   - ++/--iterator, iterator++/--
   //   - operators '=', '==', !="
   //
   class iterator
   {
      friend class HashMap<HashKey, HashData>;
   public:
      iterator(size_t n, size_t cb, vector<HashNode>* v, typename vector<HashNode>::iterator it)
         :_numBuckets(n), _cB(cb), _buckets(v), _it(it) {}
      const HashData& operator * () const { return (*_it).second; }
      HashData& operator * () { return (*_it).second; }
      iterator& operator ++ () {
         if (_buckets[_cB].end() != ++_it) return *this;
         for (++_cB; _cB < _numBuckets; ++_cB)
            if (!_buckets[_cB].empty()) { _it = _buckets[_cB].begin(); break; }
         if (_cB == _numBuckets) { _cB = _numBuckets - 1; _it = _buckets[_cB].end(); }
         return *this;
      }
      iterator operator ++ (int) { iterator temp = *this; ++(*this); return temp; }
      bool operator == (const iterator& i) const { return _cB == i._cB && _it == i._it; }
      bool operator != (const iterator& i) const { return _cB != i._cB || _it != i._it; }
   private:
      size_t               _numBuckets;
      size_t               _cB;
      vector<HashNode>*    _buckets;
      typename vector<HashNode>::iterator _it;
   };

   void init(size_t b) {
      reset(); _numBuckets = b; _buckets = new vector<HashNode>[b]; }
   void reset() {
      _numBuckets = 0;
      if (_buckets) { delete [] _buckets; _buckets = 0; }
   }
   void clear() {
      for (size_t i = 0; i < _numBuckets; ++i) _buckets[i].clear();
   }
   size_t numBuckets() const { return _numBuckets; }

   vector<HashNode>& operator [] (size_t i) { return _buckets[i]; }
   const vector<HashNode>& operator [](size_t i) const { return _buckets[i]; }

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

   // check if k is in the hash...
   // if yes, return true;
   // else return false;
   bool check(const HashKey& k) const {
      size_t b = bucketNum(k);
      for (size_t i = 0; i < _buckets[b].size(); ++i)
         if (k == _buckets[b][i].fst) return true;
      return false;
   }

   // query if k is in the hash...
   // if yes, replace d with the data in the hash and return true;
   // else return false;
   bool query(const HashKey& k, HashData& d) const {
      size_t b = bucketNum(k);
      for (size_t i = 0; i < _buckets[b].size(); ++i)
         if (k == _buckets[b][i].fst){
            d = _buckets[b][i].snd;
            return true;
         }
      return false;
   }

   // update the entry in hash that is equal to k (i.e. == return true)
   // if found, update that entry with d and return true;
   // else insert d into hash as a new entry and return false;
   bool update(const HashKey& k, HashData& d) {
      size_t b = bucketNum(k);
      for (size_t i = 0; i < _buckets[b].size(); ++i){
         if (k == _buckets[b][i].fst){
            _buckets[b][i].snd = d;
            return true;
         }
      }
      insert(k, d);
      return false;
   }

   // return true if inserted d successfully (i.e. k is not in the hash)
   // return false is k is already in the hash ==> will not insert
   bool insert(const HashKey& k, const HashData& d) {
      if (check(k)) return false;
      _buckets[bucketNum(k)].push_back(make_pair(k, d));
      return true;
   }

   // return true if removed successfully (i.e. k is in the hash)
   // return fasle otherwise (i.e. nothing is removed)
   bool remove(const HashKey& k) {
      size_t b = bucketNum(k);
      for (size_t i = 0; i < _buckets[b].size(); ++i){
         if (k == _buckets[b][i]){
            _buckets[b][i] = _buckets[b].back();
            _buckets[b].pop_back();
            return true;
         }
      }
      return false;
   }

private:
   // Do not add any extra data member
   size_t                   _numBuckets;
   vector<HashNode>*        _buckets;

   size_t bucketNum(const HashKey& k) const {
      return (k() % _numBuckets); }

};


//---------------------
// Define Cache classes
//---------------------
// To use Cache ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//
// class CacheKey
// {
// public:
//    CacheKey() {}
//    
//    size_t operator() () const { return 0; }
//   
//    bool operator == (const CacheKey&) const { return true; }
//       
// private:
// }; 
// 
template <class CacheKey, class CacheData>
class Cache
{
typedef pair<CacheKey, CacheData> CacheNode;

public:
   Cache() : _size(0), _cache(0) {}
   Cache(size_t s) : _size(0), _cache(0) { init(s); }
   ~Cache() { reset(); }

   // NO NEED to implement Cache::iterator class

   // TODO: implement these functions
   //
   // Initialize _cache with size s
   void init(size_t s) { reset(); _size = s; _cache = new CacheNode[s]; }
   void reset() {  _size = 0; if (_cache) { delete [] _cache; _cache = 0; } }

   size_t size() const { return _size; }

   CacheNode& operator [] (size_t i) { return _cache[i]; }
   const CacheNode& operator [](size_t i) const { return _cache[i]; }

   // return false if cache miss
   bool read(const CacheKey& k, CacheData& d) const {
      size_t i = k() % _size;
      if (k == _cache[i].first) {
         d = _cache[i].second;
         return true;
      }
      return false;
   }
   // If k is already in the Cache, overwrite the CacheData
   void write(const CacheKey& k, const CacheData& d) {
      size_t i = k() % _size;
      _cache[i].first = k;
      _cache[i].second = d;
   }

private:
   // Do not add any extra data member
   size_t         _size;
   CacheNode*     _cache;
};


#endif // MY_HASH_H
