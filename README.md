
# intel8080-emulator

An emulator for the Intel 8080 microprocessor, written in C89/ANSI C. 
Supports asynchronous interrupts and emulates part of the CP/M 2.2 BIOS for 8080 binaries that target it.

I've tried my best to make this as portable and accurate as possible! I'd appreciate any feedback towards this goal.

### Portability:
- Written in C89/ANSI C, taking advantage of C99 features if available.
- Tested with gcc 6.3.0 -std=c89 and WebAssembly/emcc on Debian 9, and with MSVC on Windows 8.
- Attempt made to support async interrupts on as many environments as possible **(see i8080_predef.h and i8080_sync.c)**. This is untested, but should work on Windows versions >= XP, POSIX environments >= 199506, and GNUC compilers >= version 4.7.0.
### Accuracy:
- Passes test roms TST8080.COM and CPUTEST.COM from Microcosm Associates (Kelly Smith test) and Supersoft Associates 1980-1981.
- More tests to be added!
### libi8080emu, libi8080
This project consists of two libraries: libi8080emu, and libi8080emu/libi8080. 
- libi8080 is the core emulation library and can be used standalone to emulate an i8080.
- libi8080emu wraps around libi8080 to provide debugging functionality and CP/M 2.2 BIOS emulation.

## Building and running tests:
This depends on git-lfs to pull the test files in libi8080emu/tests. It can be auto-installed from this project's CMakeLists.

To build on Linux, install CMake and run this in the repo directory:
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFORCE_INSTALL_DEP=OFF && cmake --build build --target all
```
For Windows, install CMake through Visual Studio 2017/2019 with the Linux Development option checked. Then run this in the repo directory with PowerShell as administrator:
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFORCE_INSTALL_DEP=ON; cmake --build build
```
**DFORCE_INSTALL_DEP=ON** will try to auto-install git-lfs if it is unavailable.
- On Linux this is installed through apt-get.
- On Windows, this will first install and configure the Chocolatey package manager, then install git-lfs through Chocolatey.

If you'd rather not install git-lfs, you can download and replace the libi8080emu/tests folder instead, and run cmake with **DFORCE_INSTALL_DEP=OFF**.

This will create the build/i8080emu executable, which can be called from the command line.

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
