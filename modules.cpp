#include "modules.h"

Module::Module(const string &name, ADDRINT base, ADDRINT upper_address, 
               UINT32 id) : _name(name), _base(base), 
                            _upper_address(upper_address), _id(id) {
}
