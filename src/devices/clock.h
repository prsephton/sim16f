#pragma once
#include "device_base.h"

class Clock: public Device {
  public:
	bool stopped;
	bool high;
	BYTE phase;
	BYTE Q1;
	BYTE Q2;
	BYTE Q3;
	BYTE Q4;

	Clock(): stopped(true), high(false), phase(0), Q1(1), Q2(0), Q3(0), Q4(0) {}

	void toggle();
	void stop();
	void start();
};


