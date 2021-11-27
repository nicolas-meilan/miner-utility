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
    StaticJsonDocument<2048> json;
    deserializeJson(json, this->json);
    return json[index];
  }
  template <typename T>
  T getAttribute(String key)
  {
    if (!this->attributeExists(key))
      return (T) false;

    StaticJsonDocument<2048> json;
    deserializeJson(json, this->json);
    return json[key];
  }
  template <typename T>
  T getAttribute(int length, ...)
  {
    va_list keys;
    va_start(keys, length);
    StaticJsonDocument<2048> json;
    deserializeJson(json, this->json);
    const char *firstKey = va_arg(keys, char *);
    StaticJsonDocument<2048> neestedJson = json[firstKey];
    for (int i = 1; i < length - 1; i++)
    {
      const char *key = va_arg(keys, char *);
      const String neestedStr = neestedJson[key];
      deserializeJson(neestedJson, neestedStr);
    }
    return neestedJson[va_arg(keys, char *)];
  }
  int size();
  boolean attributeExists(String key);
  boolean attributeExists(int length, ...);

private:
  String json;
};

#endif
