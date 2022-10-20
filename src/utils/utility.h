//________________________________________________
// Just some utility functions
#ifndef __utility_h__
#define __utility_h__

#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>
#include <thread>
#include <chrono>

const std::string int_to_string(int i);
const std::string int_to_hex(int i, const char *prefix="0x", const char *suffix="");
const std::string to_upper(std::string in);
bool is_decimal(const std::string &in);
bool is_hex(const std::string &in);
bool FileExists(const std::string &s);
int as_int(const std::string &a_val);
const std::string as_text(double a_value, int a_precision=4);
bool float_equiv(double a, double b, double limit=1.0e-12);

//_____________________________________________________________________________________________________________
// A convenience class to deal with values that have a unit.  Return a value and a magnitude.
double value_and_unit(double a_value, int &a_mag);
const std::chrono::microseconds wait_interval(100);
//_____________________________________________________________________________________________________________
// Return properly formatted numbers with units
const std::string unit_text(double a_value, const std::string &unit);

class LockUI {
	static std::mutex mtx;
	static int semaphore;
	static std::thread::id tid;

  public:
	LockUI(bool lock=true) {
		if (lock) acquire();
	}
	~LockUI() {
		if (semaphore) release();
	}
	void acquire() {
		if (tid == std::this_thread::get_id()) {
			++semaphore;
			return;
		}
		while (not mtx.try_lock())
			std::this_thread::sleep_for(wait_interval);
		++semaphore;
		tid =  std::this_thread::get_id();
	}

	void release() {
		if (semaphore > 0 && --semaphore == 0) {
			mtx.unlock();
			tid = std::thread::id();   // does not represent a thread
		}
	}
};


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
