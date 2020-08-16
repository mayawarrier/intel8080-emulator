
#ifndef FRONTEND_TEST_H
#define FRONTEND_TEST_H

#include <cstddef>
#include "i8080_types.h"

struct i8080;

// 65,536 bytes aka 64KB
constexpr auto VM_64K = 0x10000;
// all tests share the same buffer
extern i8080_word_t VM_MEMORY[VM_64K];

constexpr auto CPM_TPA_BEGIN = 0x0100;
constexpr auto CPM_TPA_SIZE = 0xe300;

enum i8080_fatal_error
{
	None,
	UnhandledIOWrite,
	UnhandledIORead,
	UnhandledInterrupt,
	UnhandledCPMBDOSCall,
	UnhandledCPMBIOSCall
};

// Context to recover from on longjmp.
// Functionally the same as an exception. Throwing
// the real thing across the C/C++ boundary is unsafe.
struct i8080_longjmp_context final
{
	i8080_fatal_error err_type;
	i8080_word_t io_port;
	i8080_word_t io_word;
	int bdos_callno;
	i8080_addr_t at_addr;
	i8080_longjmp_context(void) noexcept;
	i8080_longjmp_context(const struct i8080 *i8080_, i8080_fatal_error err_type_,...) noexcept;
	i8080_longjmp_context& operator=(i8080_longjmp_context&& other) noexcept;
	i8080_longjmp_context& operator=(const i8080_longjmp_context& other) noexcept;
	struct i8080* get_i8080(void) const noexcept;
	~i8080_longjmp_context() noexcept;
private:
	struct i8080* i8080;
	void free_i8080(struct i8080* ptr) const noexcept;
	struct i8080* copy_i8080(const struct i8080* src) const noexcept;
	void copy_remaining(const i8080_longjmp_context& other) noexcept;
};


// Load and run TST8080.COM, CPUTEST.COM, and INTERRUPTS.COM from tests/.
// Returns 0 if successful.
int libi8080_default_tests(void);

// Load and run an 8080 binary at a given filepath.
// Returns 0 if successful.
int libi8080_user_test(const char filepath[]);


int libcpm80vm_default_tests(void);

int libcpm80vm_user_test(const char filepath[]);


// Prompts user for [yn] response and returns result.
bool yesno_prompt(const char prompt[]);

// Dump the value of all registers, flags, cycles and
// (optionally) the contents of memory into memdump.bin.
void i8080_dump_stats_to_console(const struct i8080* i8080);

// Load into memory the contents at filepath.
// Prints error messages to console.
// Returns 0 if successful, else -1.
int load_memory(const char filepath[], i8080_word_t* memory, std::size_t maxlen, const char console_prefix[] = "");


#endif
