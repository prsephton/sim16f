#include "utility.h"

#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <sstream>
#include <iomanip>
#include <cmath>

std::mutex LockUI::mtx;
int LockUI::semaphore = 0;
std::thread::id LockUI::tid;

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

int as_int(const std::string &a_val) {
	int i;
	std::stringstream(a_val) >> i;
	return i;
}

bool FileExists(const std::string &s)
{
  struct stat buffer;
  return (stat (s.c_str(), &buffer) == 0);
}

bool float_equiv(double a, double b, double limit) {
	return fabs(a - b) < limit;
}

const std::string as_text(void *a_val) {
	std::ostringstream s;
	s << std::hex << a_val;
	return s.str();
}

const std::string as_text(double a_value, int a_precision) {
	std::ostringstream s;
	s << std::fixed << std::setprecision(a_precision) << std::defaultfloat << a_value;
	return s.str();
}

//_____________________________________________________________________________________________________________
// A convenience class to deal with values that have a unit.
double value_and_unit(double a_value, int &a_mag) {
	a_mag = float_equiv(a_value, 0)?0:floor(log10(fabs(a_value)) / 3.0);
	double divisor = pow(10, a_mag*3);
	return a_value / divisor;
}

//_____________________________________________________________________________________________________________
// Return properly formatted numbers with units
const std::string unit_text(double a_value, const std::string &unit) {
	int l_mag;
	a_value = value_and_unit(a_value, l_mag);
	std::string l_vtext = as_text(a_value, 6);
// Ω µ
	switch (l_mag) {
	  case -4 :
		  return l_vtext + " p" + unit;
	  case -3 :
		  return l_vtext + " n" + unit;
	  case -2 :
		  return l_vtext + " µ" + unit;
	  case -1 :
		  return l_vtext + " m" + unit;
	  case 0 :
		  return l_vtext + " " + unit;
	  case 1 :
		  return l_vtext + " K" + unit;
	  case 2 :
		  return l_vtext + " M" + unit;
	  case 3 :
		  return l_vtext + " G" + unit;
	  case 4 :
		  return l_vtext + " T" + unit;
	  default :
		  return l_vtext + "x10^" + int_to_string(l_mag*3) + " " + unit;
	}
}

ScopedRedirect::ScopedRedirect(std::ostream & inOriginal, std::ostream & inRedirect) :
	mOriginal(inOriginal),
	mOldBuffer(inOriginal.rdbuf(inRedirect.rdbuf()))
{ }

ScopedRedirect::~ScopedRedirect(){
	mOriginal.rdbuf(mOldBuffer);
}
