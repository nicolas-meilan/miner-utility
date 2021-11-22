/*
  Wifi.h - Wifi manager.
*/
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>

#include "Wifi.h"

const String Wifi::VOID_IP = "0.0.0.0";

Wifi *Wifi::instance = nullptr;

Wifi::Wifi()
{
  this->ip = Wifi::VOID_IP;
};

Wifi &Wifi::getInstance()
{
  if (Wifi::instance == nullptr)
    Wifi::instance = new Wifi;

  return *Wifi::instance;
}

String Wifi::tryToConnect(void (*onTryToConnect)())
{
  for (int i = 0; i < Wifi::CONNECTION_ATTEMPS; i++)
  {
    if (onTryToConnect)
      (*onTryToConnect)();
    delay(Wifi::DELAY_PER_CONNECTION);
    if (this->isConnected())
    {
      this->ip = WiFi.localIP().toString();
      return this->ip;
    }
  }

  this->disconnect();

  return this->ip;
}

String Wifi::connect(String ssid, void (*onTryToConnect)())
{
  WiFi.begin(ssid.c_str(), "");
  return this->tryToConnect(onTryToConnect);
}

String Wifi::connect(String ssid, String passwd, void (*onTryToConnect)())
{
  WiFi.begin(ssid.c_str(), passwd.c_str());
  return this->tryToConnect(onTryToConnect);
}

String Wifi::connect(String ssid, String passwd, String mdns, void (*onTryToConnect)())
{
  WiFi.begin(ssid.c_str(), passwd.c_str());
  this->tryToConnect(onTryToConnect);
  if (this->isConnected())
  {
    this->mdns(mdns);
  }

  return this->getIp();
}

bool Wifi::mdns(String mdnsName)
{
  return MDNS.begin(mdnsName.c_str());
}

void Wifi::disconnect()
{
  WiFi.disconnect();
}

void Wifi::accessPoint(String ssid)
{
  IPAddress ip = WiFi.softAP(ssid.c_str(), "");
  this->ip = ip.toString();
}

void Wifi::accessPoint(String ssid, String passwd)
{
  IPAddress ip = WiFi.softAP(ssid.c_str(), passwd.c_str());
  this->ip = ip.toString();
}

void Wifi::disconnectAccessPoint()
{
  WiFi.softAPdisconnect();
}

boolean Wifi::isConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

String Wifi::getIp()
{
  return this->ip;
}