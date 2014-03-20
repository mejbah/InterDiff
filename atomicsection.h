#ifndef ATOMIC_SECTION_H
#define ATOMIC_SECTION_H

#include <map>
#include <set>
#include <vector>
#include "instruction.h"


typedef hash_set<addr_type, hash<addr_type>, AddrEq> HashAddressSet;

class AtomicSection {
 public:
  pc_type _ins;
  HashAddressSet _read_set, _write_set;
  addr_type _read_addr1;
  addr_type _read_addr2;
  addr_type _write_addr;
  line_no_type _line_no;
  access_count_type _access_counter;

 public:
  AtomicSection();
  // This constructor initializes the atomic section with a dynamic instruction
  AtomicSection(pc_type pc, addr_type write_addr, size_t write_len,
                addr_type read_addr, addr_type read_addr2, size_t read_len,
                access_count_type access_counter);
  void Set(pc_type pc, addr_type write_addr, size_t write_len,
                addr_type read_addr, addr_type read_addr2, size_t read_len,
                access_count_type access_counter);
  ~AtomicSection();

  void Clear();

  
  // The assignment overwrites the lhs atomic section
  // That means, the lhs atomic section first clears its instructions and
  // read, write set and access counter and then it writes the new 
  // components. 
  AtomicSection& operator= (AtomicSection& rhs);

  friend ostream& operator << (ostream& os,
                               AtomicSection& dynamic_atomic_section);
};

ostream& operator << (ostream& os,
                      AtomicSection& dynamic_atomic_section);
  
#endif  // end of class
