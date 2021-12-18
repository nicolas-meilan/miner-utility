/*
  CoinSound.h - Server manager.
*/
#include <Arduino.h>

#include "CoinSound.h"

CoinSound::CoinSound(int pin)
{
  this->pin = pin;
  pinMode(pin, OUTPUT);
};

void CoinSound::dispatchMarioCoinSound()
{
  tone(this->pin, NOTE_B5, 200);
  tone(this->pin, NOTE_E6, 550);
  noTone(this->pin);
}
