/*
  Json.h - Json manager.
*/
#ifndef Json_h
#define Json_h

#include <Arduino.h>
#include <ArduinoJson.h>

class Json
{
public:
  Json(String jsonStr);
  Json(String jsonStr, String baseKey);
  template <typename T>
  T getArrayItem(int index)
  {
    return this->json[index];
  }
  template <typename T>
  T getAttribute(String key)
  {
    if (!this->attributeExists(key))
      return (T) false;

    return this->json[key];
  }
  template <typename T>
  T getAttribute(int length, ...)
  {
    va_list keys;
    va_start(keys, length);
    const char *firstKey = va_arg(keys, char *);
    StaticJsonDocument<2048> neestedJson = this->json[firstKey];
    for (int i = 1; i < length - 1; i++)
    {
      const char *key = va_arg(keys, char *);
      neestedJson = neestedJson[key];
    }
    return neestedJson[va_arg(keys, char *)];
  }
  int size();
  boolean attributeExists(String key);
  boolean attributeExists(int length, ...);

private:
  StaticJsonDocument<2048> json;
};

#endif
