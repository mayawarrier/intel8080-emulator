/* 
 * File:   i8080_build.h
 * Author: dhruvwarrier
 * 
 * Enable build on non-POSIX compliant compilers.
 * 
 * If building on Windows, include this before including <stdio.h> in every file. 
 *
 * Created on June 30, 2019, 2:56 PM
 */

#ifndef I8080_BUILD_H
#define I8080_BUILD_H

// Suppress deprecation errors with scanf, printf, etc
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#endif

#endif /* I8080_BUILD_H */