//________________________________________________
// Just some utility functions
#ifndef __utility_h__
#define __utility_h__

#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>

const std::string int_to_string(int i);
const std::string int_to_hex(int i, const char *prefix="0x", const char *suffix="");
const std::string to_upper(std::string in);
bool is_decimal(const std::string &in);
bool is_hex(const std::string &in);
bool FileExists(const std::string &s);

class ScopedRedirect
{
  public:
    ScopedRedirect(std::ostream & inOriginal, std::ostream & inRedirect);
    ~ScopedRedirect();

  private:
    ScopedRedirect(const ScopedRedirect&);
    ScopedRedirect& operator=(const ScopedRedirect&);

    std::ostream & mOriginal;
    std::streambuf * mOldBuffer;
};


#endif
