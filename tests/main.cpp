
#include <cstdlib>
#include <cstdarg>
#include <string>
#include <iostream>

#define CXXOPTS_NO_RTTI
#include "cxxopts.hpp"
#include "emu.hpp"

static const std::pair<const char*, emu_opts> TESTS[] = 
{
    {"TST8080.COM", emu_opts()},
    {"CPUTEST.COM", emu_opts()},
    {"8080PRE.COM", emu_opts()},
    {"8080EXM.COM", emu_opts()}
};
static constexpr auto NUM_TESTS = sizeof(TESTS) / sizeof(TESTS[0]);

static int bail(const char* format, ...) noexcept
{
    std::va_list args;
    va_start(args, format);
    emu_vprinterr(format, args);
    va_end(args);
    return EXIT_FAILURE;
}

static int run(const std::string& file, emu_opts opts)
{
    int e;
    if ((e = emu_init(opts)) != 0 ||
        (e = emu_load(file.c_str())) != 0 ||
        (e = emu_run()) != 0)
        return e;
    return 0;
}

int main(int argc, char** argv)
{
    try {
        cxxopts::Options opts("i8080emu", "Emulate an Intel 8080.");
        opts.add_options()
            ("h,help", "Show usage.")
            ("con", "Emulate CP/M-80 console. Program will be loaded at 0x100.",
                cxxopts::value<bool>()->default_value("true"))
            ("kintr", "Convert Ctrl+C interrupts to 8080 interrupts.")
            ("f,file", "Input file.", cxxopts::value<std::string>(), "<file>");
        opts.add_options("Test")
            ("t,tests", "Run tests.")
            ("testdir", "Look for test binaries in this directory.",
                cxxopts::value<std::string>()->default_value("testbin"), "<dir>");

        auto res = opts.parse(argc, argv);
        if (res["help"].as<bool>())
        {
            std::cout << opts.help() << std::endl;
            return EXIT_SUCCESS;
        }

        if (res["tests"].count() != 0)
        {
            auto& testdir = res["testdir"].as<std::string>();
            for (std::size_t i = 0; i < NUM_TESTS; ++i)
            {
                auto& test = TESTS[i];
                std::cout << "\033[1;33m********** Running test ";
                std::cout << i + 1 << "/" << NUM_TESTS << ": ";
                std::cout << test.first << "\033[0m" << std::endl;

                int e = run(testdir + "/" + test.first, test.second);
                if (e) emu_errexit(e);

                std::cout << "\n\033[1;33m**********\033[0m\n" << std::endl;
            }
            return EXIT_SUCCESS;
        }
        else {
            if (res["file"].count() == 0)
                return bail("No input file");

            auto& file = res["file"].as<std::string>();
            bool conv_key_intr = res["kintr"].as<bool>();
            bool use_cpm_con = res["con"].as<bool>();

            int e = run(file, emu_opts(conv_key_intr, use_cpm_con));
            if (e) emu_errexit(e);
            
            return EXIT_SUCCESS;
        }
    }
    catch (const cxxopts::exceptions::exception& e)
    {
        return bail("Bad options: %s", e.what());
    }
}
