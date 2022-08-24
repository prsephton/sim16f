#include <iostream>
#include "run_tests.h"

#ifdef TESTING

int main(int a_argc, char *a_argv[]) {
	std::cout << std::endl << std::endl;
	std::cout << "============================================================================" << std::endl;
	std::cout << "Testing basic devices" << std::endl;
	std::cout << "============================================================================" << std::endl;
	Tests::test_devices();
	std::cout << std::endl << std::endl;
	std::cout << "============================================================================" << std::endl;
	std::cout << "Testing Assembler Implementation" << std::endl;
	std::cout << "============================================================================" << std::endl;
	Tests::test_assembler();
	std::cout << std::endl << std::endl;
	std::cout << "============================================================================" << std::endl;
	std::cout << "Testing the comparator module" << std::endl;
	std::cout << "============================================================================" << std::endl;
	Tests::test_comparator_module();
	std::cout << std::endl << std::endl;
	std::cout << "============================================================================" << std::endl;
	std::cout << "Testing PORTA & PORTB devices" << std::endl;
	std::cout << "============================================================================" << std::endl;
	Tests::test_ports();
}

#endif
