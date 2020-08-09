
#include <cstdio>
#include <cstdlib>
#include <csetjmp>

#include <string>
#include <limits>
#include <iostream>
#include <chrono>
#include <atomic>
#include <thread>

#include "i8080.h"
#include "i8080_opcodes.h"
#include <test.hpp>

#define libi8080_EXIT (30)
#define libi8080_CPM_TPA_BEGIN (0x0100)
#define libi8080_CPM_TPA_SIZE (0xe300)

static i8080_word_t memory_read(const struct i8080 *, i8080_addr_t a) { return VM_MEMORY[a]; }
static void memory_write(const struct i8080 *, i8080_addr_t a, i8080_word_t w) { VM_MEMORY[a] = w; }

/* For unrecoverable errors that will likely 
   Have to use longjmp to do this instead of exceptions as exceptions cannot be
   thrown safely across the C/C++ boundary */
std::jmp_buf i8080_fatal;

static void fatal_io_write(const struct i8080 *, i8080_addr_t port, i8080_word_t word) {
	std::printf("\nUnhandled I/O write to port 0x%02x, word: 0x%02x\n", port & 0xff, word);
	std::longjmp(i8080_fatal, 1);
}

static i8080_word_t fatal_io_read(const struct i8080 *, i8080_addr_t port) {
	std::printf("\nUnhandled I/O read from port 0x%02x\n", port & 0xff);
	std::longjmp(i8080_fatal, 1);
#if (__cplusplus <= 201703L)
	/* return may be required to make compiler happy */
	return 0;
#endif
}

static i8080_word_t fatal_interrupt_read(const struct i8080 *) {
	std::printf("\nUnhandled interrupt\n");
	std::longjmp(i8080_fatal, 1);
#if (__cplusplus <= 201703L)
	return 0;
#endif
}

struct cpm80_fake_vm {
	struct i8080 cpu;
	struct i8080_monitor monitor;
	int num_wboot; // number of calls to CP/M warm boot
};

// Emulate just as much of CP/M BDOS and WBOOT as required to run included test binaries.
static int libi8080_cpm_call(struct i8080* cpu)
{
	bool unhandled = false;
	switch (cpu->pc)
	{
		case 0xfa04: {
			// came here from WBOOT
			struct cpm80_fake_vm* vm = static_cast<struct cpm80_fake_vm*>(cpu->udata);
			vm->num_wboot += 1;

			if (vm->num_wboot == 1) {
				/* CP/M 
				   We have no booting to do; jump straight to the program */
				cpu->pc = libi8080_CPM_TPA_BEGIN;
			}

			if (vm->num_wboot == 2) {
				// The program is re-booting CP/M i.e. has terminated
				return libi8080_EXIT;
			}
		} break;

		case 0x0006: {
			// came here from BDOS
			switch (cpu->c)
			{
				// print single char
				case 0x02: std::printf("%c", cpu->e);
					break;

				// print CP/M string ($-terminated)
				case 0x09: {
					i8080_word_t c;
					i8080_addr_t ptr = (i8080_addr_t)(cpu->d << 8) | cpu->e; // starts at [DE]
					while ((c = cpu->memory_read(cpu, ptr)) != '$') {
						std::printf("%c", c);
						ptr++;
					}
				} break;

				default: {
					std::cout << "Unhandled CP/M BDOS call, code: " << cpu->c << std::endl;
					unhandled = true;
				} break;
			}
		} break;

		default: {
			std::cout << "Unhandled CP/M BIOS call, address: " << cpu->pc << std::endl;
			unhandled = true;
		} break;
	}

	// crash
	if (unhandled) std::longjmp(i8080_fatal, 1);

	return 0;
}

static i8080_word_t libi8080_cpm_interrupt_read(const struct i8080 *) {
	/* For the interrupts test, RST 0 causes
	   a call to CP/M WBOOT which we can catch later
	   to detect when the program exits. */
	return i8080_RST_0;
}

static void libi8080_setup_cpm_environment(struct cpm80_fake_vm& vm)
{
	i8080_init(&vm.cpu);

	vm.cpu.memory_read = memory_read;
	vm.cpu.memory_write = memory_write;
	vm.cpu.interrupt_read = libi8080_cpm_interrupt_read;

	vm.cpu.io_read = fatal_io_read;
	vm.cpu.io_write = fatal_io_write;

	// triggered by RST 7 (0xff)
	vm.monitor.enter_monitor = libi8080_cpm_call;
	vm.cpu.monitor = &vm.monitor;

	vm.num_wboot = 0;
	vm.cpu.udata = &vm;

	// jump to WBOOT (0xfa03) on startup
	vm.cpu.memory_write(&vm.cpu, 0x0000, i8080_JMP);
	vm.cpu.memory_write(&vm.cpu, 0x0001, 0x03);
	vm.cpu.memory_write(&vm.cpu, 0x0002, 0xfa);

	/* Put breakpoints at WBOOT (0xfa03) and BDOS (0x0005)
	   so we regain control and can handle these calls. */
	vm.cpu.memory_write(&vm.cpu, 0xfa03, i8080_RST_7); // WBOOT does not return
	vm.cpu.memory_write(&vm.cpu, 0x0005, i8080_RST_7);
	vm.cpu.memory_write(&vm.cpu, 0x0006, i8080_RET);
}

static void libi8080_setup_usual_environment(struct i8080& cpu)
{
	i8080_init(&cpu);

	cpu.memory_read = memory_read;
	cpu.memory_write = memory_write;

	/* If user-supplied test does any I/O (or sends an
	   interrupt somehow), we can't do anything sensible!
	   The safest thing to do is just crash. */
	cpu.io_read = fatal_io_read;
	cpu.io_write = fatal_io_write;
	cpu.interrupt_read = fatal_interrupt_read;
}

static void overwrite_memory(struct i8080& cpu, i8080_addr_t dest, i8080_word_t word, std::size_t nwords)
{
	for (std::size_t i = dest; i < dest + nwords; ++i) {
		cpu.memory_write(&cpu, (i8080_addr_t)i, word);
	}
}

static int run_emulator(struct i8080& cpu)
{
	while (!i8080_next(&cpu));

	int mon_res = cpu.exitcode;
	if (mon_res == libi8080_EXIT) return 0;
	return mon_res;
}

static int run_emulator_synchronizable(struct i8080& cpu, std::atomic_flag& lock)
{
	while (1) {
		while (lock.test_and_set(std::memory_order_acquire));
		if (i8080_next(&cpu)) break;
		lock.clear(std::memory_order_release);
	}

	int mon_res = cpu.exitcode;
	if (mon_res == libi8080_EXIT) return 0;
	return mon_res;
}

static int load_and_run(struct i8080& cpu, const std::string& filepath, i8080_addr_t from, std::size_t maxlen)
{
	int err = load_memory(filepath.c_str(), VM_MEMORY + from, maxlen);
	if (err) return err;

	return run_emulator(cpu);
}

static int load_and_run_interrupted(struct i8080& cpu, const std::string& filepath, i8080_addr_t from, std::size_t maxlen, int ms_to_interrupt)
{
	int err = load_memory(filepath.c_str(), VM_MEMORY + from, maxlen);
	if (err) return err;

	// synchronize interrupt with cpu
	#if __cplusplus >= 202002L
	std::atomic_flag lock; // ATOMIC_FLAG_INIT is deprecated from c++20
	#else
	std::atomic_flag lock = ATOMIC_FLAG_INIT;
	#endif

	std::thread intgen_thread([&] {
		std::this_thread::sleep_for(std::chrono::milliseconds(ms_to_interrupt));
		while (lock.test_and_set(std::memory_order_acquire));
		i8080_interrupt(&cpu);
		lock.clear(std::memory_order_release);
	});

	err = run_emulator_synchronizable(cpu, lock);

	intgen_thread.join();

	return err;
}

static void libi8080_cpm_reset(struct cpm80_fake_vm& vm)
{
	vm.num_wboot = 0;
	vm.cpu.cycles = 0;
	// overwrite the transient program area
	overwrite_memory(vm.cpu, libi8080_CPM_TPA_BEGIN, 0xff, libi8080_CPM_TPA_SIZE);
	i8080_reset(&vm.cpu);
}

// default test paths to run if no path given
static const int num_default_tests = 3;
static const std::string TST8080_COM = "tests/TST8080.COM";
static const std::string CPUTEST_COM = "tests/CPUTEST.COM";
static const std::string INTERRUPTS_COM = "tests/INTERRUPT.COM";

static int libi8080_TST8080_COM(struct cpm80_fake_vm& vm)
{
	std::cout << "\nLoading file \"TST8080.COM\"";
	std::cout << "\n-------------------------------------------------------------------------\n";
	int err = load_and_run(vm.cpu, TST8080_COM, libi8080_CPM_TPA_BEGIN, libi8080_CPM_TPA_SIZE);
	std::cout << "\n-------------------------------------------------------------------------\n";
	std::cout << "Ran for " << vm.cpu.cycles << " cycles.\n\n";
	return err;
}

static int libi8080_CPUTEST_COM(struct cpm80_fake_vm& vm)
{
	std::cout << "\nLoading file \"CPUTEST.COM\"";
	std::cout << "\n-------------------------------------------------------------------------\n";
	int err = load_and_run(vm.cpu, CPUTEST_COM, libi8080_CPM_TPA_BEGIN, libi8080_CPM_TPA_SIZE);
	std::cout << "\n-------------------------------------------------------------------------\n";
	std::cout << "Ran for " << vm.cpu.cycles << " cycles.\n\n";
	return err;
}

static int libi8080_INTERRUPTS_COM(struct cpm80_fake_vm& vm)
{
	std::cout << "\nLoading file \"INTERRUPT.COM\"";
	std::cout << "\n-------------------------------------------------------------------------\n";
	int err = load_and_run_interrupted(vm.cpu, INTERRUPTS_COM, libi8080_CPM_TPA_BEGIN, libi8080_CPM_TPA_SIZE, 5000);
	std::cout << "\n-------------------------------------------------------------------------\n";
	std::cout << "Ran for " << vm.cpu.cycles << " cycles.\n\n";
	return err;
}

int libi8080_default_tests()
{
	struct cpm80_fake_vm vm;
	libi8080_setup_cpm_environment(vm);

	if (setjmp(i8080_fatal) != 0) {
		// drop execution
		i8080_dump_stats(vm.cpu);
		std::exit(EXIT_FAILURE);
	}

	int err = 0;

	libi8080_cpm_reset(vm);
	err |= libi8080_TST8080_COM(vm);

	libi8080_cpm_reset(vm);
	err |= libi8080_CPUTEST_COM(vm);

	libi8080_cpm_reset(vm);
	err |= libi8080_INTERRUPTS_COM(vm);

	system("pause");

	return err;
}

int libi8080_user_test(std::string& filepath)
{
	bool is_TST8080_COM = filepath == TST8080_COM;
	bool is_CPUTEST_COM = filepath == CPUTEST_COM;
	bool is_INTERRUPTS_COM = filepath == INTERRUPTS_COM;

	if (is_TST8080_COM || is_CPUTEST_COM || is_INTERRUPTS_COM) {
		// this is one of the included tests, enable CP/M emulation
		struct cpm80_fake_vm vm;
		libi8080_setup_cpm_environment(vm);
		libi8080_cpm_reset(vm);

		if (setjmp(i8080_fatal) != 0) {
			i8080_dump_stats(vm.cpu);
			std::exit(EXIT_FAILURE);
		}

		if (is_TST8080_COM) return libi8080_TST8080_COM(vm);
		if (is_CPUTEST_COM) return libi8080_CPUTEST_COM(vm);
		if (is_INTERRUPTS_COM) return libi8080_INTERRUPTS_COM(vm);
	}

	// this is a user test!
	struct i8080 i8080;
	libi8080_setup_usual_environment(i8080);
	i8080_reset(&i8080);

	if (setjmp(i8080_fatal) != 0) {
		// drop execution
		i8080_dump_stats(i8080);
		std::exit(EXIT_FAILURE);
	}

	std::cout << "\nLoading file \"" << filepath.c_str() << "\"";
	std::cout << "\n-------------------------------------------------------------------------\n";
	int err = load_and_run(i8080, filepath, 0x0000, VM_64K);
	std::cout << "\n-------------------------------------------------------------------------\n";
	std::cout << "Ran for " << i8080.cycles << " cycles.\n\n";
	return err;
}
