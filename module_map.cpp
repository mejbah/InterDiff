#include "module_map.h"
#include <iostream>
#include <assert.h>

ModuleInfo::ModuleInfo(long long old_base, long long old_upper_address, string & name) : _old_base(old_base),
                                                    _old_upper_address(old_upper_address),
                                                    _name(name) {
}

void ModuleInfo::SetTranslated(long long translated_base, long long translated_upper_address) {
  _translated_base = translated_base;
  _translated_upper_address = translated_upper_address;
  assert((_translated_upper_address - _translated_base) == 
         (_old_upper_address - _old_base));
}

long long ModuleInfo::GetTranslated(long long old_address) {
  assert((old_address >= _old_base) && (old_address <= _old_upper_address));
  long long  _translated_address = old_address - _old_base + _translated_base;
  assert( _translated_address <= _translated_upper_address);
//  if(_translated_address <= _translated_upper_address)
//	std::cout << "Assertion error : " << "old_address " << old_address <<" old_base "<< _old_base <<"::: Module name : " << *GetName()<< std::endl;
  return _translated_address;
}

string * ModuleInfo::GetName(void) {
  return &_name;
}

bool LongLongLt::operator()(long long a, long long b) const {
  return a < b;
}

size_t StringHash::operator() (const string *str) const {
  hash<char> H;
  size_t result = 0;
  for (int i = 0; i < str->length(); i++)
    result += H((*str)[i]);
  return result;
}

bool StringEq::operator()(const string *str1, const string *str2) const {
  if (str1->compare(*str2) == 0)
    return true;
  else
    return false;  
}

void ModuleTranslator::InsertModule(long long base, long long upper_address, string *name) {
  if (_mmap_by_base.find(base) == _mmap_by_base.end()) {
    ModuleInfo *mi = new ModuleInfo(base, upper_address, *name);
    cerr << "Inserted Module " << *name << endl;
    _mmap_by_base[base] = mi;
    _mmap_by_name[mi->GetName()] = mi;
  }
}

void ModuleTranslator::SetTranslatedModule(long long base, long long upper_address, string *name) {
  if (_mmap_by_name.find(name) == _mmap_by_name.end()) {
    cerr << "Original module not found!!!" << endl;
    cerr << *name << endl;
    exit(-1);
  }
  _mmap_by_name[name]->SetTranslated(base, upper_address);
}

long long ModuleTranslator::GetTranslated(long long old_address) {
  ModuleMapByBase::iterator it;
  if ((it = _mmap_by_base.lower_bound(old_address)) 
      == _mmap_by_base.end()) {
    if (_mmap_by_base.size() == 0) {
      cerr << "Unknown module!!!" << endl;
      return old_address;
      //exit(-1);
    }
    ModuleMapByBase::iterator end_1_it = _mmap_by_base.end();
    end_1_it--;
    if (end_1_it->second->_old_upper_address < old_address) {
      cerr << "Unknown module!!!" << endl;
     // exit(-1);
     return old_address;
    }
  }
  if (it == _mmap_by_base.begin()) {
    cerr << "Unknown module!!!" << endl;
    //exit(-1);
    return old_address;
  }
  it--;
  return it->second->GetTranslated(old_address);
}

ModuleTranslator::~ModuleTranslator() {
  for (ModuleMapByBase::iterator it = _mmap_by_base.begin(); 
       it != _mmap_by_base.end(); ) {
    ModuleInfo *mi = it->second;
    ++it;
    delete mi;
  }
}
