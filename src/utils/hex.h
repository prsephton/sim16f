#pragma once

#include "../cpu_data.h"

// Load CPU data from a hex file
bool load_hex(const std::string &a_filename, CPU_DATA &cpu);

// Dump CPU data to hex file
bool dump_hex(const std::string &a_filename, CPU_DATA &cpu);

