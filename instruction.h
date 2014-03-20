#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <ext/hash_map>
#include <ext/hash_set>
#include <set>
#include <vector>
#include <iostream>
#include <string>

using namespace std;
using __gnu_cxx::hash_set;
using __gnu_cxx::hash_map;
using __gnu_cxx::hash;

typedef unsigned long pc_type;
typedef long line_no_type;
typedef unsigned long addr_type;
typedef unsigned long access_count_type;

static const addr_type INVALID_ADDR = 0;
const access_count_type INFINITY = ~0;

enum ITER_TYPE {INIT, MIDDLE, END, DONT_CARE};

enum LINE_TYPE {EOF_TYPE, LOCK_TYPE, UNLOCK_TYPE, NORMAL_TYPE, FREE_TYPE};
  
class AddrLt {
 public:
  bool operator()(const addr_type a, const addr_type b)const;
};

class  AddrEq {
 public:
  bool operator () (const addr_type a, const addr_type b) const;
};

class AccessCountLt {
 public:
   bool operator()(const access_count_type a, const access_count_type b)const;
};

// Find nth occurance of c in s
size_t find_nth(const string& s, char c, int n);

#endif  // end of class

