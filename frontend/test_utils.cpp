
#include <cstdio> // easy formatted printing
#include <iostream>
#include <fstream>

#include "i8080.h"
#include <test.hpp>

#define concatenate(w1, w2) (((w1) << 8) | (w2))

// externed in test.hpp. Required for all tests
i8080_word_t VM_MEMORY[VM_64K];

bool yesno_prompt(const char prompt[])
{
	std::string line;
	while (true) {
		std::cout << prompt << " [yn] ";
		std::getline(std::cin, line);
		if (line.length() == 1 && line.find_first_of("YyNn") != std::string::npos) {
			break;
		}
	}
	return line == "Y" || line == "y";
}

// dump debug info
void i8080_dump_stats(const struct i8080& i8080)
{
	std::printf("\nDebug information:\n");
	std::printf("PC: 0x%04x\n", i8080.pc);
	std::printf("SP: 0x%04x\n", i8080.sp);
	std::printf("A: 0x%02x\n", i8080.a);
	std::printf("B: 0x%02x\n", i8080.b);
	std::printf("C: 0x%02x\n", i8080.c);
	std::printf("D: 0x%02x\n", i8080.d);
	std::printf("E: 0x%02x\n", i8080.e);
	std::printf("H: 0x%02x\n", i8080.h);
	std::printf("L: 0x%02x\n", i8080.l);
	std::printf("BC: 0x%04x\n", concatenate(i8080.b, i8080.c));
	std::printf("DE: 0x%04x\n", concatenate(i8080.d, i8080.e));
	std::printf("HL: 0x%04x\n", concatenate(i8080.h, i8080.l));
	std::printf("Flags: s=%d, z=%d, acy=%d, cy=%d, p=%d\n", i8080.s, i8080.z, i8080.acy, i8080.cy, i8080.p);
	std::printf("HLT: %d, INTE: %d\n", i8080.halt, i8080.inte);
	std::printf("Cycles: %lu", i8080.cycles);

	bool should_dump = yesno_prompt("\nDump memory to file?");

	if (should_dump) {
		std::ofstream memdump("memdump.bin", std::ofstream::out | std::ofstream::binary);
		if (!memdump.is_open()) {
			std::cout << "File not open." << std::endl;
			return;
		}

		char buf[VM_64K];
		for (std::size_t i = 0; i < VM_64K; ++i) {
			buf[i] = (char)i8080.memory_read(&i8080, (i8080_addr_t)i);
		}

		memdump.write(buf, VM_64K);
		memdump.close();
		if (memdump.fail()) {
			std::cout << "File write error." << std::endl;
		}
	}
}

int load_memory(const char filepath[], i8080_word_t* memory, std::size_t maxlen)
{
	std::ifstream file(filepath, std::ifstream::in | std::ifstream::binary);
	if (!file.is_open()) {
		std::cout << "File not open." << std::endl;
		return -1;
	}

	// check size
	file.ignore(std::numeric_limits<std::streamsize>::max());
	if (file.fail() || !file.eof()) {
		file.close();
		std::cout << "File stream error." << std::endl;
		return -1;
	}
	auto length = file.gcount();
	if ((std::size_t)length > maxlen) {
		file.close();
		std::cout << "File larger than " << maxlen << " bytes." << std::endl;
		return -1;
	}

	// rewind
	file.clear();
	file.seekg(0, std::ifstream::beg);

	// read
	char buf[VM_64K];
	file.read(buf, length);
	if (file.fail()) {
		file.close();
		std::cout << "File read error." << std::endl;
		return -1;
	}
	file.close();

	// copy to memory
	for (auto i = 0; i < length; ++i) {
		memory[i] =  (i8080_word_t)buf[i];
	}

	return 0;
}
