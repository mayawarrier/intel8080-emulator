
#ifndef KEYINTR_HPP
#define KEYINTR_HPP

extern bool keyintr_initlzd;
extern bool keyintr_sigsset;

// First-time init.
bool keyintr_init(void);

void keyintr_destroy(void);

// Start intercepting keyboard interrupts.
bool keyintr_setsigs(void);

// Stop intercepting keyboard interrupts.
void keyintr_resetsigs(void);

// Wait for a keyboard interrupt.
// Returns true on success.
bool keyintr_wait(void);

#endif