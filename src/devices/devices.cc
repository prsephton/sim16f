#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include "devices.h"
#include "../utils/smart_ptr.cc"

std::mutex DeviceEventQueue::mtx;

void EEPROM::load(const std::string &a_file) {
	clear();
	int fd = open(a_file.c_str(), O_RDONLY);
	int c = read(fd, data, sizeof(data));
	if (c < 0)
		throw(std::string("Cannot read EEPROM data from file: ") + a_file);
}


void Clock::stop() { stopped=true; phase=0; high=false;}
void Clock::start() { stopped=false; }
void Clock::toggle() {
	if (stopped) return;
	high = !high;
	if (high) {
		++phase; phase = phase % 4;
	}

//	std::cout << "toggle clock; high:" << high << "; phase:" << (int)phase << "\n";
	DeviceEventQueue eq;
	eq.queue_event(new DeviceEvent<Clock>(*this, "oscillator", {(BYTE)high}));

	Q1 = phase == 0; if (Q1) eq.queue_event(new DeviceEvent<Clock>(*this, "Q1"));
	Q2 = phase == 1; if (Q2) eq.queue_event(new DeviceEvent<Clock>(*this, "Q2"));
	Q3 = phase == 2; if (Q3) eq.queue_event(new DeviceEvent<Clock>(*this, "Q3"));
	Q4 = phase == 3; if (Q4) eq.queue_event(new DeviceEvent<Clock>(*this, "Q4"));

	eq.queue_event(new DeviceEvent<Clock>(*this, "CLKOUT", {(BYTE)(phase/2?1:0)}));

	if (high and Q1) {
		eq.queue_event(new DeviceEvent<Clock>(*this, "cycle"));
	}
}


void Flash::load(const std::string &a_file) {
	clear();
	int fd = open(a_file.c_str(), O_RDONLY);
	int c = read(fd, data, sizeof(data));
	if (c < 0)
		throw(std::string("Cannot read flash data from file: ") + a_file);
}

std::queue< SmartPtr<QueueableEvent> >DeviceEventQueue::events;

template <class Clock> class
	DeviceEvent<Clock>::registry  DeviceEvent<Clock>::subscribers;

