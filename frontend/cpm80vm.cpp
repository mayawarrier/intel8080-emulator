
#include <cstddef>
#include <cassert>
#include <cstdio>

#include <stdexcept>
#include <algorithm>
#include <array>

#include "i8080/i8080_opcodes.h"
#include <cpm80vm.hpp>

// Entry point to user programs.
static constexpr i8080_addr_t CPM_TPA_ORIGIN = 0x0100;

#define lobyte(x) ((x) & 0xff)
#define hibyte(x) (((x) & 0xff00) >> 8)

// Sequence that returns control to the VM.
static const std::array<i8080_word_t, 3> HOST_CALL =
{
	i8080_OUT,
	// port doesn't matter. 
	// calls are identified by location
	0xff,
	i8080_RET
};
// Subtracted from PC to get location of host call.
static constexpr i8080_addr_t HOST_CALL_PC_OFFSET = 2;


// Get CCP base address. May vary by machine.
static inline i8080_addr_t get_ccp_addr(unsigned kmemsize)
{
    // (CP/M 2.2 alteration guide, sample GETSYS)
    return (i8080_addr_t)(1024 * (kmemsize - 7));
}

inline cpm80vm* cpm80vm::owner_of(const struct i8080* cpu) noexcept
{
    cpm80vm* owner = static_cast<cpm80vm*>(cpu->udata);
    assert(cpu == owner->get_cpu());
    return owner;
}

struct c_cpm80vm final
{
	static i8080_word_t cpu_memory_read(const struct i8080* cpu, i8080_addr_t addr) noexcept 
	{
		return *(cpm80vm::owner_of(cpu)->memory.get() + addr);
	}
	static void cpu_memory_write(const struct i8080* cpu, i8080_addr_t addr, i8080_word_t word) noexcept 
	{
		*(cpm80vm::owner_of(cpu)->memory.get() + addr) = word;
	}

	static i8080_word_t cpu_io_read(const struct i8080* cpu, i8080_addr_t port) noexcept
	{
		cpm80vm* vm = cpm80vm::owner_of(cpu);
        if (vm->io_read) return vm->io_read(vm, port);
        else {
            std::fprintf(stderr, "\nVM: Ignored read from port "
                "%d by address 0x%04x", lobyte(port), cpu->pc - 2);
            return 0;
        }
	}
    static i8080_word_t cpu_interrupt_read(const struct i8080* cpu) noexcept
    {
        cpm80vm* vm = cpm80vm::owner_of(cpu);
        if (vm->interrupt_read) return vm->interrupt_read(vm);
        else {
            std::fprintf(stderr, "\nVM: Ignored interrupt at %lu cycles", cpu->cycles);
            return i8080_NOP;
        }
    }
    static void cpu_io_write(const struct i8080* cpu, i8080_addr_t port, i8080_word_t word) noexcept
    {
        cpm80vm* vm = cpm80vm::owner_of(cpu);
        i8080_addr_t hostcall_addr = cpu->pc - HOST_CALL_PC_OFFSET;

        if (vm->try_host_call(hostcall_addr)) return;
        else if (vm->io_write) vm->io_write(vm, port, word);
        else {
            std::fprintf(stderr, "\nVM: Ignored write to port "
                "%d by address 0x%04x w/ data: %d", lobyte(port), cpu->pc - 2, word);
        }
    }
};

cpm80vm::cpm80vm(
    unsigned kmemsize,
    cpm80vm::input_handler io_read_handler,
    cpm80vm::output_handler io_write_handler,
    cpm80vm::interrupt_handler interrupt_handler) :

    kmemsize(kmemsize),
    io_read(io_read_handler),
    io_write(io_write_handler),
    interrupt_read(interrupt_handler),

    // CCP base address.
    ccp_addr(get_ccp_addr(kmemsize)),

    // BDOS base address.
    // (CP/M 2.2 alteration guide, pg 17)
    bdos_addr(get_ccp_addr(kmemsize) + 0x0806),

    // BIOS jump table base address.
    bios_table_addr(get_ccp_addr(kmemsize) + 0x1600),

    // BIOS functions base address.
    bios_impl_addr((i8080_addr_t)(1024 * (std::size_t)kmemsize - 17 * HOST_CALL.size()))
{
    if (kmemsize < 1 || kmemsize > 64)
        throw std::out_of_range("Invalid memory size.");

    std::size_t memsize = 1024 * (std::size_t)kmemsize;
    memory = std::unique_ptr<i8080_word_t[]>(new i8080_word_t[memsize]);

    // init 8080
    cpu.enable = 1;
    cpu.udata = this; // set owner
    cpu.memory_read = &c_cpm80vm::cpu_memory_read;
    cpu.memory_write = &c_cpm80vm::cpu_memory_write;
    cpu.io_read = &c_cpm80vm::cpu_io_read;
    cpu.io_write = &c_cpm80vm::cpu_io_write;
    cpu.interrupt_read = &c_cpm80vm::cpu_interrupt_read;

    // fill BIOS jump table + host calls
    i8080_addr_t bios_table_ptr = bios_table_addr;
    i8080_addr_t bios_impl_ptr = bios_impl_addr;
    for (auto i = 0; i < 17; ++i)
    {
        // is this indirection necessary?
        memory[bios_table_ptr] = i8080_JMP;
        memory[bios_table_ptr + 1] = lobyte(bios_impl_ptr);
        memory[bios_table_ptr + 2] = hibyte(bios_impl_ptr);
        std::copy(HOST_CALL.begin(), HOST_CALL.end(), &memory[bios_impl_ptr]);

        bios_table_ptr += 3;
        bios_impl_ptr += (i8080_addr_t)HOST_CALL.size();
    }

    // BDOS host call
    std::copy(HOST_CALL.begin(), HOST_CALL.end(), &memory[bdos_addr]);

    // Jump to boot (BIOS 0) on startup
    memory[0x0000] = i8080_JMP;
    memory[0x0001] = lobyte(bios_table_addr);
    memory[0x0002] = hibyte(bios_table_addr);

    reset();
}

void cpm80vm::program_write(const char* data, std::size_t begin, std::size_t end)
{
    if (end < begin || begin < CPM_TPA_ORIGIN || end >= ccp_addr)
        throw std::out_of_range("program_write: Invalid range.");
        
    std::copy(data, data + end - begin + 1, &memory[begin]);
}

// True if okay to continue running.
inline bool cpm80vm::okay(void) const noexcept
{
    return exitcode == cpm80vm_exitcode::VM_Success;
}

void cpm80vm::interrupt(void) noexcept
{
    if (!okay()) {
        std::fprintf(stderr, "\nVM: Cannot send interrupt after exit");
        return;
    }
    i8080_interrupt(&cpu);
}

inline void cpm80vm::next(void) noexcept
{
    assert(okay());
    int err = i8080_next(&cpu);
    assert(!err);
    (void)err; // may be unused
}

cpm80vm_exitcode cpm80vm::step(void) noexcept
{
    if (okay()) next();
    return exitcode;
}

inline cpm80vm_exitcode cpm80vm::run(void) noexcept
{
    while (okay()) next();
    return exitcode;
}

void cpm80vm::reset(void) noexcept
{
    i8080_reset(&cpu);
    num_wboot = 0;
    exitcode = cpm80vm_exitcode::VM_Success;
}

cpm80vm_exitcode cpm80vm::restart(void) noexcept
{
    reset();
    return run();
}
cpm80vm_exitcode cpm80vm::resume(void) noexcept
{
    // cannot resume past end of program
    if (exitcode == cpm80vm_exitcode::VM_ProgramExit)
        return exitcode;

    exitcode = cpm80vm_exitcode::VM_Success;
    return run();
}

void cpm80vm::warm_boot(void) noexcept
{
	assert(okay());

	num_wboot += 1;
	switch (num_wboot)
	{
		// Came here from startup
		case 1:
			// Make BDOS entry
			// (program may use this to determine program area) 
			memory[0x0005] = i8080_JMP;
			memory[0x0006] = lobyte(bdos_addr);
			memory[0x0007] = hibyte(bdos_addr);

			// Make WBOOT entry
			// (program may use this to determine BIOS call addrs)
			memory[0x0000] = i8080_JMP;
			memory[0x0001] = lobyte(bios_table_addr + 3);
			memory[0x0002] = hibyte(bios_table_addr + 3);

            // Push WBOOT return address onto stack
            cpu.sp = ccp_addr + 0x07aa;
            memory[cpu.sp] = hibyte(bios_table_addr + 3); cpu.sp--;
            memory[cpu.sp] = lobyte(bios_table_addr + 3);

            // start program
            cpu.pc = CPM_TPA_ORIGIN;

			break;

		// Came here from program, terminate
        case 2: exitcode = cpm80vm_exitcode::VM_ProgramExit;
			break;

		default: assert("Bad num_wboot");
	}
}

bool cpm80vm::try_host_call(i8080_addr_t addr) noexcept
{
    assert(okay());

    // todo: char escape

    // is BIOS?
	if (addr >= bios_impl_addr)
	{
        // implemented BOOT, WBOOT, CONST, CONIN, CONOUT

		int bios_callno = (int)((addr - bios_impl_addr) / HOST_CALL.size());
		switch (bios_callno)
		{
			// BOOT (Cold boot)
			case 0: warm_boot();
				break;

			// WBOOT (Warm boot)
			case 1: warm_boot();
				break;

			// CONST (Console status)
			case 2: cpu.a = 0x00; // always ready
				break;

			// CONIN (Console in)
			case 3: cpu.a = (i8080_word_t)std::getchar();
				break;

			// CONOUT (Console out)
			case 4: std::putchar((unsigned char)cpu.c);
				break;

            default: exitcode = cpm80vm_exitcode::VM_UnimplementedSyscall;
                std::fprintf(stderr, "\nVM: BIOS call %d not implemented", bios_callno);
				break;
		}
    }
    // is BDOS?
    else if (addr == bdos_addr)
    {
        // implemented functions 0, 2, 9

        int bdos_callno = cpu.c;
		switch (bdos_callno)
		{
			// BDOS 0: system reset (warm boot)
			case 0: warm_boot();
				break;

			// BDOS 2: print char
			case 2: std::putchar((unsigned char)cpu.e);
				break;

			// BDOS 9: print string ($-terminated in CP/M)
			case 9: {             
				i8080_addr_t str_ptr = ((i8080_addr_t)cpu.d << 8) | cpu.e;
                unsigned char c;
				while ((c = (unsigned char)cpu.memory_read(&cpu, str_ptr)) != '$') {
                    std::putchar(c);
					str_ptr++;
				}
				break;
			}

			default: exitcode = cpm80vm_exitcode::VM_UnimplementedSyscall;
                std::fprintf(stderr, "\nVM: BDOS function %d not implemented", bdos_callno);
				break;
		}
    }
    else return false;

    return true;
}
