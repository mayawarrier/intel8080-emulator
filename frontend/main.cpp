
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <array>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <run.hpp>
#include <util.hpp>

using namespace run8080;

#define EXE_NAME "i8080emu"

static bool on_run_tests(void)
{
    bool success = true;
    //success &= test::run("bin/CPUTEST.COM", run_params(true, true, false));
    //success &= test::run("bin/TST8080.COM", test::run_params(true, true, false));
    //success &= test::run("bin/INTERRUPT.COM", test::run_params(true, true, true));
    return success;
}

int main(int argc, char** argv)
{
    const char NO_INPUT_FILES_MSG[] = "";

    if (!argv || argc < 2)
    {
        
        return 0;
    }

    const char HELP_MSG[] = "\n"
        "Emulate an INTEL 8080 microprocessor.\n\n"
        "Usage: " EXE_NAME " [options] [file]\n"
        "Options:\n"
        "   --help\t\t\tDisplay this message.\n"
        "   --cpm\t\t\tTreat file as a CP/M 2.2 binary.\n"
        "   --cpm-console\t\t\tEmulate a CP/M 2.2 console (WIP).\n"
        "   --intercept-sigint\t\t\tConvert SIGINTs into 8080 interrupts.\n"
        "   --run-tests\t\t\tRun all tests.\n";

    bool is_cpm80_binary = false;
    bool use_cpm80_console = false;
    bool intercept_sigint = false;
    bool require_file = false;

    const std::array<util::opt_info, 5> opts = {
        util::opt_info("--help", "-h", util::make_opt_callback(
            [&] { std::cout << HELP_MSG; })),
        util::opt_info("--cpm", nullptr, util::make_opt_callback(
            [&] { is_cpm80_binary = true; require_file = true; })),
        util::opt_info("--cpm-console", nullptr, util::make_opt_callback(
            [&] { use_cpm80_console = true; require_file = true; })),
        util::opt_info("--intercept-sigint", nullptr, util::make_opt_callback(
            [&] { intercept_sigint = true; require_file = true; })),
        util::opt_info("--tests", "-t", util::make_opt_callback(on_run_tests)),
    };

    util::do_opts_info do_opts_outinfo;
    bool success = util::do_opts(argc, argv, opts.data(), (int)opts.size(), nullptr, do_opts_outinfo);

    if (require_file)
    {
        int fname_index = do_opts_outinfo.last_extracted_arg_index + 1;
        auto& rem_arg_indices = do_opts_outinfo.remaining_arg_indices;
        // binary search
        auto fname_itr = std::lower_bound(rem_arg_indices.begin(), rem_arg_indices.end(), fname_index);
        bool fname_index_found = fname_itr != rem_arg_indices.end() && *fname_itr == fname_index;

        if (fname_index_found) {
            char* filename = argv[fname_index];

            //test::run_params tparams(is_cpm80_binary, use_cpm80_console, intercept_sigint);


        }

    }

    

    

    //if (!fname_index_found) {
    //
    //}
    //
    //if (fname_index_found)
    //{
    //    
    //    rem_arg_indices.erase(fname_itr);
    //}
    //
    //
    //if (rem_arg_indices.size() > 0) {
    //    //for
    //    std::cout << EXE_NAME " error: Unrecognized argument " << argv[unreco]
    //}

    

    



    return 0;



    //const char* arg;
    //if (argc > 1 && argv && (arg = argv[1]))
    //{
    //    run_params tparms; // default ok
    //    success = run(arg, tparms);
    //} else {
    //    // run all tests in bin/
    //    
    //}
    //
    //if (success) return EXIT_SUCCESS;
    //else return EXIT_FAILURE;
}
