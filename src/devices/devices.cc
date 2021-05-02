#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include "devices.h"


void EEPROM::load(const std::string &a_file) {
	clear();
	int fd = open(a_file.c_str(), O_RDONLY);
	int c = read(fd, data, sizeof(data));
	if (c < 0)
		throw(std::string("Cannot read EEPROM data from file: ") + a_file);
}


void Clock::toggle(PINS &pins) {
	high = !high;
	if (high) {
		++phase; phase = phase % 4;
	}
//	std::cout << "toggle clock; high:" << high << "; phase:" << (int)phase << "\n";
	pins.set_pin(PINS::CLKIN, high? 5.0 : 0.0);
	if (high) {
		PINS::Event event = PINS::Event(PINS::CLKIN, "oscillator", 5.0, 0.0);
		pins.events.push(event);
	}

	Q1 = phase == 0;
	Q2 = phase == 1;
	Q3 = phase == 2;
	Q4 = phase == 3;
	pins.set_pin(PINS::CLKOUT, phase/2? 5.0 : 0.0);
	if (high and Q1) {
		PINS::Event event = PINS::Event(PINS::CLKOUT, "cycle", 5.0, 0.0);
		pins.events.push(event);
	}
}


void Flash::load(const std::string &a_file) {
	clear();
	int fd = open(a_file.c_str(), O_RDONLY);
	int c = read(fd, data, sizeof(data));
	if (c < 0)
		throw(std::string("Cannot read flash data from file: ") + a_file);
}

