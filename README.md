


# intel8080-emulator

An emulator for the Intel 8080 microprocessor, written in C89/ANSI C. 
Supports asynchronous interrupts and emulates part of the CP/M 2.2 BIOS for 8080 binaries that target it.

I've tried my best to make this as portable and accurate as possible! I'd appreciate any feedback towards this goal.

### Portability:
- Written in C89/ANSI C, taking advantage of C99 features if available.
- Tested with **gcc** 6.3.0 on Debian 9, WebAssembly/**emcc** on Debian 10 through WSL with a WebAssembly-thread compatible browser (Chrome 60.0+, with the feature turned on), and with **MSVC** on Windows 8.
- Attempt made to support async interrupts on as many environments as possible **(see i8080_predef.h and i8080_sync.c)**. This is untested, but should work on Windows versions >= XP, POSIX environments >= 199506, and GNUC compilers >= version 4.7.0.
### Accuracy:
- Passes test ROMs TST8080.COM (Kelly Smith test) from Microcosm Associates 1980 and CPUTEST.COM from Supersoft Associates 1981.
- Passes test_interrupts.COM, which tests if async interrupts are synchronized and serviced correctly.
- More tests to be added!
### libi8080emu, libi8080
- libi8080 is the core emulation library and can be used standalone to emulate an i8080.
- libi8080emu wraps around libi8080 to provide debugging functionality and CP/M 2.2 BIOS emulation.

## Building and running tests:

**Dependencies:** git-lfs, cmake

**Setup:** Install git-lfs and run `git-lfs install && git-lfs pull` in the repo directory (or use the project's CMakeLists.txt to automatically install dependencies and perform setup).

**Steps to build only the libi8080 and libi8080emu libraries:**

Build only the C89-compliant libraries with:
```
cmake . -DCMAKE_BUILD_TYPE=Release -DLIBS_ONLY=ON
cmake --build .
```
**Steps to build with the command line frontend (written in C++11):**
```
cmake . -DCMAKE_BUILD_TYPE=Release -DFIRST_TIME_SETUP=OFF
cmake --build .
```
**DFIRST_TIME_SETUP=ON** 
- Will try to auto-install git-lfs if it is unavailable (this requires admin privileges).
- On Windows, this will first install and configure the Chocolatey package manager, then install git-lfs through Chocolatey.

### Command line tool:
Frontend to libi8080emu, to run tests or other ROMs from the command line.
```
i8080-emu, an emulator for the INTEL 8080 microprocessor with some CP/M 2.2 BIOS support.
Supports async interrupts, CPM 2.2 BDOS ops 2 and 9, and a simple command processor at CP/M WBOOT.

Usage: i8080-emu [options]
Options:
   -h   --help                  Print this help message.
   -e   --env ENV               Set the environment. "default" or "cpm".
   -f   --file FILE             Execute the file as i8080 binary.
   --run-all-tests              Run all the test files under tests/.
```
--run-all-tests:
```

-------------------------- Test 1: TST8080.COM --------------------------
                8080/8085 CPU Diagnostic, Kelly Smith, 1980
-------------------------------------------------------------------------
MICROCOSM ASSOCIATES 8080/8085 CPU DIAGNOSTIC
 VERSION 1.0  (C) 1980

 CPU IS OPERATIONAL
-------------------------------------------------------------------------

-------------------------- Test 2: CPUTEST.COM --------------------------
        Supersoft Associates CPU test, Diagnostics II suite, 1981
-------------------------------------------------------------------------

DIAGNOSTICS II V1.2 - CPU TEST
COPYRIGHT (C) 1981 - SUPERSOFT ASSOCIATES

ABCDEFGHIJKLMNOPQRSTUVWXYZ
CPU IS 8080/8085
BEGIN TIMING TEST
END TIMING TEST
CPU TESTS OK

-------------------------------------------------------------------------

---------------------- Test 3: test_interrupts.COM ----------------------
                Prints '+' in a loop for 0.2s, then returns to
                WBOOT upon receiving interrupt.
-------------------------------------------------------------------------
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
-------------------------------------------------------------------------

3/3 tests passed.
```
## Helpful resources:
- [superzazu's 8080 emulator](https://github.com/superzazu/8080)
- [Detecting OS types using compiler macros](http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system)
- [Overview of the CP/M BIOS](https://www.seasip.info/Cpm/bios.html)
