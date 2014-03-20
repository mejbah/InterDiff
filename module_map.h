#ifndef MODULE_MAP_H
#define MODULE_MAP_H

#include <map>
#include <ext/hash_map>
#include <string>

using namespace std;
using __gnu_cxx::hash_map;
using __gnu_cxx::hash;

class ModuleInfo {
 public: 
  long long _old_base;
  long long _translated_base;
  long long _old_upper_address;
  long long _translated_upper_address;
  string _name;

 public:
  ModuleInfo(long long old_id, long long old_upper_address, string & name);
  void SetTranslated(long long translated_base, long long translated_upper_address);
  long long GetTranslated(long long old_address);
  string * GetName(void);
};

class StringHash {
 public:
  size_t operator() (const string *str) const;
};

class StringEq {
 public:
  bool operator() (const string *str1, const string *str2) const;
};

class LongLongLt {
 public:
  bool operator() (long long a, long long b) const;
};

typedef map<const long long, ModuleInfo*, LongLongLt> ModuleMapByBase;
typedef hash_map<string*, ModuleInfo*, StringHash, StringEq> ModuleMapByName;

class ModuleTranslator {
 private:
  ModuleMapByBase _mmap_by_base;
  ModuleMapByName _mmap_by_name;

 public:
  void InsertModule(long long base, long long upper_address, string *name);
  void SetTranslatedModule(long long base, long long upper_address, string *name);
  long long GetTranslated(long long old_address);
  ~ModuleTranslator();
};

#endif
