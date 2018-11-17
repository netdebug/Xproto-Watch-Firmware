#ifndef _UTILS_H
#define _UTILS_H

#include <stdint.h>

void Randomize(void);               // Randomize random number generator
uint8_t prandom(void);              // Pseudo Random Number - Linear congruential generator
uint8_t qrandom(void);              // Quick Pseudo Random Number
uint8_t half(uint8_t number);       // Converts an uint to int, then returns the half
uint8_t twice(uint8_t number);      // Converts an uint to int, then return the double, or saturates
char NibbleToChar(uint8_t nibble);  // Converts a nibble to the corresponding ASCII representing the HEX value
int8_t Sine60(int8_t angle, int8_t scale);        // Scaled Sine: 60 values -> 2*PI
int8_t Cosine60(int8_t angle, int8_t scale);      // Scaled Cosine: 60 values -> 2*PI

#endif