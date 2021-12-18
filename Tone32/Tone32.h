#ifndef _TONE32_h
#define _TONE32_h

#define TONE_CHANNEL 15
#include "Arduino.h"
#include "pitches.h"

void tone(uint8_t pin, unsigned int frequency, unsigned long duration = 0);
void noTone(uint8_t pin);
#endif



