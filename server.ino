#include <Arduino.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "Wifi.h"
#include "CustomServer.h"
#include "Json.h"

// COINS
const String ETH = "ETH";
const String USD = "USD";
const String ARS = "ARS";
const int UNPAID_COIN_EXPONENTIAL = 3;
const int COIN_DECIMALS = 5;
const int FIAT_DECIMALS = 2;

// Wifi
const String ACCESS_POINT_SSID = "minerUtilityConfig";
const String MDNS_NAME = "minerUtility";
const String CONNECTING = "Conectando ...";
const String CONFIGURE = "Modo Configurable";
const String FAILED_CONNECTION_TITLE = "No se pudo conectar a la red wifi";
const String FAILED_CONNECTION_MESSAGE = "Pruebe a resetear y volver a configurar";

// DELAYS
const int BASE_DELAY = 350;
const int DELAY_FETCH = 60000 * 5;

// Server
const int SERVER_PORT = 80;
const String SERVER_RESPONSE_TYPE = "application/json";

// GET API
const String MINER_URL = "https://api.ethermine.org/miner/:WALLET";
const String PRICES_URL = "https://min-api.cryptocompare.com/data/price?fsym=" + ETH + "&tsyms=" + USD + ',' + ARS;
const String CURRENT_STATS = "/currentStats";
const String PAYOUTS = "/payouts";

// Keys
const String CONFIGURATION_KEY = "configuration";
const String SSID_KEY = "ssid";
const String PASSWORD_KEY = "password";
const String ETH_WALLET_KEY = "wallet";
const String HAS_CONFIGURATION_KEY = "hasConf";

// Display
const int DISPLAY_WIDTH = 16;
const int DISPLAY_HEIGHT = 2;
const int DISPLAY_SDA = 23;
const int DISPLAY_SCL = 22;
const String SEPARATOR = "/";

// LEDS
const int GREEN_LED = 12;
const int RED_LED = 13;
const int FLICKER_DELAY = 500;

// Reset
const int RESET_BUTTON = 32;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
LiquidCrystal_I2C lcd(0x27, DISPLAY_WIDTH, DISPLAY_HEIGHT);
Preferences preferences;
Wifi wifi = Wifi::getInstance();
CustomServer server(SERVER_PORT);
HTTPClient http;
bool hasConfiguration = false;
String ethWallet = "";
unsigned long prevMillis = millis() + DELAY_FETCH;

void setupConfig()
{
  String connectionSsid = preferences.getString(SSID_KEY.c_str(), "");
  String passwd = preferences.getString(PASSWORD_KEY.c_str(), "");
  ethWallet = preferences.getString(ETH_WALLET_KEY.c_str(), "");
  hasConfiguration = preferences.getBool(HAS_CONFIGURATION_KEY.c_str(), false);
  lcd.clear();
  lcd.home();
  if (hasConfiguration)
  {
    lcd.print(CONNECTING);
    wifi.connect(connectionSsid, passwd, MDNS_NAME, onWifiTryToConnect);
    if (wifi.isConnected())
    {
      timeClient.begin();
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, LOW);
    }
    else
    {
      lcd.clear();
      lcd.home();
      lcd.print(FAILED_CONNECTION_TITLE);
      lcd.setCursor(0, 1);
      lcd.print(FAILED_CONNECTION_MESSAGE);
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, HIGH);
    }
  }
  else
  {
    lcd.print(CONFIGURE);
    lcd.setCursor(0, 1);
    lcd.print(ACCESS_POINT_SSID);
    wifi.accessPoint(ACCESS_POINT_SSID); // 192.168.4.1
    initializeServer();
  }
}

void setWifiConfig()
{
  Json body(server.getBody());
  String ssid = body.getAttribute<String>(SSID_KEY);
  String passwd = body.getAttribute<String>(PASSWORD_KEY);
  if (ssid)
    preferences.putString(SSID_KEY.c_str(), ssid);
  if (passwd)
    preferences.putString(PASSWORD_KEY.c_str(), passwd);
  String configSsid = preferences.getString(SSID_KEY.c_str(), "");
  String configIp = preferences.getString(SSID_KEY.c_str(), "");
  server.send(200, SERVER_RESPONSE_TYPE, "{}");
  if (configSsid)
  {
    preferences.putBool(HAS_CONFIGURATION_KEY.c_str(), true);
    delay(BASE_DELAY);
    wifi.disconnectAccessPoint();
    setupConfig();
  }
}

void setEthWallet()
{
  Json body(server.getBody());
  if (!body.attributeExists(ETH_WALLET_KEY))
  {
    server.send(422, SERVER_RESPONSE_TYPE, "{ error: \"Missing Wallet\" }");
    return;
  }

  String wallet = body.getAttribute<String>(ETH_WALLET_KEY);

  preferences.putString(ETH_WALLET_KEY.c_str(), wallet);
  server.send(200, SERVER_RESPONSE_TYPE, "{}");
}

void initializeServer()
{
  server.on("/wifiConfig", HTTP_POST, setWifiConfig);
  server.on("/ethwallet", HTTP_POST, setEthWallet);

  server.begin();
}

double formatCoinProfitToDouble(String unpaidStr)
{
  const int expIndex = unpaidStr.indexOf('e');
  double unpaid = unpaidStr.substring(0, expIndex).toDouble();
  for (int i = 0; i < UNPAID_COIN_EXPONENTIAL; i++)
  {
    unpaid = unpaid / 10;
  }

  return unpaid;
}

unsigned long getActualMonthTimestamp()
{
  const unsigned long currentTimestamp = timeClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&currentTimestamp);
  // initialize the month
  ptm->tm_mday = 1;
  ptm->tm_hour = 0;
  ptm->tm_min = 0;
  ptm->tm_sec = 0;

  return mktime(ptm);
}

void getEthPrice(double *prices)
{
  http.begin(PRICES_URL.c_str());
  const int httpResponseCode = http.GET();
  if (httpResponseCode >= 200 && httpResponseCode < 400)
  {
    Json response(http.getString());
    double usdPerEth = response.getAttribute<double>(USD);
    double arsPerEth = response.getAttribute<double>(ARS);
    prices[0] = usdPerEth;
    prices[1] = arsPerEth;
  }
}

void getMinerData(double *savedResponse)
{
  String fetchURL = MINER_URL + CURRENT_STATS;
  fetchURL.replace(":WALLET", ethWallet);
  http.begin(fetchURL.c_str());
  const int httpResponseCode = http.GET();
  if (httpResponseCode >= 200 && httpResponseCode < 400)
  {
    Json response(http.getString());
    double ethPerDay = response.getAttribute<double>(2, "data", "coinsPerMin") * 60 * 24;
    double usdPerDay = response.getAttribute<double>(2, "data", "usdPerMin") * 60 * 24;
    double unpaid = formatCoinProfitToDouble(response.getAttribute<String>(2, "data", "unpaid"));
    savedResponse[0] = ethPerDay;
    savedResponse[1] = usdPerDay;
    savedResponse[2] = unpaid;
    http.end();
    return;
  }

  http.end();
  savedResponse[0] = 0;
  savedResponse[1] = 0;
}

double getMonthlyProfit()
{
  const unsigned long month = getActualMonthTimestamp();
  String fetchURL = MINER_URL + PAYOUTS;
  fetchURL.replace(":WALLET", ethWallet);
  http.begin(fetchURL.c_str());
  const int httpResponseCode = http.GET();
  if (httpResponseCode >= 200 && httpResponseCode < 400)
  {
    Json response(http.getString());
    Json payouts(response.getAttribute<String>("data"));
    double amount = 0;
    for (int i = 0; i < payouts.size(); i++)
    {
      Json payoutItem(payouts.getArrayItem<String>(i));
      const unsigned long paidOn = payoutItem.getAttribute<unsigned long>("paidOn");
      const double payoutAmount = formatCoinProfitToDouble(payoutItem.getAttribute<String>("amount"));
      if (paidOn > month)
        amount += payoutAmount;
    }

    return amount;
  }

  return 0;
}

void onWifiTryToConnect()
{
  digitalWrite(GREEN_LED, HIGH);
  delay(FLICKER_DELAY);
  digitalWrite(GREEN_LED, LOW);
}

void onConfiguration()
{
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, HIGH);
  delay(FLICKER_DELAY);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
}

void setup()
{
  Serial.begin(9600);
  Wire.begin(DISPLAY_SDA, DISPLAY_SCL);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(RESET_BUTTON, INPUT);
  lcd.begin(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  lcd.backlight();
  preferences.begin(CONFIGURATION_KEY.c_str(), false); // RW
  setupConfig();
}

void loop()
{
  if (hasConfiguration)
  {
    const unsigned long millisDifference = millis() - prevMillis;
    const int resetButton = digitalRead(RESET_BUTTON);
    if (resetButton == HIGH)
    {
      preferences.putBool(HAS_CONFIGURATION_KEY.c_str(), false);
      setupConfig();
    }
    else if (millisDifference >= DELAY_FETCH)
    {
      prevMillis = millis();
      timeClient.update();
      double response[3];
      double prices[2];
      getEthPrice(prices);
      getMinerData(response);
      double monthlyProfit = getMonthlyProfit();
      lcd.clear();
      lcd.home();
      lcd.print(String(response[0], COIN_DECIMALS) + ' ' + ETH + SEPARATOR);
      lcd.print(String(response[1], FIAT_DECIMALS) + ' ' + USD + SEPARATOR);
      lcd.print(String((response[0] * prices[1]), FIAT_DECIMALS) + ' ' + ARS);
      lcd.setCursor(0, 1);
      lcd.print(String(monthlyProfit, COIN_DECIMALS) + ' ' + ETH + SEPARATOR);
      lcd.print(String(monthlyProfit * prices[0], FIAT_DECIMALS) + ' ' + USD + SEPARATOR);
      lcd.print(String((monthlyProfit * prices[1]), FIAT_DECIMALS) + ' ' + ARS);
    }
  }
  else
  {
    onConfiguration();
    server.handleClient();
  }
  lcd.scrollDisplayLeft();
  delay(BASE_DELAY);
}