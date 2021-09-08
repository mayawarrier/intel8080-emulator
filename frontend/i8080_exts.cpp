
#include <i8080_exts.hpp>
#include "i8080.h"

#define concat(w1, w2) (((w1) << 8) | (w2))

namespace exts
{
void i8080_dump(const struct i8080* i8080, std::FILE* out) noexcept
{
	std::fprintf(out, "\nProgram counter: 0x%04x", i8080->pc);
	std::fprintf(out, "\nStack pointer: 0x%04x", i8080->sp);

	std::fprintf(out, "\n\n--- Registers:");
	std::fprintf(out, "\na: 0x%02x", i8080->a);
	std::fprintf(out, "\nb: 0x%02x", i8080->b);
	std::fprintf(out, "\nc: 0x%02x", i8080->c);
	std::fprintf(out, "\nd: 0x%02x", i8080->d);
	std::fprintf(out, "\ne: 0x%02x", i8080->e);
	std::fprintf(out, "\nh: 0x%02x", i8080->h);
	std::fprintf(out, "\nl: 0x%02x", i8080->l);

	std::fprintf(out, "\n\n--- Double registers:");
	std::fprintf(out, "\nbc: 0x%04x", concat(i8080->b, i8080->c));
	std::fprintf(out, "\nde: 0x%04x", concat(i8080->d, i8080->e));
	std::fprintf(out, "\nhl: 0x%04x", concat(i8080->h, i8080->l));

	std::fprintf(out, "\n\n--- Flags:");
	std::fprintf(out, "\nSign: %d", i8080->s);
	if (i8080->s == 1) std::fprintf(out, " (negative)");
	else if (i8080->s == 0) std::fprintf(out, " (positive)");
	
	std::fprintf(out, "\nZero: %d", i8080->z);
	std::fprintf(out, "\nCarry: %d", i8080->cy);
	std::fprintf(out, "\nAux carry: %d", i8080->ac);
	
	std::fprintf(out, "\nParity: %d", i8080->p);
	if (i8080->p == 1) std::fprintf(out, " (even)");
	else if (i8080->p == 0) std::fprintf(out, " (odd)");

	std::fprintf(out, "\n");
	std::fprintf(out, "\nHalt: %d", i8080->halt);
	std::fprintf(out, "\nInterrupt enable: %d", i8080->inte);
	std::fprintf(out, "\nInterrupt received: %d", i8080->intr);
	std::fprintf(out, "\nCycles: %lu", i8080->cycles);
}

bool i8080_dump_memory(const struct i8080* i8080, std::FILE* out,
	std::size_t memsize, const char* word_format) noexcept
{
	for (auto i = 0; i < memsize; ++i)
	{
		int wrote = std::fprintf(out, word_format,
			i8080->memory_read(i8080, (i8080_addr_t)i));

		if (wrote < 0) return false;
	}
	return true;
}
}
