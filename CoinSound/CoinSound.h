/*
  CoinSound.h - Buzzer coin sound.
*/
#ifndef CoinSound_h
#define CoinSound_h

#include <Arduino.h>

#include "Tone32.h"

class CoinSound
{
  public:
    CoinSound(int pin);
    void dispatchMarioCoinSound();
  
  private:
    int pin;
};

#endif
