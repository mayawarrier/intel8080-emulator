/* 
 * libi8080emu constants.
 */

/* The starting location of the CP/M Transient Program Area i.e.
 * the first valid location to load a program written for CP/M.
 * Set this environment with emu_set_cpm_env(). */
#define CPM_START_OF_TPA (0x0100)

// Port address that CP/M will use to read/write characters from/to a console
#define CPM_CONSOLE_ADDR (0x00)

/* The first valid location to load a program with the default
 * environment. Set this environment with emu_set_default_env(). */
#define DEFAULT_START_PA (0x0040)