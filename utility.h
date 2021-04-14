//________________________________________________
// Just some utility functions
#ifndef __utility_h__
#define __utility_h__

#include <string>
#include <algorithm>

const std::string int_to_string(int i);
const std::string int_to_hex(int i);
const std::string to_upper(std::string in);
bool is_decimal(const std::string &in);
bool is_hex(const std::string &in);
#endif
