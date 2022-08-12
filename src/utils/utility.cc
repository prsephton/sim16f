#include "utility.h"

#include <cstdio>
#include <cstring>
#include <sys/stat.h>


const std::string int_to_string(int i)  {
	char buf[32];
	snprintf(buf, sizeof(buf), "%i", i);
	return std::string(buf);
}

const std::string int_to_hex(int i, const char *prefix, const char *suffix)  {
	char buf[32];
	snprintf(buf, sizeof(buf), "%s%X%s", prefix, i, suffix);
	return std::string(buf);
}

const std::string to_upper(std::string in) {
	 transform(in.begin(), in.end(), in.begin(), ::toupper);
	 return in;
}

bool is_decimal(const std::string &in) {
	for (auto c: in) {
		if (!std::isdigit(c)) return false;
	}
	return true;
}

bool is_hex(const std::string &in) {
	for (auto c: in) {
		if (!std::isdigit(c) && strchr("ABCDEF", std::toupper(c))==NULL) return false;
	}
	return true;
}

bool FileExists(const std::string &s)
{
  struct stat buffer;
  return (stat (s.c_str(), &buffer) == 0);
}


ScopedRedirect::ScopedRedirect(std::ostream & inOriginal, std::ostream & inRedirect) :
	mOriginal(inOriginal),
	mOldBuffer(inOriginal.rdbuf(inRedirect.rdbuf()))
{ }

ScopedRedirect::~ScopedRedirect(){
	mOriginal.rdbuf(mOldBuffer);
}
