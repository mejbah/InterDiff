#include <iostream>
#include <iterator>
#include "atomicsection.h"

AtomicSection::AtomicSection() : _ins(0), _access_counter(0) {
}

// We have optimized the set to include only the start address.
// The assumption is that a variable will be consistently accessed
// with its start address.
// REMEMBER: IF WE INSERT EACH BYTE OR WORD OF THE ACCESS INSETAD OF
// THE START, WE HAVE TO CHANGE ReadAddressExecHistory_New of CONFLICT PAIR 
// FILE.
AtomicSection::AtomicSection(pc_type pc, addr_type write_addr, size_t write_len,
                addr_type read_addr, addr_type read_addr2, size_t read_len,
                access_count_type access_counter) : 
                    _ins(pc), _access_counter(access_counter) {
  //_ins_set.insert(pc);
  if (read_addr != INVALID_ADDR){
    //addr_type end_addr = read_addr + read_len;
    ////for (addr_type start_addr = read_addr & ACCESS_SIZE_MASK; 
    //for (addr_type start_addr = read_addr; 
    //     start_addr < end_addr; start_addr += ACCESS_SIZE )
    //  _read_set.insert(start_addr);
    _read_set.insert(read_addr);
    _read_addr1 = read_addr;
    if (read_addr2 != INVALID_ADDR) {
    //  end_addr = read_addr2 + read_len;
    //  //for (addr_type start_addr = read_addr2 & ACCESS_SIZE_MASK; 
    //  for (addr_type start_addr = read_addr2; 
    //      start_addr < end_addr; start_addr += ACCESS_SIZE)
    //    _read_set.insert(start_addr);
      _read_set.insert(read_addr2);
      _read_addr2 = read_addr2;
    }
  }

  if (write_addr != INVALID_ADDR) {
    //addr_type end_addr = write_addr + write_len;
    ////for (addr_type start_addr = write_addr & ACCESS_SIZE_MASK; 
    //for (addr_type start_addr = write_addr; 
    //     start_addr < end_addr; start_addr += ACCESS_SIZE )
    //  _write_set.insert(start_addr);
    _write_set.insert(write_addr);
    _write_addr = write_addr;
  }
}


AtomicSection::~AtomicSection() {
  _read_set.clear();
  _write_set.clear();
}


void AtomicSection::Clear() {
  _read_set.clear();
  _write_set.clear();
  _access_counter = 0;
  _ins = 0;
  _read_addr1 = INVALID_ADDR;
  _read_addr2 = INVALID_ADDR;
  _write_addr = INVALID_ADDR;
}


void AtomicSection::Set(pc_type pc, addr_type write_addr, size_t write_len,
                addr_type read_addr, addr_type read_addr2, size_t read_len,
                access_count_type access_counter) { 
  Clear();
  _ins = pc;
  _access_counter = access_counter;
  //_ins_set.insert(pc);
  if (read_addr != INVALID_ADDR){
    //addr_type end_addr = read_addr + read_len;
    ////for (addr_type start_addr = read_addr & ACCESS_SIZE_MASK; 
    //for (addr_type start_addr = read_addr; 
    //     start_addr < end_addr; start_addr += ACCESS_SIZE )
    //  _read_set.insert(start_addr);
    _read_set.insert(read_addr);
    _read_addr1 = read_addr;
    if (read_addr2 != INVALID_ADDR) {
    //  end_addr = read_addr2 + read_len;
    //  //for (addr_type start_addr = read_addr2 & ACCESS_SIZE_MASK; 
    //  for (addr_type start_addr = read_addr2; 
    //      start_addr < end_addr; start_addr += ACCESS_SIZE)
    //    _read_set.insert(start_addr);
      _read_set.insert(read_addr2);
      _read_addr2 = read_addr2;
    }
  }

  if (write_addr != INVALID_ADDR) {
    //addr_type end_addr = write_addr + write_len;
    ////for (addr_type start_addr = write_addr & ACCESS_SIZE_MASK; 
    //for (addr_type start_addr = write_addr; 
    //     start_addr < end_addr; start_addr += ACCESS_SIZE )
    //  _write_set.insert(start_addr);
    _write_set.insert(write_addr);
    _write_addr = write_addr;
  }
}


// The assignment overwrites the lhs atomic section
// That means, the lhs atomic section first clears its instructions and
// read, write set and access counter and then it writes the new 
// components. 
AtomicSection& AtomicSection::operator= (AtomicSection& rhs) {
  if (this == &rhs)
    return *this;

  _access_counter = rhs._access_counter;
  _ins = rhs._ins;
  _read_set = rhs._read_set;
  _write_set = rhs._write_set;
  _read_addr1 = rhs._read_addr1;
  _read_addr2 = rhs._read_addr2;
  _write_addr = rhs._write_addr;
  return *this;
}


ostream& operator << (ostream& os,
                      AtomicSection& dynamic_atomic_section) {
  os << dynamic_atomic_section._access_counter << " [" << hex
     << dynamic_atomic_section._ins << "]" << dec << endl;
  
  os << dynamic_atomic_section._read_set.size() << endl;
  copy(dynamic_atomic_section._read_set.begin(),
       dynamic_atomic_section._read_set.end(),
       ostream_iterator<addr_type>(os, " "));
  os << endl << dynamic_atomic_section._write_set.size() << endl;
  copy(dynamic_atomic_section._write_set.begin(),
       dynamic_atomic_section._write_set.end(),
       ostream_iterator<addr_type>(os, " "));
  os << endl;
  return os;
}

