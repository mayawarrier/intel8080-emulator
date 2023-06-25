
#ifndef KEYINTR_HPP
#define KEYINTR_HPP

extern bool keyintr_initlzd;

// First-time init.
bool keyintr_init(void);

// Start listening for keyboard interrupts.
bool keyintr_start(void);

// Stop listening for keyboard interrupts.
void keyintr_end(void);

// Wait for a keyboard interrupt.
// Returns true on success.
bool keyintr_wait(void);

#endif