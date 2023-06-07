
#ifndef EMU_HPP
#define EMU_HPP

#include <cstdarg>
#include "i8080/i8080.h"

struct emu_opts
{
    // Convert keyboard interrupts into 8080 interrupts.
    bool conv_key_intr;
    // Emulate CP/M-80 console.
    bool use_cpm_con;

    // sensible defaults
    emu_opts() : 
        conv_key_intr(false), 
        use_cpm_con(true)
    {}

    emu_opts(bool conv_key_intr, bool use_cpm_con) :
        conv_key_intr(conv_key_intr),
        use_cpm_con(use_cpm_con)
    {}
};

// (Re)initialize emulator.
int emu_init(emu_opts opts);

// Load binary.
int emu_load(const char* filepath);

// Run binary.
int emu_run(void);

// Free memory.
// Not necessary to call this 
// before calling emu_init() again.
void emu_destroy(void);


enum emu_err
{
    // Could not set up or intercept 
    // keyboard interrupts.
    EMU_EKEYINTR = -2,
    // Could not read file.
    EMU_EFILE = -1,

    EMU_EHNDLR = i8080_EHNDLR,
    EMU_EOPCODE = i8080_EOPCODE,
    // Unimplemented BDOS call.
    EMU_EBDOS,
    // Program called debugger.
    EMU_EDBGR
};

// Print error message.
void emu_printerr(const char* format, ...) noexcept;
void emu_vprinterr(const char* format, std::va_list vlist) noexcept;

// Exit after an error.
void emu_errexit(int err);

#endif