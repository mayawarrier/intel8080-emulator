/* 
 * File:   utilities.h
 * Author: ellie
 *
 * Created on June 23, 2019, 9:01 PM
 */

#ifndef UTILITIES_H
#define UTILITIES_H

// Gets a string safely from user input, loads it into a buffer,
// and returns its length.
int get_string_safely(char * buffer, int max_length) {
    int i = 0;
    char c;
    while (i < max_length && (c = getchar()) != '\n') {
        buffer[i++] = c;
    }
    buffer[i] = '\0';
    return i;
}

#endif /* UTILITIES_H */

