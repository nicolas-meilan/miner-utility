/*
  Json.h - Json manager.
*/
#include <Arduino.h>

#include "Json.h"

Json::Json(String json)
{
  this->json = json;
};

Json::Json(String json, String baseKey)
{
  const int indexOfKey = json.indexOf(baseKey);
  const int initialSubjsonIndex = indexOfKey + baseKey.length() + 2;
  const String subJson = json.substring(initialSubjsonIndex, json.length() - 1);
  this->json = subJson;
};

int Json::size()
{
  StaticJsonDocument<2048> json;
  deserializeJson(json, this->json);

  return json.size();
}

boolean Json::attributeExists(String key)
{
  StaticJsonDocument<2048> json;
  deserializeJson(json, this->json);

  return json.containsKey(key);
}

boolean Json::attributeExists(int length, ...)
{
  va_list keys;
  va_start(keys, length);
  StaticJsonDocument<2048> json;
  deserializeJson(json, this->json);
  const char *firstKey = va_arg(keys, char *);
  StaticJsonDocument<2048> neestedJson = json;
  if (!json.containsKey(firstKey))
    return false;
  for (int i = 1; i < length; i++)
  {
    const char *key = va_arg(keys, char *);
    if (!neestedJson.containsKey(key))
      return false;
    neestedJson = neestedJson[key];
  }

  return true;
}
