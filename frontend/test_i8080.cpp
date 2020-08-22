
#include <cstdio>
#include <cstddef>
#include <cstring>
#include <csetjmp>

#include <iostream>
#include <chrono>
#include <atomic>
#include <thread>

#include "i8080.h"
#include "i8080_opcodes.h"
#include <test.hpp>

#define libi8080_EXIT (30)

static i8080_word_t memory_read(const struct i8080 *, i8080_addr_t a) { return VM_MEMORY[a]; }
static void memory_write(const struct i8080 *, i8080_addr_t a, i8080_word_t w) { VM_MEMORY[a] = w; }

/* Have to use longjmp to do this instead of exceptions as
 * exceptions cannot be thrown safely across the C/C++ boundary */
static std::jmp_buf on_i8080_fatal;
// Context to recover from; clobber safe
static struct i8080_longjmp_context i8080_fatal_context;

static void fatal_io_write(const struct i8080 *cpu, i8080_addr_t port, i8080_word_t word) noexcept
{
	i8080_fatal_context = i8080_longjmp_context(cpu, i8080_fatal_error::UnhandledIOWrite, port & 0xff, word);
	std::longjmp(on_i8080_fatal, 1);
}

static i8080_word_t fatal_io_read(const struct i8080 *cpu, i8080_addr_t port) noexcept
{
	i8080_fatal_context = i8080_longjmp_context(cpu, i8080_fatal_error::UnhandledIORead, port & 0xff);
	std::longjmp(on_i8080_fatal, 1);
#if (__cplusplus <= 201703L)
	return 0; // might be required to make compiler happy?
#endif
}

static i8080_word_t fatal_interrupt_read(const struct i8080 *cpu) noexcept
{
	i8080_fatal_context = i8080_longjmp_context(cpu, i8080_fatal_error::UnhandledInterrupt);
	std::longjmp(on_i8080_fatal, 1);
#if (__cplusplus <= 201703L)
	return 0; // might be required to make compiler happy?
#endif
}

/* A "fake" cpm80_vm designed to provide just enough emulation to
 * run 8080 binaries that need entry points or functions from CP/M. */
struct cpm80_fake_vm final
{
	struct i8080 cpu;
	struct i8080_monitor monitor;
	int num_wboot; // number of calls to CP/M warm boot
};

// Emulate just as much of CP/M BDOS and WBOOT as required.
static int libi8080_cpm_call(struct i8080* cpu) noexcept
{
	bool fatal = false;
	switch (cpu->pc)
	{
		case 0xfa04: {
			// came here from WBOOT
			struct cpm80_fake_vm* vm = static_cast<struct cpm80_fake_vm*>(cpu->udata);
			vm->num_wboot += 1;

			if (vm->num_wboot == 1) {
				/* This would have been CP/M's startup boot process!
				   We have no booting to do; jump straight to the program */
				cpu->pc = CPM_TPA_BEGIN;
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
					char strbuf[128]; char c; int i = 0;
					// fill with nulls
					std::memset(strbuf, '\0', 128 * sizeof(char));

					i8080_addr_t ptr = (i8080_addr_t)(cpu->d << 8) | cpu->e; // starts at [DE]
					while ((c = (char)cpu->memory_read(cpu, ptr)) != '$') {
						strbuf[i++] = c;
						if (i == 128) {
							// buffer full, flush to console
							std::printf("%s", strbuf);
							std::memset(strbuf, '\0', 128 * sizeof(char));
							i = 0;
						}
						ptr++;
					}
					std::printf("%s", strbuf); // write remaining chars
				} break;

				default: {
					i8080_fatal_error err = i8080_fatal_error::UnhandledCPMBDOSCall;
					i8080_fatal_context = i8080_longjmp_context(cpu, err, (int)cpu->c);
					fatal = true;
				} break;
			}
		} break;

		default: {
			i8080_fatal_error err = i8080_fatal_error::UnhandledCPMBIOSCall;
			i8080_fatal_context = i8080_longjmp_context(cpu, err, (i8080_addr_t)cpu->pc);
			fatal = true;
		} break;
	}

	// crash
	if (fatal) std::longjmp(on_i8080_fatal, 1);

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

static int i8080_run(struct i8080& cpu)
{
	while (!i8080_next(&cpu));

	int res = cpu.exitcode;
	if (res == libi8080_EXIT) return 0;
	return res;
}

static int i8080_interrupted_run(struct i8080& cpu, int ms_to_interrupt)
{
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

	while (1) {
		while (lock.test_and_set(std::memory_order_acquire));
		if (i8080_next(&cpu)) break;
		lock.clear(std::memory_order_release);
	}

	intgen_thread.join();

	int err = cpu.exitcode;
	if (err == libi8080_EXIT) return 0; // regular exit

	return err;
}

static void libi8080_cpm_reset(struct cpm80_fake_vm& vm)
{
	vm.num_wboot = 0;
	vm.cpu.cycles = 0;
	// overwrite the transient program area (EEPROMs were often blanked with 0xff)
	std::memset(VM_MEMORY + CPM_TPA_BEGIN, 0xff, CPM_TPA_SIZE * sizeof(i8080_word_t));
	i8080_reset(&vm.cpu);
}

// default test paths to run if no path given
static const char TST8080_COM[] = "bin/TST8080.COM";
static const char CPUTEST_COM[] = "bin/CPUTEST.COM";
static const char INTERRUPTS_COM[] = "bin/INTERRUPT.COM";

static int libi8080_load_and_run(struct i8080& cpu, const char filepath[], int is_cpm_mode = 1, int ms_to_interrupt = -1)
{
	int err;
	std::cout << "\n>> Loading from file \"" << filepath << "\"";
	if (is_cpm_mode) {
		err = load_memory(filepath, VM_MEMORY + CPM_TPA_BEGIN, CPM_TPA_SIZE, "\n>> ");
	} else {
		err = load_memory(filepath, VM_MEMORY, VM_64K, "\n>> ");
	}
	
	if (err) return err;
	
	std::cout << "\n--------------------------------------------\n";
	if (ms_to_interrupt <= 0) err = i8080_run(cpu);
	else err = i8080_interrupted_run(cpu, ms_to_interrupt);
	std::cout << "\n--------------------------------------------\n";
	std::cout << ">> Ran for " << cpu.cycles << " cycles.\n\n";

	return err;
}

// print debug stats
static void on_i8080_fatal_dump_stats(void)
{
	i8080_fatal_error err_type = i8080_fatal_context.err_type;
	i8080_word_t io_port = i8080_fatal_context.io_port;
	i8080_word_t io_word = i8080_fatal_context.io_word;
	int bdos_callno = i8080_fatal_context.bdos_callno;
	i8080_addr_t at_addr = i8080_fatal_context.at_addr;

	switch (err_type)
	{
		case i8080_fatal_error::UnhandledIOWrite:
			std::printf("\nUnhandled write 0x%02x to port 0x%02x\n", io_word, io_port);
			break;
		case i8080_fatal_error::UnhandledIORead:
			std::printf("\nUnhandled read from port 0x%02x\n", io_port);
			break;
		case i8080_fatal_error::UnhandledInterrupt:
			std::cout << "\nUnhandled interrupt\n";
			break;
		case i8080_fatal_error::UnhandledCPMBDOSCall:
			std::printf("\nUnhandled CP/M BDOS call, code: %d\n", bdos_callno);
			break;
		case i8080_fatal_error::UnhandledCPMBIOSCall:
			std::printf("\nUnhandled CP/M BIOS call, address: 0x%04x\n", at_addr);
			break;
	}

	i8080_dump_stats_to_console(i8080_fatal_context.get_i8080());
}

int libi8080_default_tests(void)
{
	struct cpm80_fake_vm vm;
	libi8080_setup_cpm_environment(vm);

	if (setjmp(on_i8080_fatal) != 0) {
		on_i8080_fatal_dump_stats();
		return -1;
	}

	int err = 0;

	libi8080_cpm_reset(vm);
	err |= libi8080_load_and_run(vm.cpu, TST8080_COM);

	libi8080_cpm_reset(vm);
	err |= libi8080_load_and_run(vm.cpu, CPUTEST_COM);

	libi8080_cpm_reset(vm);
	err |= libi8080_load_and_run(vm.cpu, INTERRUPTS_COM, 1, 5000);

	return err;
}

int libi8080_user_test(const char filepath[])
{
	bool is_TST8080_COM = std::strcmp(filepath, TST8080_COM) == 0;
	bool is_CPUTEST_COM = std::strcmp(filepath, CPUTEST_COM) == 0;
	bool is_INTERRUPTS_COM = std::strcmp(filepath, INTERRUPTS_COM) == 0;

	if (is_TST8080_COM || is_CPUTEST_COM || is_INTERRUPTS_COM) {
		// this is one of the included tests, enable CP/M emulation
		struct cpm80_fake_vm vm;
		libi8080_setup_cpm_environment(vm);
		libi8080_cpm_reset(vm);

		if (setjmp(on_i8080_fatal) != 0) {
			on_i8080_fatal_dump_stats();
			return -1;
		}

		if (is_TST8080_COM) return libi8080_load_and_run(vm.cpu, TST8080_COM);
		if (is_CPUTEST_COM) return libi8080_load_and_run(vm.cpu, CPUTEST_COM);
		if (is_INTERRUPTS_COM) return libi8080_load_and_run(vm.cpu, INTERRUPTS_COM, 1, 5000);
	}

	// this is a user test!
	struct i8080 i8080;
	libi8080_setup_usual_environment(i8080);
	i8080_reset(&i8080);

	if (setjmp(on_i8080_fatal) != 0) {
		on_i8080_fatal_dump_stats();
		return -1;
	}	

	return libi8080_load_and_run(i8080, filepath, 0);
}
