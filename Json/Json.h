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
  Json(String jsonStr, String key);
  template <typename T>
  T getArrayItem(int index)
  {
    return this->json[index];
  }

  template <typename T>
  T getAttribute(String key)
  {
    Json::Attribute attribute = this->attributeFromString(key);
    JsonObject neestedJson = this->json.as<JsonObject>();

    for (int i = 0; i < attribute.length -1; i++)
    {
      const String actualKey = attribute.keys[i];
      neestedJson = neestedJson[actualKey];
    }

    T finalAttribute = neestedJson[attribute.keys[attribute.length -1]];

    return finalAttribute;
  }

  int size();
  boolean attributeExists(String key);

private:
  static const int maxJsonAttributes = 10;
  StaticJsonDocument<2048> json;

  struct Attribute
  {
    String keys[maxJsonAttributes];
    int length = 0;
  };

  Attribute attributeFromString(String key);
  String removeKeyFromString(String jsonStr, String key);
};

#endif
