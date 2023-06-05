
#ifndef EMU_HPP
#define EMU_HPP

#include <cstdlib>
#include <cstdarg>
#include <string>
#include <vector>
#include <utility>

#include "i8080/i8080.h"

enum emu_err
{
    // Could not set up or intercept 
    // keyboard interrupts.
    EMU_EKEYINTR = -3,
    // Could not read file.
    EMU_EFILE = -2,
    // Caught C++ exception.
    EMU_EEXC = -1,

    EMU_EHNDLR = i8080_EHNDLR,
    EMU_EOPCODE = i8080_EOPCODE,
    // Unimplemented BDOS call.
    EMU_EBDOS,
    // Program called debugger.
    EMU_EDBGR
};

struct emu_opts
{
    bool conv_key_intr;
    bool use_cpm_con;
};

// (Re)initialize emulator.
int emu_init(emu_opts opts);

// Load binary.
int emu_load(const char* filepath);

// Run binary.
int emu_run(void);

struct emu_teststate
{
    i8080 cpu;
    std::vector<std::pair<i8080_addr_t, i8080_word_t>> ram;
};

struct emu_testdata
{
    std::string name;
    emu_opts opts;
    emu_teststate init_state;
    emu_teststate final_state;
    // ignored if no console output
    std::string final_con_out;
};

// Run test.
// for return value < 0, test failed to run.
// for return value > 0, test did not pass.
int emu_test(emu_testdata& test);

// Free memory.
// A call to this is not necessary before 
// calling emu_init() again.
void emu_destroy(void);


void emu_vprinterr(const char* format, std::va_list vlist) noexcept;

inline void emu_printerr(const char* format, ...) noexcept
{
    std::va_list args;
    va_start(args, format);
    emu_vprinterr(format, args);
    va_end(args);
}

const char* emu_errname(int err) noexcept;
#endif