/*
  Json.h - Json manager.
*/
#include <Arduino.h>

#include "Json.h"

Json::Json(String jsonStr)
{
  deserializeJson(this->json, jsonStr);
};

Json::Json(String jsonStr, String baseKey)
{
  const int indexOfKey = jsonStr.indexOf(baseKey);
  const int initialSubjsonIndex = indexOfKey + baseKey.length() + 2;
  const String subJsonStr = jsonStr.substring(initialSubjsonIndex, jsonStr.length() - 1);
  deserializeJson(this->json, subJsonStr);
};

int Json::size()
{
  return this->json.size();
}

boolean Json::attributeExists(String key)
{
  return this->json.containsKey(key);
}

boolean Json::attributeExists(int length, ...)
{
  va_list keys;
  va_start(keys, length);
  const char *firstKey = va_arg(keys, char *);
  StaticJsonDocument<2048> neestedJson = this->json;
  if (!this->json.containsKey(firstKey))
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
