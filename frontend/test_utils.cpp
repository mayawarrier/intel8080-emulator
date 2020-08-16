
#include <cstddef>
#include <cstdio> // easy formatted printing
#include <cstdarg>
#include <cstring>

#include <new>
#include <limits>
#include <exception>
#include <string>
#include <iostream>
#include <fstream>

#include "i8080.h"
#include <test.hpp>

#define concatenate(w1, w2) (((w1) << 8) | (w2))

// global, externed in test.hpp. Required for all tests
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
void i8080_dump_stats_to_console(const struct i8080* i8080)
{
	std::printf("\nDebug information:\n");
	std::printf("PC: 0x%04x\n", i8080->pc);
	std::printf("SP: 0x%04x\n", i8080->sp);
	std::printf("A: 0x%02x\n", i8080->a);
	std::printf("B: 0x%02x\n", i8080->b);
	std::printf("C: 0x%02x\n", i8080->c);
	std::printf("D: 0x%02x\n", i8080->d);
	std::printf("E: 0x%02x\n", i8080->e);
	std::printf("H: 0x%02x\n", i8080->h);
	std::printf("L: 0x%02x\n", i8080->l);
	std::printf("BC: 0x%04x\n", concatenate(i8080->b, i8080->c));
	std::printf("DE: 0x%04x\n", concatenate(i8080->d, i8080->e));
	std::printf("HL: 0x%04x\n", concatenate(i8080->h, i8080->l));
	std::printf("Flags: s=%d, z=%d, acy=%d, cy=%d, p=%d\n", i8080->s, i8080->z, i8080->acy, i8080->cy, i8080->p);
	std::printf("HLT: %d, INTE: %d\n", i8080->halt, i8080->inte);
	std::printf("Cycles: %lu\n", i8080->cycles);

	bool should_dump = yesno_prompt("\nDump memory to file?");

	if (should_dump) {
		std::ofstream memdump("memdump.bin", std::ofstream::out | std::ofstream::binary);
		if (!memdump.is_open()) {
			std::cout << "File cannot be opened." << std::endl;
			return;
		}

		i8080_word_t(*memory_read)(const struct i8080*, i8080_addr_t) = i8080->memory_read;

		char buf[VM_64K];
		for (std::size_t i = 0; i < VM_64K; ++i) {
			buf[i] = (char)memory_read(i8080, (i8080_addr_t)i);
		}

		memdump.write(buf, VM_64K);
		memdump.close();
		if (memdump.fail()) {
			std::cout << "File write error." << std::endl;
		}
	}
}

int load_memory(const char filepath[], i8080_word_t* memory, std::size_t maxlen, const char console_prefix[])
{
	std::ifstream file(filepath, std::ifstream::in | std::ifstream::binary);
	if (!file.is_open()) {
		std::cout << console_prefix << "File cannot be opened.";
		return -1;
	}

	// check size
	file.ignore(std::numeric_limits<std::streamsize>::max());
	if (file.fail() || !file.eof()) {
		file.close();
		std::cout << console_prefix << "File stream error.";
		return -1;
	}
	auto length = file.gcount();
	if ((std::size_t)length > maxlen) {
		file.close();
		std::cout << console_prefix << "File larger than " << maxlen << " bytes.";
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
		std::cout << console_prefix << "File read error.";
		return -1;
	}
	file.close();

	std::cout << console_prefix << "Got " << length << " bytes.";

	// copy to memory
	for (auto i = 0; i < length; ++i) {
		memory[i] =  (i8080_word_t)buf[i];
	}

	return 0;
}

i8080_longjmp_context::i8080_longjmp_context(
	const struct i8080 *i8080_, i8080_fatal_error err_type_,...) noexcept
{
	i8080 = copy_i8080(i8080_);
	err_type = err_type_;

	io_port = 0;
	io_word = 0;
	bdos_callno = -1;
	at_addr = 0;

	std::va_list args;
	va_start(args, err_type_);

	switch (err_type_) 
	{
		case i8080_fatal_error::UnhandledIOWrite:
			io_port = va_arg(args, i8080_word_t);
			io_word = va_arg(args, i8080_word_t);
			break;

		case i8080_fatal_error::UnhandledIORead:
			io_port = va_arg(args, i8080_word_t);
			break;

		case i8080_fatal_error::UnhandledCPMBDOSCall:
			bdos_callno = va_arg(args, int);
			break;

		case i8080_fatal_error::UnhandledCPMBIOSCall:
			at_addr = va_arg(args, i8080_addr_t);
			break;
	}

	va_end(args);
}

i8080_longjmp_context::i8080_longjmp_context(void) noexcept 
{
	i8080 = nullptr;
	err_type = i8080_fatal_error::None;
	io_port = 0;
	io_word = 0;
	bdos_callno = -1;
	at_addr = 0;
}

struct i8080* i8080_longjmp_context::get_i8080(void) const noexcept { return i8080; }

void i8080_longjmp_context::copy_remaining(const i8080_longjmp_context& other) noexcept
{
	err_type = other.err_type;
	io_port = other.io_port;
	io_word = other.io_word;
	bdos_callno = other.bdos_callno;
	at_addr = other.at_addr;
}

i8080_longjmp_context& 
i8080_longjmp_context::operator=(i8080_longjmp_context&& other) noexcept
{
	if (this != &other) {
		free_i8080(i8080);
		i8080 = other.i8080; // steal
		other.i8080 = nullptr;
		copy_remaining(other);
	}
	return *this;
}

i8080_longjmp_context&
i8080_longjmp_context::operator=(const i8080_longjmp_context& other) noexcept
{
	if (this != &other) {
		free_i8080(i8080);
		i8080 = copy_i8080(other.i8080);
		copy_remaining(other);
	}
	return *this;
}

i8080_longjmp_context::~i8080_longjmp_context() noexcept { free_i8080(i8080); }

struct i8080* i8080_longjmp_context::copy_i8080(const struct i8080* src) const noexcept
{
	auto ni8080 = static_cast<struct i8080*>(::operator new(sizeof(struct i8080), std::nothrow));
	if (ni8080 == nullptr) {
		free_i8080(ni8080);
		std::terminate();
	}
	std::memcpy(ni8080, src, sizeof(struct i8080));
	return ni8080;
}

void i8080_longjmp_context::free_i8080(struct i8080* ptr) const noexcept
{
	if (ptr) ::operator delete(ptr, std::nothrow);
}
