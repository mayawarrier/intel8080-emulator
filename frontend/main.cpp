
#include <cstdlib>
#include <iostream>
#include <test.hpp>

int main(int argc, char** argv)
{
    bool success = true;
    const char* arg;
    if (argc > 1 && argv && (arg = argv[1]))
    {
        testi8080::test_params tparms; // default ok
        success = testi8080::run_test(arg, tparms);
    } else {
        // run all tests in bin/
        success &= testi8080::run_test("bin/CPUTEST.COM", testi8080::test_params(true, false));
        success &= testi8080::run_test("bin/TST8080.COM", testi8080::test_params(true, false));
        success &= testi8080::run_test("bin/INTERRUPT.COM", testi8080::test_params(true, true));
    }

    if (success) return EXIT_SUCCESS;
    else return EXIT_FAILURE;
}
