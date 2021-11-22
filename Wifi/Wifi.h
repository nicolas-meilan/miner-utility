/*
  Wifi.h - Wifi manager.
*/
#ifndef Wifi_h
#define Wifi_h

#include <Arduino.h>
#include <WiFi.h>

class Wifi
{
public:
  String connect(String ssid, void (*onTryToConnect)() = NULL);
  String connect(String ssid, String passwd, void (*onTryToConnect)() = NULL);
  String connect(String ssid, String passwd, String mdns, void (*onTryToConnect)() = NULL);
  bool mdns(String mdnsName);
  void disconnect();
  void accessPoint(String ssid, String passwd);
  void accessPoint(String ssid);
  void disconnectAccessPoint();
  String getIp();
  static Wifi &getInstance();
  boolean isConnected();
  static const String VOID_IP;

private:
  Wifi();
  String ip;
  static Wifi *instance;
  static const int DELAY_PER_CONNECTION = 500;
  static const int CONNECTION_ATTEMPS = 30;

  String tryToConnect(void (*onTryToConnect)() = NULL);
};

#endif
