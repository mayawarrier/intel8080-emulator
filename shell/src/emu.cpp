
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <climits>
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
            emu_printerr("Unknown BDOS call %d", callno);
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
static constexpr std::size_t cpm80_lowsize = 0x100;
// minimum possible size = CCP + BDOS
static constexpr std::size_t cpm80_highsize = 0x1600;

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
        // we can intercept this to detect if the CPU is executing garbage memory
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

    if (opts.conv_key_intr && !keyintr_sigsset)
    {
        if (!keyintr_initlzd)
        {
            if (!keyintr_init())
                return EMU_EKEYINTR;
        }
        if (!keyintr_setsigs())
            return EMU_EKEYINTR;
    }
    else if (!opts.conv_key_intr && keyintr_sigsset)
        keyintr_resetsigs();
        
    return 0;
}

void emu_destroy(void)
{
    keyintr_destroy();
    EMU.mem.reset();
    std::string().swap(EMU.conout);
}

int emu_load(const char* binfile)
{
    auto* fs = std::fopen(binfile, "rb");
    if (!fs) {
        emu_printerr("%s could not be opened.", binfile);
        return EMU_EFILE;
    }

    unsigned long max_binsize = 65536; // 64K
    i8080_word_t* binp = EMU.mem.get();
    if (EMU.opts.use_cpm_con)
    {
        max_binsize -= (cpm80_lowsize + cpm80_highsize);
        binp += cpm80_lowsize;
    }

    if (sizeof(i8080_word_t) == 1)
    {
        auto n = std::fread(binp, 1, max_binsize, fs);
        (void)n; // Werror=unused-result
    }
    else {
        i8080_word_t* p = binp; int c;
        while ((c = std::fgetc(fs)) != EOF)
            *p++ = i8080_word_t(c);
    }
    if (std::ferror(fs))
    {
        std::fclose(fs);
        emu_printerr("%s could not be read.", binfile);
        return EMU_EFILE;
    }
    else if (!std::feof(fs))
    {
        std::fgetc(fs); // set eof
        if (!std::feof(fs))
        {
            std::fclose(fs);
            emu_printerr("%s is too large.", binfile);
            return EMU_EFILE;
        }
    }
    std::fclose(fs);

    return 0;
}

template <std::size_t Bufsz = 64, typename T>
static inline void addfmtstr(std::string& str, const char* format, T value)
{
    char buf[Bufsz];
    std::snprintf(buf, Bufsz, format, value);
    str += buf;
}

static std::string i8080_dbginfo(const i8080* cpu)
{
    std::string ret;
    addfmtstr(ret, "a=0x%02x\n", cpu->a);
    addfmtstr(ret, "b=0x%02x\n", cpu->b);
    addfmtstr(ret, "c=0x%02x\n", cpu->c);
    addfmtstr(ret, "d=0x%02x\n", cpu->d);
    addfmtstr(ret, "e=0x%02x\n", cpu->e);
    addfmtstr(ret, "h=0x%02x\n", cpu->h);
    addfmtstr(ret, "l=0x%02x\n", cpu->l);
    addfmtstr(ret, "bc=0x%04x\n", wordconcat(cpu->b, cpu->c));
    addfmtstr(ret, "de=0x%04x\n", wordconcat(cpu->d, cpu->e));
    addfmtstr(ret, "hl=0x%04x\n", wordconcat(cpu->h, cpu->l));
    addfmtstr(ret, "sp=0x%04x\n", cpu->sp);
    addfmtstr(ret, "pc=0x%04x\n", cpu->pc);
    addfmtstr(ret, "sign=%u\n", cpu->s);
    addfmtstr(ret, "zero=%u\n", cpu->z);
    addfmtstr(ret, "carry=%u\n", cpu->cy);
    addfmtstr(ret, "aux-carry=%u\n", cpu->ac);
    addfmtstr(ret, "parity=%u\n", cpu->p);
    addfmtstr(ret, "inte=%u\n", cpu->inte);
    addfmtstr(ret, "intr=%u\n", cpu->intr);
    addfmtstr(ret, "halt=%u\n", cpu->halt);
    addfmtstr(ret, "cycles=%llu", cpu->cycles);   
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
    int i80err = 0;
    while (!EMU.quit)
    {
        i80err = i8080_next(&EMU.cpu);
        if (i80err) break;

        if (EMU.cpu.halt)
        {
            if (!keyintr_wait())
                return EMU_EKEYINTR;

            i8080_interrupt(&EMU.cpu);
        }
    }
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