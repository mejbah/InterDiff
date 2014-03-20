#include "instruction.h"

bool AddrLt::operator()(const addr_type a, const addr_type b)const {
  return a < b;
}

bool AddrEq::operator()(const addr_type a, const addr_type b) const {
  return a == b;
}

bool AccessCountLt::operator()(const access_count_type a, const access_count_type b)const {
  return a < b;
} 

size_t find_nth(const string& s, char c, int n) {
  size_t result = 0;
  int count = 0;
  if (!n) return string::npos;
  while (1) {
    result = s.find(c, result);
    if (result == string::npos)
      break;
    count++;
    if (count == n)
      return result;
    result++;
    if (result == s.size())
      return string::npos;
  }
  return string::npos;
}
