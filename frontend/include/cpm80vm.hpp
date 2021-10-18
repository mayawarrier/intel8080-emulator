
#ifndef CPM80_VM_H
#define CPM80_VM_H 

#include <cstddef>
#include <memory>
#include "i8080/i8080.h"

enum cpm80vm_exitcode
{
    VM_Success,
    VM_ProgramExit,
    VM_UnimplementedSyscall
};

class cpm80vm
{
public:
	typedef i8080_word_t(*interrupt_handler)(cpm80vm*);
	typedef i8080_word_t(*input_handler)(cpm80vm*, i8080_addr_t port); // I/O read	
	typedef void(*output_handler)(cpm80vm*, i8080_addr_t port, i8080_word_t word); // I/O write

private:
	const unsigned kmemsize;
    const i8080_addr_t ccp_addr;
	const i8080_addr_t bdos_addr;
    const i8080_addr_t bios_table_addr;
	const i8080_addr_t bios_impl_addr;

	struct i8080 cpu;
	std::unique_ptr<i8080_word_t[]> memory;

	// # of calls to CP/M warm boot.
	int num_wboot;
	cpm80vm_exitcode exitcode;

	const input_handler io_read;
	const output_handler io_write;
	const interrupt_handler interrupt_read;

	friend struct c_cpm80vm;

public:
	cpm80vm(
		unsigned kmemsize, // Memory size in kilobytes (max 64).
		input_handler io_read_handler = nullptr,
		output_handler io_write_handler = nullptr,
		interrupt_handler interrupt_handler = nullptr);

    inline const struct i8080* get_cpu(void) const noexcept { return &cpu; }
    inline const i8080_word_t* get_memory(void) const noexcept { return memory.get(); }

    // Get memory size in kilobytes.
    inline unsigned get_memsize_kb(void) const noexcept { return kmemsize; }
    // Get memory size in bytes.
    inline std::size_t get_memsize(void) const noexcept { return (std::size_t)kmemsize * 1024; }

    // Write to program memory. 
    // Throws std::out_of_range if range overlaps CP/M 2.2 system memory 
    // (< 0x0100 and >= 1024 * (get_memsize_kb() - 7)).
    void program_write(const char* data, std::size_t begin, std::size_t end);

    // Step through program.
    cpm80vm_exitcode step(void) noexcept;

    // Interrupt the CPU.
    void interrupt(void) noexcept;

    // Reset execution to beginning.
    void reset(void) noexcept;

	// Continue until program exit or next failure.
    cpm80vm_exitcode resume(void) noexcept;

    // Reset and continue until program exit or next failure.
    cpm80vm_exitcode restart(void) noexcept;

    static cpm80vm* owner_of(const struct i8080* cpu) noexcept;

private:
    void next(void) noexcept;
    cpm80vm_exitcode run(void) noexcept;
	
	bool okay(void) const noexcept;

    void warm_boot(void) noexcept;
    bool try_host_call(i8080_addr_t) noexcept;
};

#endif /* CPM80_VM_H */ 
