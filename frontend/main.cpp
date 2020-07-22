
#include <iostream>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <test.hpp>
#include "i8080.h"
//#include "cpm80_vm.h"

#define libi8080_K64 (0x10000)

// 64KB of emulator memory
static i8080_word_t MEMORY[libi8080_K64];

static i8080_word_t memory_read(i8080_addr_t addr) {
	return MEMORY[addr];
}

static void memory_write(i8080_addr_t addr, i8080_word_t word) {
	MEMORY[addr] = word;
}

static const unsigned d1[] = { 10, 0, 1, 26, 6, 1024, 243, 64, 64, 2, 1 };
static const unsigned d2[] = { 2, 1, 0 };
static const unsigned d3[] = { 2, 2, 0 };
static const unsigned d4[] = { 2, 3, 0 };

static const unsigned* const disk_params[] = { d1, d2, d3, d4 };

int i8main(int argc, char** argv)
{
	int err;
	std::string test_path;

	// change for platform or usage
	const int argmax = 5;
	const int args_start_at = 1;

	if (argv && argc >= args_start_at) {
		for (int i = args_start_at; i < std::min(argc, argmax); ++i) {
			if (argv[i]) {
				// use the first valid string
				test_path = argv[i];
				break;
			}
		}
	}

	if (test_path.empty()) {
		err = i8080_run_default_tests();
	} else {
		err = i8080_run_user_test(test_path);
	}

	if (err) return EXIT_FAILURE;
	else return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
	return i8main(argc, argv);
	//struct i8080 cpu;
	//cpu.memory_read = memory_read;
	//cpu.memory_write = memory_write;
	//cpu.io_read = nullptr;
	//cpu.io_write = nullptr;
	//cpu.interrupt_read = nullptr;
	//i8080_reset(&cpu);
	//
	//struct cpm80_vm vm;
	//vm.cpu = &cpu;
	//
	//cpm80_addr_t disk_dph_out[4];
	//cpm80_bios_redefine_disks(&vm, 4, disk_params, 0x0000, 0x0100, disk_dph_out);
}
