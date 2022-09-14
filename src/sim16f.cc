#include "cpu.h"
#include "ui/application.h"


//___________________________________________________________________________________
// Some runtime parameters
struct RunParams {
	CPU           cpu;
	unsigned long delay_us;
	bool          debug;
	std::string   filename;
};

//___________________________________________________________________________________
// Runtime thread for the machine.  This deals with iterating device queues and
// processing instructions.  As fast as possible.
void *run_machine(void *a_params) {
	RunParams &params = *(RunParams *)a_params;
	CPU &cpu = params.cpu;
	try {
		while (cpu.running()) {
			try {
				if (not cpu.process_queue())
					sleep_for_us(100);
			} catch (std::string &error) {
				std::cerr << error << "\n";
			}
		}
	} catch(const char *message) {
		std::cerr << "Machine Abnormal Exit: " << message << "\n";
	} catch(const std::string &message) {
		std::cerr << "Machine Abnormal Exit: " << message << "\n";
	}
	std::cout << "Machine exiting" << std::endl;
	pthread_exit(NULL);
	return 0;
}

//___________________________________________________________________________________
// Implement the clock device by using a microsecond sleep which generates queue
// events at the appropriate frequency.
void *run_clock(void *a_params) {
	RunParams &params = *(RunParams *)a_params;
	CPU &cpu = params.cpu;
	std::cout << "Running CPU clock: delay is: " << params.delay_us << "\n";
	try {
		cpu.run_clock(params.delay_us, params.debug);
	} catch(const std::string &message) {
		std::cerr << "CPU Abnormal Exit: " << message << "\n";
	}
	std::cout << "Clock stopped" << std::endl;
	pthread_exit(NULL);
	return 0;
}


#include "utils/cmdline.h"
//___________________________________________________________________________________
// Host thread.
#ifndef TESTING

int main(int argc, char *argv[]) {
	RunParams params;
	CPU &cpu = params.cpu;

	std::string outfile = "-";
	unsigned long frequency = 8;

	CommandLine cmdline(argc, argv);
	if (cmdline.cmdOptionExists("-h") || argc==1) {
		std::cout << "A PIC16f6xxx simulator\n";
		std::cout << "   Disclaimer:  things may break.  It's not my fault.\n";
		std::cout << "\n";
		std::cout << "sim16f <options>\n";
		std::cout << "  available options:\n";
		std::cout << "    -i              - interactive: prevents a GTK window from being displayed.\n";
		std::cout << "    -a filename     - assemble a list of instructions read from <filename>.\n";
		std::cout << "    -x filename     - read a .hex file and configure the CPU.\n";
		std::cout << "    -c config_words - configure the CPU. eg: 'sim16f -c 0x10,0x20 [,...]\n";
		std::cout << "    -d filename     - read a hex file and output disassembled instructions.\n";
		std::cout << "    -o filename     - output to file instead of stdout.\n";
		std::cout << "    -e eeprom_bytes - read in eeprom data. eg: 'sim16f -e 0x10,0x20,[,...]'\n";
		std::cout << "    -f frequency    - set the clock frequency in Hz.\n";
		std::cout << "    -u filename     - dump the current CPU configuration into a hex file.\n";
		std::cout << "    -r              - run the emulator\n";
		std::cout << "    -g              - run the emulator in debug mode\n";
		std::cout << "    -m model        - select the kind of processor [default 16f628a]\n";
		std::cout << "\n";
		std::cout << "Options may be used together.  For example,\n";
		std::cout << "  'sim16f -c 0x10,0x20 -a test.a -e 0x10,0x20 -u -o test.hex'\n";
		std::cout << "  will produce a hex file that can be read by most PIC programmers,\n";
		std::cout << "and './sim16f -x test.hex -g' will load a hex file and execute with debug.\n";
		std::cout << "\n";
		std::cout << "Note that although frequency is in Hz, the CPU needs 4 clock cycles\n";
		std::cout << "per instruction, so for example, a frequency of 8 should process two \n";
		std::cout << "instructions cycles per second.  Some instructions (eg. goto) consume\n";
		std::cout << "more than one instruction cycle.\n";
		std::cout << "\n";
	}

	try {
		if (cmdline.cmdOptionExists("-m")) {
			if (cmdline.getCmdOption("-m") == "?") {
				std::cout << "Available CPU models:\n";
				std::cout << "    pic16f627\n";
				std::cout << "    pic16f628\n";
				std::cout << "    pic16f648\n";
			} else
				cpu.model(cmdline.getCmdOption("-m"));
		} else
			cpu.model("16f628a");

		if (cmdline.cmdOptionExists("-f")) {
			char *p;
			std::string freq = cmdline.getCmdOption("-f");
			frequency = strtoul(freq.c_str(), &p, 10);
			if (!p) throw(std::string("Invalid frequency argument: ") + freq);
		}
		if (cmdline.cmdOptionExists("-x")) {
			std::string fn = cmdline.getCmdOption("-x");
			if (!FileExists(fn.c_str())) throw(std::string("File does not exist: ")+fn);
			cpu.load_hex(fn);
			params.filename = fn;
		}
		if (cmdline.cmdOptionExists("-a")) {
			std::string fn = cmdline.getCmdOption("-a");
			if (!FileExists(fn.c_str())) throw(std::string("File does not exist: ")+fn);
			try {
				cpu.assemble(fn);
			} catch (std::string &err) {
				std::cout << "error in assembly: " << err << "\n";
			}
			params.filename = fn.substr(fn.find("."))+".hex";
		}
		if (cmdline.cmdOptionExists("-c")) {
			cpu.configure(cmdline.getCmdOption("-c"));
		}
		if (cmdline.cmdOptionExists("-e")) {
			cpu.load_eeprom(cmdline.getCmdOption("-e"));
		}
		if (cmdline.cmdOptionExists("-o")) {
			outfile = cmdline.getCmdOption("-o");
		}

		//_______________________________________________________
		// commands with possible output must come after this point
		if (cmdline.cmdOptionExists("-d")) {
			std::string fn = cmdline.getCmdOption("-d");
			if (fn.length()) cpu.disassemble(fn); else cpu.disassemble(fn);
		}
		if (cmdline.cmdOptionExists("-u")) {
			std::string fn = cmdline.getCmdOption("-u");
			if (fn.length()) cpu.dump_hex(fn);
		}


		if (cmdline.cmdOptionExists("-i") ) {
			if (cmdline.cmdOptionExists("-r") || cmdline.cmdOptionExists("-g")) {
				pthread_t machine, clock;
				unsigned long clock_speed_hz = frequency;
				params.delay_us = 1000000 / clock_speed_hz;
				params.debug = cmdline.cmdOptionExists("-g");
				pthread_create(&machine, 0, run_machine, &params);
				pthread_create(&clock, 0, run_clock, &params);

				std::cout << "Running CPU clock: delay is: " << params.delay_us << " us\n";

				// do stuff, cpu.stop()
				pthread_join(machine, NULL);
				pthread_join(clock, NULL);
				std::cout << "Application terminating" << std::endl;
			}
		} else {
			cpu.load_hex("test.hex");
			pthread_t machine, clock;
			unsigned long clock_speed_hz = frequency;
			params.delay_us = 1000000 / clock_speed_hz;
			params.debug = true;

			pthread_create(&machine, 0, run_machine, &params);
			pthread_create(&clock, 0, run_clock, &params);

			run_application(cpu.cpu_data());
			cpu.stop();
			pthread_join(machine, NULL);
			pthread_join(clock, NULL);
			std::cout << "Application terminating" << std::endl;
		}


	} catch (std::string &err) {
		std::cerr << "Error: " << err << "\n";
		return 0;
	}

}
#endif
