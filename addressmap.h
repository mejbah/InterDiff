#ifndef ADDRESS_MAP_H
#define ADDRESS_MAP_H

#include "instruction.h"
#include <map>
#include <vector>
#include <utility>

using namespace std;

extern bool dbg_flag;

class AddressTranslator {
 private:
  addr_type _next;
  addr_type _safety_margin;
 public:
  AddressTranslator();
  addr_type NextAddr(size_t size);
};

typedef map<const access_count_type, addr_type, AccessCountLt> TranslatorMap;

typedef pair<access_count_type, addr_type> LifeRange;
typedef vector<LifeRange *> LifeRangeVector;
class AddressMarker {
 public:
  static AddressTranslator address_translator;
  addr_type _start_addr, _end_addr;
  LifeRangeVector _life_range_vector;
 public:
  void Set(addr_type start_addr, addr_type end_addr);
  void InsertLifeRange(access_count_type access_counter);
  addr_type Translate(addr_type address, access_count_type access_counter);
  AddressMarker(addr_type start_addr, addr_type end_addr);
  ~AddressMarker();
  void Print();
};

typedef vector<AddressMarker *> AddressMarkerVector;
class AddressMarkerVectorHandler {
 public:
  AddressMarkerVector _address_marker_vector; 
  vector<pair<addr_type, size_t> *> _malloc_vector;
 private:
  void InsertRange(addr_type s, addr_type e);
 public:
  void Allocate(addr_type addr, size_t size);
  void Free(addr_type addr, access_count_type access_counter);
  ~AddressMarkerVectorHandler();
  //Returns true if address is present; false otherwise
  bool IsPresent(addr_type address, addr_type &translated_addr,
                 access_count_type access_counter);
  void Print();
  void SanitizeTranslator();
};

#endif  // end of class
