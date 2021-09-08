
#ifndef LIBI8080_TEST_H
#define LIBI8080_TEST_H

#include <cstddef>
#include <memory>
#include <vector>
#include "i8080.h"

namespace testi8080
{
struct i8080_state
{
    i8080_word_t a, b, c, d, e, h, l;

    i8080_addr_t sp;
    i8080_addr_t pc;

    unsigned s : 1;  /* Sign flag */
    unsigned z : 1;  /* Zero flag */
    unsigned cy : 1; /* Carry flag */
    unsigned ac : 1; /* Aux carry flag */
    unsigned p : 1;  /* Parity flag */

    unsigned inte : 1; /* Interrupt enable */
    unsigned intr : 1; /* Interrupt pending */
    unsigned halt : 1; /* In WAIT state? */

    unsigned long cycles;

    std::shared_ptr<i8080_word_t[]> memory;
};

struct test_params
{
    // unimplemented
    // If true, SIGINTs send interrupts to the 8080.
    //bool enable_interrupts;

    // Emulate CP/M 2.2 OS + devices (WIP).
    // Required by some tests.
    bool emulate_cpm80;

    // Send a no-op interrupt after 5 seconds.
    bool test_interrupts;

    //std::shared_ptr<i8080_state> initial; // unimplemented
    //std::shared_ptr<i8080_state> expected_final; // unimplemented

    // Default params
    test_params() {
        test_interrupts = false;
        emulate_cpm80 = false;
    }

    test_params(bool emulate_cpm80, bool test_interrupts) :
        emulate_cpm80(emulate_cpm80), test_interrupts(test_interrupts)
    {}
};

// Load and run an 8080 binary.
// Returns true if test executed without error.
bool run_test(const char filepath[], const struct test_params& params);

}

#endif
