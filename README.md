# intel8080-emulator

An Intel 8080 microprocessor emulator. Features:
- Accurate and complete (see test results below)
- Supports async 8080 interrupts (see tests/bin/INTERRUPT.COM)
- Portable:
    - libi8080 is C89-compatible and freestanding.
    - Testing tool is written in C++11 (GCC >= 4.9, clang >= 3.1, MSVC >= VS 2015 should work)
    - Supports CMake versions as low as 3.1.

### Targets
- libi8080: base library that can be used standalone to emulate an 8080.
- i8080emu: command line tool to run tests or simple CP/M-80 binaries.

## Building
Install [CMake](https://cmake.org/). 
Open terminal (or Windows Powershell), cd to the source directory and run:
```
mkdir build
cd build
cmake ..
cmake --build .
```

## Running
./i8080emu --help 
```
Emulate an Intel 8080.
Usage:
  i8080emu [OPTION...]

  -h, --help         Show usage.
      --con          Emulate CP/M-80 console. Program will be loaded at
                     0x100. (default: true)
      --kintr        Convert Ctrl+C interrupts to 8080 interrupts.
  -f, --file <file>  Input file.

 Test options:
  -t, --tests          Run tests.
      --testdir <dir>  Look for test files in this directory. (default:
                       testbin)

```
./i8080emu --kintr -f testbin/INTERRUPT.COM \
This test requires user input, so it does not run from --tests by default.
```
Waiting for interrupt...
^CReceived, exit to DOS.
```
./i8080emu --tests 
```
********** Running test 1/4: TST8080.COM
MICROCOSM ASSOCIATES 8080/8085 CPU DIAGNOSTIC
 VERSION 1.0  (C) 1980

 CPU IS OPERATIONAL
**********

********** Running test 2/4: CPUTEST.COM

DIAGNOSTICS II V1.2 - CPU TEST
COPYRIGHT (C) 1981 - SUPERSOFT ASSOCIATES

ABCDEFGHIJKLMNOPQRSTUVWXYZ
CPU IS 8080/8085
BEGIN TIMING TEST
END TIMING TEST
CPU TESTS OK

**********

********** Running test 3/4: 8080PRE.COM
8080 Preliminary tests complete
**********

********** Running test 4/4: 8080EXM.COM
8080 instruction exerciser
dad <b,d,h,sp>................  PASS! crc is:14474ba6
aluop nn......................  PASS! crc is:9e922f9e
aluop <b,c,d,e,h,l,m,a>.......  PASS! crc is:cf762c86
<daa,cma,stc,cmc>.............  PASS! crc is:bb3f030c
<inr,dcr> a...................  PASS! crc is:adb6460e
<inr,dcr> b...................  PASS! crc is:83ed1345
<inx,dcx> b...................  PASS! crc is:f79287cd
<inr,dcr> c...................  PASS! crc is:e5f6721b
<inr,dcr> d...................  PASS! crc is:15b5579a
<inx,dcx> d...................  PASS! crc is:7f4e2501
<inr,dcr> e...................  PASS! crc is:cf2ab396
<inr,dcr> h...................  PASS! crc is:12b2952c
<inx,dcx> h...................  PASS! crc is:9f2b23c0
<inr,dcr> l...................  PASS! crc is:ff57d356
<inr,dcr> m...................  PASS! crc is:92e963bd
<inx,dcx> sp..................  PASS! crc is:d5702fab
lhld nnnn.....................  PASS! crc is:a9c3d5cb
shld nnnn.....................  PASS! crc is:e8864f26
lxi <b,d,h,sp>,nnnn...........  PASS! crc is:fcf46e12
ldax <b,d>....................  PASS! crc is:2b821d5f
mvi <b,c,d,e,h,l,m,a>,nn......  PASS! crc is:eaa72044
mov <bcdehla>,<bcdehla>.......  PASS! crc is:10b58cee
sta nnnn / lda nnnn...........  PASS! crc is:ed57af72
<rlc,rrc,ral,rar>.............  PASS! crc is:e0d89235
stax <b,d>....................  PASS! crc is:2b0471e9
Tests complete
**********
```


## Helpful resources:
 * [8080 Programming manual](https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf)
 * [RadioShack's 8080 manual](https://archive.org/details/8080-8085_Assembly_Language_Programming_1977_Intel)
 * [8080 Opcode table](http://pastraiser.com/cpu/i8080/i8080_opcodes.html)
 * [Detecting OS types using compiler macros](https://web.archive.org/web/20191012035921/http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system)
* [Overview of the CP/M BIOS](https://www.seasip.info/Cpm/bios.html)
