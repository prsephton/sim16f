//________________________________________________
// Just some utility functions
#ifndef __utility_h__
#define __utility_h__

#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>


const std::string int_to_string(int i);
const std::string int_to_hex(int i, const char *prefix="0x", const char *suffix="");
const std::string to_upper(std::string in);
bool is_decimal(const std::string &in);
bool is_hex(const std::string &in);
bool FileExists(const std::string &s);
int as_int(const std::string &a_val);


class LockUI {
	static std::mutex mtx;
	int semaphore = 0;

  public:
	LockUI(bool lock=true) {
		if (lock) acquire();
	}
	~LockUI() {
		if (semaphore) release();
	}
	void acquire() {
		if (!semaphore) mtx.lock();
		++semaphore;
	}
	void release() {
		if (semaphore > 0 && --semaphore == 0)
			mtx.unlock();
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
