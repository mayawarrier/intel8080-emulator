
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <string>
#include <memory>
#include <cassert>
#include <iostream>

#include "cxxopts/cxxopts.hpp"
#include "i8080/i8080_opcodes.h"
#include "emu.hpp"





//static emu_test parse_testfile(std::ifstream& f)
//{
//    emu_test ret;
//    // todo
//    return ret;
//}

inline int bail(const char* format, ...) noexcept
{
    std::va_list args;
    va_start(args, format);
    emu_vprinterr(format, args);
    va_end(args);
    return EXIT_FAILURE;
}

int main(int argc, char** argv)
{
    cxxopts::Options parser("i8080emu", "Emulate an Intel 8080 microprocessor.");
    parser.add_options()
        ("h,help", "Show usage.")
        ("c,cpmcon", "Emulate CP/M-80 console.")
        ("i,kintr", "Convert keyboard interrupts into 8080 interrupts.")
        ("t,test", "One or more tests to run and verify.");

    auto res = parser.parse(argc, argv);
    if (res["help"].as<bool>()) 
    {
        std::cout << parser.help() << std::endl;
        return EXIT_SUCCESS;
    }

    //auto& files = res.unmatched();
    //if (res["test"].as<bool>()) {
    //
    //    for (auto& file : files)
    //    {
    //        std::ifstream f(file);
    //        if (!f) 
    //            return bail(file + " could not be opened.");
    //
    //        
    //    }
    //    
    //}

    for (auto& file : res.unmatched())
    {
        //cpm80vm vm(64, nullptr, nullptr,
        //    [](cpm80vm*) noexcept -> i8080_word_t { return i8080_NOP; });
        //
        //const unsigned max_prog_size = vm.max_program_addr() + 1 - 256;
        //
        //auto* fs = std::fopen(file.c_str(), "rb");
        //if (!fs) 
        //    return bail(file + " could not be opened.");
        //
        //std::fread(vm.memory() + 256, 1, max_prog_size + 1, fs);
        //if (std::ferror(fs) || !std::feof(fs))
        //{
        //    std::fclose(fs);
        //    return bail(file + " could not be read or is too large.");
        //}
        //std::fclose(fs);
        //
        //cpm80vm_exitcode exitcode;
        //exitcode = vm.restart();
        //
        //if (!(exitcode == VM_ProgramExit || exitcode == VM_Success))
        //{
        //    std::cerr << file << " failed.";
        //    return EXIT_FAILURE;
        //}

        emu_opts opts;
        opts.conv_key_intr = true;
        opts.use_cpm_con = true;

        if (emu_init(opts) != 0)
            return bail("fail init");

        if (emu_load(file.c_str()) != 0)
            return bail("fail load");

        if (emu_run() != 0)
            return bail("fail run");

        emu_destroy();
    }
}
