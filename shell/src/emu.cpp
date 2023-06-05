
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <climits>
#include <string>
#include <memory>

#include "i8080/i8080.h"
#include "i8080/i8080_opcodes.h"

#include "keyintr.hpp"
#include "emu.hpp"


static struct emu
{
    i8080 cpu;
    std::unique_ptr<i8080_word_t[]> mem;
    bool quit;
    int err;
    emu_opts opts;

    // testing
    bool rcrd_con;
    std::string conout;
}
EMU;

#define wordconcat(w1, w2) (((i8080_dword_t)(w1) << 8) | (w2))

void emu_vprinterr(const char* format, std::va_list vlist) noexcept
{
    std::fputs("\n\033[1;31mi8080emu: error: ", stderr);
    std::vfprintf(stderr, format, vlist);
    std::fputs("\033[0m\n", stderr);
}

static i8080_word_t intr_read(const i8080*) noexcept { return i8080_NOP; }

static i8080_word_t mem_read(const i8080*, i8080_addr_t addr) noexcept
{
    return EMU.mem[addr];
}

static void mem_write(const i8080*, i8080_addr_t addr, i8080_word_t word) noexcept
{
    EMU.mem[addr] = word;
}

static void io_write(const i8080*, i8080_word_t port, i8080_word_t word) noexcept
{
    emu_printerr("Unhandled I/O write to "
        "port %d w/ data: 0x%02x", port, word);
    EMU.quit = true;
    EMU.err = EMU_EHNDLR;
}

static i8080_word_t io_read(const i8080* cpu, i8080_word_t port) noexcept
{
    emu_printerr("Unhandled I/O read from "
        "port %d, clobbered acc: 0x%02x", port, cpu->a);
    EMU.quit = true;
    EMU.err = EMU_EHNDLR;
    return 0;
}

static void emu_putchar(i8080_word_t c) noexcept
{
    std::putchar(c);

    if (EMU.rcrd_con) {
        // try-catch, just in case
        try {
            EMU.conout.push_back(char(c));
        }
        catch (...) {
            emu_printerr("Out of memory");
            EMU.quit = true;
            EMU.err = EMU_EEXC;
        }
    }
}

static bool emu_cpm80_call(const i8080* cpu, i8080_addr_t addr) noexcept
{
    switch (addr)
    {   
    case 0x0000: // WBOOT
        EMU.quit = true;
        EMU.err = 0;
        break;
        
    case 0x0005: // BDOS
    {
        int callno = (int)cpu->c;
        switch (callno)
        {          
        case 2: // print char
            emu_putchar(cpu->e);
            break;
    
        case 9: // print $-terminated string
        {
            i8080_word_t c;
            i8080_addr_t ptr = wordconcat(cpu->d, cpu->e);
            while ((c = EMU.mem[ptr++]) != '$')
                emu_putchar(c);
            break;
        }
        default:
            emu_printerr("Unimplemented BDOS call %d", callno);
            EMU.quit = true;
            EMU.err = EMU_EBDOS;
            break;
        }
        break;
    }
    case 0x0038:
        emu_printerr("Program called debugger");
        EMU.quit = true;
        EMU.err = EMU_EDBGR;
        break;

    default:
        return false;
    }

    return true;
}

static void cpm80_io_write(const i8080* cpu, i8080_word_t port, i8080_word_t word) noexcept
{
    // offset PC by size of OUT instruction
    if (!emu_cpm80_call(cpu, cpu->pc - i8080_addr_t(2)))
        io_write(cpu, port, word);
}

// https://obsolescence.wixsite.com/obsolescence/cpm-internals  
static constexpr auto cpm80_lowsize = 0x100u;
// minimum possible size = CCP + BDOS
static constexpr auto cpm80_highsize = 0x1600u;

// Injected at operating system call locations.
static constexpr i8080_word_t emu_call[] = { i8080_OUT, 0xff, i8080_RET };


int emu_init(emu_opts opts)
{
    if (!EMU.mem)
    {
        EMU.mem.reset(new i8080_word_t[65536]); // 64K
        EMU.cpu.mem_read = mem_read;
        EMU.cpu.mem_write = mem_write;
        EMU.cpu.io_read = io_read;
        EMU.cpu.intr_read = intr_read;
    }

    if (opts.use_cpm_con)
    {
        // CP/M reserved RST 7 for debuggers like DDT!
        // can intercept this to detect if the CPU is executing garbage memory
        std::memset(EMU.mem.get(), i8080_RST_7, 65536 * sizeof(i8080_word_t));
        std::memcpy(&EMU.mem[0x0038], emu_call, sizeof(emu_call));

        std::memcpy(&EMU.mem[0x0000], emu_call, sizeof(emu_call)); // WBOOT
        std::memcpy(&EMU.mem[0x0005], emu_call, sizeof(emu_call)); // BDOS

        EMU.cpu.io_write = cpm80_io_write;
    }
    else EMU.cpu.io_write = io_write;

    EMU.quit = false;
    EMU.err = 0;
    EMU.opts = opts;
    // testing
    EMU.rcrd_con = false;
    EMU.conout.clear();    
    return 0;
}

void emu_destroy(void)
{
    EMU.mem.reset();
    std::string().swap(EMU.conout);
}

int emu_load(const char* filepath)
{
    std::FILE* fs = std::fopen(filepath, "rb");
    if (!fs) {
        emu_printerr("%s could not be opened.", filepath);
        return EMU_EFILE;
    }

    auto max_binsize = 65536u; // 64K
    i8080_word_t* binp = EMU.mem.get();
    if (EMU.opts.use_cpm_con)
    {
        max_binsize -= (cpm80_lowsize + cpm80_highsize);
        binp += cpm80_lowsize;
    }

    constexpr bool word_is_uchar = sizeof(i8080_word_t) == 1; // VS C4127
    if (word_is_uchar)
    {
        auto n = std::fread(binp, 1, max_binsize, fs);
        (void)n; // Werror=unused-result
    }
    else { 
        decltype(max_binsize) i = 0; int c;
        while ((c = std::fgetc(fs)) != EOF && i < max_binsize)
            binp[i++] = i8080_word_t(c);
    }
    if (std::ferror(fs))
    {
        std::fclose(fs);
        emu_printerr("%s could not be read.", filepath);
        return EMU_EFILE;
    }
    else if (!std::feof(fs))
    {
        std::fgetc(fs); // set eof
        if (!std::feof(fs))
        {
            std::fclose(fs);
            emu_printerr("%s is too large.", filepath);
            return EMU_EFILE;
        }
    }
    std::fclose(fs);

    return 0;
}

template <std::size_t Bufsz, typename T>
static inline void addfmt(std::string& str, char (&buf)[Bufsz], const char* format, T value)
{
    std::snprintf(buf, Bufsz, format, value);
    str += buf;
}

static std::string i8080_dbginfo(const i8080* cpu)
{
    char buf[32];
    std::string ret;
    addfmt(ret, buf, "a=0x%02x\n", cpu->a);
    addfmt(ret, buf, "b=0x%02x\n", cpu->b);
    addfmt(ret, buf, "c=0x%02x\n", cpu->c);
    addfmt(ret, buf, "d=0x%02x\n", cpu->d);
    addfmt(ret, buf, "e=0x%02x\n", cpu->e);
    addfmt(ret, buf, "h=0x%02x\n", cpu->h);
    addfmt(ret, buf, "l=0x%02x\n", cpu->l);
    addfmt(ret, buf, "bc=0x%04x\n", wordconcat(cpu->b, cpu->c));
    addfmt(ret, buf, "de=0x%04x\n", wordconcat(cpu->d, cpu->e));
    addfmt(ret, buf, "hl=0x%04x\n", wordconcat(cpu->h, cpu->l));
    addfmt(ret, buf, "sp=0x%04x\n", cpu->sp);
    addfmt(ret, buf, "pc=0x%04x\n", cpu->pc);
    addfmt(ret, buf, "sign=%u\n", cpu->s);
    addfmt(ret, buf, "zero=%u\n", cpu->z);
    addfmt(ret, buf, "carry=%u\n", cpu->cy);
    addfmt(ret, buf, "aux-carry=%u\n", cpu->ac);
    addfmt(ret, buf, "parity=%u\n", cpu->p);
    addfmt(ret, buf, "inte=%u\n", cpu->inte);
    addfmt(ret, buf, "intr=%u\n", cpu->intr);
    addfmt(ret, buf, "halt=%u\n", cpu->halt);
    addfmt(ret, buf, "cycles=%llu", cpu->cycles);   
    return ret;
}

const char* emu_errname(int err) noexcept
{
    switch (err)
    {
    case EMU_EKEYINTR: return "EMU_EKEYINTR";
    case EMU_EFILE: return "EMU_EFILE";
    case EMU_EEXC: return "EMU_EEXC";
    case EMU_EHNDLR: return "EMU_EHNDLR";
    case EMU_EOPCODE: return "EMU_EOPCODE";
    case EMU_EBDOS: return "EMU_EBDOS";
    case EMU_EDBGR: return "EMU_EDBGR";
    default: return "";
    }
}

static int check_err(int i80err)
{
    int err = EMU.quit ? EMU.err : i80err;
    if (err)
    {
        emu_printerr("%s", emu_errname(err));
        std::fputs("\033[1;33mCPU status:\n", stderr);
        std::fputs(i8080_dbginfo(&EMU.cpu).c_str(), stderr);
        std::fputs("\033[0m\n", stderr);
    }
    return err;
}

static int emu_do_run_with_intr(void)
{
    if (!keyintr_initlzd && !keyintr_init())
        return EMU_EKEYINTR;

    if (!keyintr_start())
        return EMU_EKEYINTR;

    int i80err = 0;
    while (!EMU.quit)
    {
        i80err = i8080_next(&EMU.cpu);
        if (i80err) break;

        if (EMU.cpu.halt)
        {
            if (!keyintr_wait())
            {
                keyintr_end();
                return EMU_EKEYINTR;
            }
            i8080_interrupt(&EMU.cpu);
        }
    }
    keyintr_end();
    return check_err(i80err);
}

static int emu_do_run(void)
{
    int i80err = 0;
    while (!EMU.quit)
    {
        i80err = i8080_next(&EMU.cpu);
        if (i80err) break;
    }
    return check_err(i80err);
}

int emu_run(void)
{
    i8080_reset(&EMU.cpu);

    // start at load location
    if (EMU.opts.use_cpm_con)
        EMU.cpu.pc = cpm80_lowsize;

    if (EMU.opts.conv_key_intr)
        return emu_do_run_with_intr();
    else
        return emu_do_run();
}