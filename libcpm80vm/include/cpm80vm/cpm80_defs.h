
#ifndef CPM80_DEFS_H
#define CPM80_DEFS_H

#include "libi8080/include/i8080.h"

/* Allow C++ compilers to link to C headers */
#ifdef __cplusplus
    #define CPM80VM_CDECL extern "C"
#else
    #define CPM80VM_CDECL
#endif

CPM80VM_CDECL typedef i8080_addr_t cpm80_addr_t;
CPM80VM_CDECL struct cpm80_vm;

#endif /* CPM80_DEFS_H */