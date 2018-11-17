/*****************************************************************************

Oscilloscope Watch

Gabotronics
October 2018

Copyright 2018 Gabriel Anzziani

This program is distributed under the terms of the GNU General Public License

ATXMEGA256A3U

Compiled with GCC, -Os optimizations

www.gabotronics.com
email me at: gabriel@gabotronics.com

*****************************************************************************/

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "utils.h"
#include "mygccdef.h"

union {
    uint32_t Seed32;
    uint16_t Seed16;
} Seed;    

// Use time to randomiza
void Randomize(void) {
    Seed.Seed16 = TCF0.CNT;
}

// Pseudo Random Number - Linear congruential generator
uint8_t prandom(void) {
    Seed.Seed32 = 25173 * Seed.Seed32 + 13849;
    return Seed.Seed32>>24;
}

// Quick Pseudo Random Number - Linear congruential generator
uint8_t qrandom(void) {
    Seed.Seed16 = 25173 * Seed.Seed16 + 1;
    return Seed.Seed16>>8;
}

// Converts an uint to int, then returns the half
uint8_t half(uint8_t number) {
    int8_t temp;
    temp=(int8_t)(number-128);
    temp=temp/2;
    return (uint8_t)(temp+128);
}

// Converts an uint to int, then return the double, or saturates
uint8_t twice(uint8_t number) {
    int8_t temp;
    if(number<=64) return 0;
    if(number>=192) return 255;
    temp = (int8_t)(number-128);
    temp = temp*2;
    return (uint8_t)(temp+128);
}

// Receives a nibble, returns the corresponding ascii that represents the HEX value
char NibbleToChar(uint8_t nibble) {
    nibble=nibble+'0';          // '0' thru '9'
    if(nibble>'9') nibble+=7;   // 'A' thru 'F'
    return nibble;
}

// 60 value sine table
const int8_t SIN60[] PROGMEM = {
    0,     13,  26,  39,  51,  63,  74,  84,  94, 102, 109, 116, 120, 124, 126,
    127,  126, 124, 120, 116, 109, 102,  94,  84,  74,  63,  51,  39,  26,  13,
    0,    -13, -26, -39, -51, -63, -74, -84, -94,-102,-109,-116,-120,-124,-126,
    -127,-126,-124,-120,-116,-109,-102, -94, -84, -74, -63, -51, -39, -26, -13
};

// Scaled Sine: 60 values -> 2*PI
int8_t Sine60(int8_t angle, int8_t scale) {
    //    if(angle<0) angle=60-angle;
    while(angle>=60) angle-=60;
    int8_t temp = (int8_t)pgm_read_byte_near(SIN60+angle);
    return FMULS8R(temp, scale);
}

// Scaled Cosine: 60 values -> 2*PI
int8_t Cosine60(int8_t angle, int8_t scale) {
    //    if(angle<0) angle=-angle;
    angle+=15;
    while(angle>=60) angle-=60;
    int8_t temp = (int8_t)pgm_read_byte_near(SIN60+angle);
    return FMULS8R(temp, scale);
}
