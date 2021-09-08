
#ifndef I8080_EXTS_H
#define I8080_EXTS_H

#include <cstdio>
#include <cstddef>

struct i8080;

namespace exts
{
// Print the values of all registers and flags.
void i8080_dump(const struct i8080* i8080, std::FILE* stream) noexcept;

// Dump all bytes of memory. Returns true on success.
bool i8080_dump_memory(const struct i8080* i8080, std::FILE* stream,
	std::size_t memsize, const char* word_format = "0x%02x, ") noexcept;

}

#endif
