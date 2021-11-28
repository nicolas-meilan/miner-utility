/*
  Json.h - Json manager.
*/
#include <Arduino.h>

#include "Json.h"

Json::Json(String jsonStr)
{
  deserializeJson(this->json, jsonStr);
};

Json::Json(String jsonStr, String key)
{
  Json::Attribute attribute = attributeFromString(key);
  String subJsonStr = jsonStr;
  for (int i = 0; i < attribute.length; i++)
  {
    subJsonStr = this->removeKeyFromString(jsonStr, key);
  }

  deserializeJson(this->json, subJsonStr);
};

int Json::size()
{
  return this->json.size();
}

boolean Json::attributeExists(String key)
{
  Json::Attribute attribute = this->attributeFromString(key);
  JsonObject neestedJson = this->json.as<JsonObject>();

  for (int i = 0; i < attribute.length; i++)
  {
    String actualKey = attribute.keys[i];
    if (!this->json.containsKey(actualKey))
      return false;

    neestedJson = neestedJson[actualKey];
  }

  return true;
}

String Json::removeKeyFromString(String jsonStr, String key)
{
  const int indexOfKey = jsonStr.indexOf(key);
  const int initialSubjsonIndex = indexOfKey + key.length() + 2;

  return jsonStr.substring(initialSubjsonIndex, jsonStr.length() - 1);
}

Json::Attribute Json::attributeFromString(String key)
{
  String keyToEdit = key;
  const char separator = '.';
  const char fakeSeparator = '-';
  int beginIndex = 0;
  int endIndex = keyToEdit.indexOf(separator);
  Json::Attribute attribute;

  if (endIndex < 0)
  {
    attribute.keys[attribute.length++] = key;
    return attribute;
  }

  while (endIndex >= 0)
  {
    attribute.keys[attribute.length++] = keyToEdit.substring(beginIndex, endIndex);
    beginIndex = endIndex + 1;
    keyToEdit.setCharAt(endIndex, fakeSeparator);
    endIndex = keyToEdit.indexOf(separator);
  }

  attribute.keys[attribute.length++] = keyToEdit.substring(beginIndex);

  return attribute;
}
