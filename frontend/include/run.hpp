
#ifndef RUN8080_HPP
#define RUN8080_HPP

#include <cstddef>
#include <memory>
#include <vector>
#include "i8080/i8080.h"

namespace run8080 {

struct i8080_state
{
    i8080_word_t a, b, c, d, e, h, l;
    i8080_addr_t sp, pc;
    // flags
    unsigned s : 1, z : 1, cy : 1, ac : 1, p : 1; 

    unsigned halt : 1;
    // interrupt enable/received
    unsigned inte : 1, intr : 1;

    unsigned long cycles;

    struct mem_map
    {
        struct entry {
            i8080_addr_t addr;
            i8080_word_t word;
        };
        std::vector<entry> entries;
    }
    memory_map;

    //static i8080_state 
};

struct run_params
{
    // Convert SIGINTs into 8080 interrupts.
    bool intercept_sigint;
    // Assume a CP/M 2.2 binary.
    bool is_cpm80_binary;
    // Emulate a CP/M 2.2 console (WIP).
    bool use_cpm80_console;

    // Processor/memory initial state. Optional.
    std::shared_ptr<i8080_state> initial_state;

    run_params(bool is_cpm80_binary, bool use_cpm80_console,
        bool intercept_sigint, std::shared_ptr<i8080_state> initial_state) :
        is_cpm80_binary(is_cpm80_binary), use_cpm80_console(use_cpm80_console),
        intercept_sigint(intercept_sigint), initial_state(initial_state)
    {}

    run_params(bool is_cpm80_binary, bool use_cpm80_console, bool intercept_sigint)
         : run_params(is_cpm80_binary, use_cpm80_console, intercept_sigint, nullptr)
    {}

    run_params() : run_params(false, false, false, nullptr)
    {}
};

struct expected_run_info
{};

struct test_params
{};

// Execute 8080 binary.
// Returns true on success.
bool run(const char filepath[], const run_params& params);

// Execute 8080 binary.
// Returns true on success.
bool expect(const char filepath[], const run_params& rparams, const test_params& tparams);

}
#endif
