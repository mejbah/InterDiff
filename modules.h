#ifndef MODULES_H
#define MODULES_H

#include <string.h>
#include "pin.H"

class Module {
 public:
  const string _name;
  ADDRINT _base;
  ADDRINT _upper_address;
  UINT32 _id;  

  Module(const string &name, ADDRINT base, ADDRINT upper_address, UINT32 id);
};
#endif
