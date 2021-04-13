#include <cstdio>
#include "utility.h"


const std::string int_to_string(int i)  {
	char buf[32];
	snprintf(buf, sizeof(buf), "%i", i);
	return std::string(buf);
}

const std::string int_to_hex(int i)  {
	char buf[32];
	snprintf(buf, sizeof(buf), "0x%X", i);
	return std::string(buf);
}

