#include "Tone32.h"

void tone(uint8_t pin, unsigned int frequency, unsigned long duration)
{
  const long delayValue = frequency ? 1000000 / frequency / 2 : 0;
  const long numCycles = frequency * duration / 1000;
  for (long i = 0; i < numCycles; i++)
  {
    digitalWrite(pin, HIGH);
    delayMicroseconds(delayValue);
    digitalWrite(pin, LOW);
    delayMicroseconds(delayValue);
  }
}

void noTone(uint8_t pin)
{
  tone(pin, 0);
}
