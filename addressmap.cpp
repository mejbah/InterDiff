#include "addressmap.h"
#include <iostream>
#include <assert.h>

AddressTranslator AddressMarker::address_translator = AddressTranslator();

AddressTranslator::AddressTranslator() : _next(INVALID_ADDR + 4),
                                         _safety_margin(4) {
}

addr_type AddressTranslator::NextAddr(size_t size) {
  addr_type ret_val = _next;
  _next += size + _safety_margin;
  //cout << "_next " << _next << ", ret_val " 
  //     << ret_val << ",  size " << size << endl;
  return ret_val;
}

void AddressMarker::Set(addr_type start_addr, addr_type end_addr) {
  _start_addr = start_addr;
  _end_addr = end_addr;
}

void AddressMarker::InsertLifeRange(access_count_type access_counter) {
  LifeRangeVector::iterator it;
  for (it = _life_range_vector.begin(); it != _life_range_vector.end(); )
    if ((*it)->first > access_counter) break;
    else ++it;
  _life_range_vector.insert(it, new LifeRange(access_counter, 
                            address_translator.NextAddr(_end_addr - _start_addr + 1)));
}

addr_type AddressMarker::Translate(addr_type address, access_count_type access_counter) {
  addr_type diff = address - _start_addr;
  LifeRangeVector::iterator it;
  LifeRange *life_range = NULL;
  for (it = _life_range_vector.begin(); it != _life_range_vector.end(); ) {
    life_range = *it;
    if (life_range->first >= access_counter)
      break;
    ++it;
  }
  return (life_range->second + diff);
}

AddressMarker::AddressMarker(addr_type start_addr, addr_type end_addr) {
  Set(start_addr, end_addr);
  _life_range_vector.insert(_life_range_vector.begin(), new LifeRange(INFINITY, start_addr));
}

AddressMarker::~AddressMarker() {
  for (LifeRangeVector::iterator it = _life_range_vector.begin();
       it != _life_range_vector.end(); ) {
    LifeRange *life_range = *it;
    ++it;
    delete life_range;
  }
  _life_range_vector.clear();
}

void AddressMarker::Print() {
  cout << "start " << _start_addr << ", end " << _end_addr;
  for (LifeRangeVector::iterator it = _life_range_vector.begin(); 
       it != _life_range_vector.end(); ) {
    LifeRange *life_range = *it;
    ++it;
    cout << endl << "ac " << life_range->first << ", addr(xlated) " 
         << life_range->second;
  }
}

void AddressMarkerVectorHandler::Allocate(addr_type addr, size_t size) {
  //LifeRange* new_life_range = new LifeRange(addr, size);
  _malloc_vector.push_back(new pair<addr_type, size_t>(addr, size));
  InsertRange(addr, addr + size - 1);
}

void AddressMarkerVectorHandler::InsertRange(addr_type s, addr_type e) {
  AddressMarker *marker = NULL;
  AddressMarkerVector::iterator it;
  bool found = false;
  int index = 0;
  for (it = _address_marker_vector.begin(); 
       it != _address_marker_vector.end(); ) {
    marker = *it;
    if (marker->_start_addr > e) break;
    if (marker->_end_addr < s) {
      ++it;
      index++;
      continue;
    }
    found = true;
    break;
  }
  if (!found) {
    _address_marker_vector.insert(it, new AddressMarker(s, e));
    return;
  }
  assert(marker != NULL);
  addr_type s1 = marker->_start_addr;
  addr_type e1 = marker->_end_addr;
  if (s1 == e1) {
    if (s < s1) {
      if (e == s1) {
        _address_marker_vector.insert(it, new AddressMarker(s, s1 - 1));
      } else if (e > s1) {
        _address_marker_vector.insert(it, new AddressMarker(s, s1 - 1));
        InsertRange(e1 + 1, e);
      } else {
        cerr << "Not possible!!!" << endl;
        exit(-1);
      }
    } else if (s == s1) {
      if (e == s1) {
        // do nothing
      } else if (e > s1) {
        InsertRange(e1 + 1, e);
      } else {
        cerr << "Not possible!!!" << endl;
        exit(-1);
      }
    } else {
      cerr << "Not possible!!!" << endl;
      exit(-1);
    }
  } else {
    if (s < s1) {
      if (e == s1) {
        _address_marker_vector.insert(it , new AddressMarker(s, s1 - 1));
        marker->_start_addr = e;
        marker->_end_addr = e;
        index++;
        int j = 0;
        for (it = _address_marker_vector.begin(); j <= index; j++)
          it++;
        _address_marker_vector.insert(it, new AddressMarker(s1 + 1, e1));
      } else if (s1 < e && e < e1) {
        _address_marker_vector.insert(it, new AddressMarker(s, s1 - 1));
        marker->_end_addr = e;
        index++;
        int j = 0;
        for (it = _address_marker_vector.begin(); j <= index; j++)
          it++;
        _address_marker_vector.insert(it, new AddressMarker(e + 1, e1));
      } else if (e == e1) {
        _address_marker_vector.insert(it, new AddressMarker(s, s1 - 1));
      } else if (e > e1) {
        _address_marker_vector.insert(it, new AddressMarker(s, s1 - 1));
        InsertRange(e1 + 1, e);
      } else {
        cerr << "Not possible!!!" << endl;
        exit(-1);
      }
    } else if (s == s1) {
      if (e == s1) {
        _address_marker_vector.insert(it, new AddressMarker(s, e));
        marker->_start_addr = s + 1;
      } else if (s1 < e && e < e1) {
        _address_marker_vector.insert(it, new AddressMarker(s, e));
        marker->_start_addr = e + 1;
      } else if (e == e1) {
        // do nothing
      } else if(e > e1) {
        InsertRange(e1 + 1, e);
      } else {
        cerr << "Not possible!!!" << endl;
        exit(-1);
      }
    } else if (s1 < s && s < e1) {
      if (e == s) {
        _address_marker_vector.insert(it, new AddressMarker(s1, s - 1));
        marker->_start_addr = s;
        marker ->_end_addr = e;
        index++;
        int j = 0;
        for (it = _address_marker_vector.begin(); j <= index; j++)
          it++;
        _address_marker_vector.insert(it, new AddressMarker(e + 1, e1));
      } else if (s < e && e < e1) {
        _address_marker_vector.insert(it, new AddressMarker(s1, s - 1));
        marker->_start_addr = s;
        marker->_end_addr = e;
        index++;
        int j = 0;
        for (it = _address_marker_vector.begin(); j <= index; j++)
          it++;
        _address_marker_vector.insert(it, new AddressMarker(e + 1, e1));
      } else if (e == e1) {
        _address_marker_vector.insert(it, new AddressMarker(s1, s - 1));
        marker->_start_addr = s;
      } else if (e > e1) {
        _address_marker_vector.insert(it, new AddressMarker(s1, s - 1));
        marker->_start_addr = s;
        InsertRange(e1 + 1, e);
      } else {
        cerr << "Not possible!!!" << endl;
        exit(-1);
      }
    } else if (s == e1) {
      if (e == e1) {
        _address_marker_vector.insert(it, new AddressMarker(s1, s - 1));
        marker->_start_addr = s; 
      } else if ( e > e1) {
        _address_marker_vector.insert(it, new AddressMarker(s1, e1 - 1));
        marker->_start_addr = s;
        marker->_end_addr = s;
        InsertRange(s + 1, e);
      } else {
        cerr << "Not possible!!!" << endl;
        exit(-1);
      }
    } else {
      cerr << "Not possible!!!" << endl;
      exit(-1);
    }
  }
}

void AddressMarkerVectorHandler::Free(addr_type addr, access_count_type access_counter) {
  vector<pair<addr_type, size_t> *>::iterator it;
  for (it = _malloc_vector.begin(); it != _malloc_vector.end(); ) {
    if ((*it)->first == addr) break;
    ++it;
  }
  if (it == _malloc_vector.end()) 
    return;
  size_t size = (*it)->second;
  pair<addr_type, size_t> *malloc_pair = *it;
  _malloc_vector.erase(it);
  delete malloc_pair;
  AddressMarkerVector::iterator amv_it;
  for (amv_it = _address_marker_vector.begin(); 
       amv_it != _address_marker_vector.end(); ) {
    if ((*amv_it)->_start_addr == addr) break;
    ++amv_it;
  }
  assert(amv_it != _address_marker_vector.end());
  for (; (*amv_it)->_start_addr <= (addr + size -1); ) {
    (*amv_it)->InsertLifeRange(access_counter);
    ++amv_it;
    if (amv_it == _address_marker_vector.end()) break;
  }
}

AddressMarkerVectorHandler::~AddressMarkerVectorHandler() {
  for (AddressMarkerVector::iterator it = _address_marker_vector.begin();
       it != _address_marker_vector.end(); ) {
    AddressMarker *marker = *it;
    ++it;
    delete marker;
  }
  _address_marker_vector.clear();
  for (vector<pair<addr_type, size_t> *>::iterator it = _malloc_vector.begin();
       it != _malloc_vector.end();) {
    pair<addr_type, size_t> *malloc_pair = *it;
    ++it;
    delete malloc_pair;
  }
  _malloc_vector.clear();
}

bool AddressMarkerVectorHandler::IsPresent(addr_type address, addr_type &translated_addr,
                                           access_count_type access_counter) {
  bool found = false;
  AddressMarker *marker = NULL;
  for (AddressMarkerVector::iterator it = _address_marker_vector.begin();
       it != _address_marker_vector.end();) {
    marker = *it;
    if (marker->_start_addr <= address && address <= marker->_end_addr) {
      found = true;
      break;
    }
    if (marker->_start_addr > address) break;
    ++it;
  }
  if (found) {
    translated_addr = marker->Translate(address, access_counter);
  } else 
    translated_addr = address;
  return found;
}

void AddressMarkerVectorHandler::Print() {
  for (AddressMarkerVector::iterator it = _address_marker_vector.begin();
       it != _address_marker_vector.end(); ) {
    AddressMarker *marker = *it;
    ++it;
    marker->Print();
    cout << endl;
  }
}

void AddressMarkerVectorHandler::SanitizeTranslator() {
  for (AddressMarkerVector::iterator it = _address_marker_vector.begin();
       it != _address_marker_vector.end(); ) {
    AddressMarker *marker = *it;
    ++it;
    marker->_life_range_vector[0]->second = marker->_start_addr;
  }
}
