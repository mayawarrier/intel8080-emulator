
#ifndef FRONTEND_TEST_H
#define FRONTEND_TEST_H

#include <cstddef>
#include <string>
#include "i8080_types.h"

struct i8080;

/* 65,536 bytes aka 64KB */
constexpr auto VM_64K = 0x10000;
extern i8080_word_t VM_MEMORY[VM_64K];


int libi8080_default_tests();

int libi8080_user_test(std::string& filepath);

int libcpm80vm_default_tests();

int libcpm80vm_user_test(const char filepath[]);


// Prompts user for [yn] response and returns result.
bool yesno_prompt(const char prompt[]);

// Dump the value of all registers, flags, cycles and
// (optionally) the contents of memory into memdump.bin.
void i8080_dump_stats(const struct i8080& i8080);

// Load into memory the contents at filepath. Prints error messages.
// Returns 0 if successful, else -1.
int load_memory(const char filepath[], i8080_word_t* memory, std::size_t maxlen);

#endif
