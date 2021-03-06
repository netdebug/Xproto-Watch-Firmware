/****************************************************************************

XMEGA Oscilloscope and Development Kit

Gabotronics
December 2018

Copyright 2018 Gabriel Anzziani

This program is distributed under the terms of the GNU General Public License 

www.gabotronics.com
email me at: gabriel@gabotronics.com

*****************************************************************************/

#ifndef _AWG_H
#define _AWG_H

#include "hardware.h"

void moveF(void);
void LoadAWGvars(void);
void SaveAWGvars(void);
void BuildWave(void);

// Global AWG variable
extern uint8_t  AWGBuffer[BUFFER_AWG];
extern uint8_t  cycles;     // Cycles in AWG buffer

#endif
