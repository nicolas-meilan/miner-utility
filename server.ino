#include <Arduino.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TM1637Display.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "Wifi.h"
#include "CustomServer.h"
#include "Json.h"
#include "CoinSound.h"

// Coins
const String ETH = "ETH";
const String USD = "USD";
const String ARS = "ARS";
const int ETH_WEI_DECIMALS = 18;
const int COIN_DECIMALS = 5;
const int FIAT_DECIMALS = 2;

// Wifi
const String ACCESS_POINT_SSID = "minerUtilityConfig";
const String MDNS_NAME = "minerUtility";
const String CONNECTING = "Conectando ...";
const String UPDATING = "Actualizando ...";
const String CONFIGURE = "Modo Configurable";
const String FAILED_CONNECTION_TITLE = "No se pudo conectar a la red wifi";
const String FAILED_CONNECTION_MESSAGE = "Pruebe a resetear y volver a configurar";

// Server
const int SERVER_PORT = 80;
const String SERVER_RESPONSE_TYPE = "application/json";

// Get API
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
const int DISPLAY_SDA = 22;
const int DISPLAY_SCL = 23;
const int SEPARATOR_CHAR_ID = 0;
const int ETH_COL = 11;
const int USD_COL = 11;
const int ARS_COL = 14;
uint8_t SEPARATOR[8] = {
    B00100,
    B00100,
    B00100,
    B00100,
    B00100,
    B00100,
    B00100,
    B00100,
};

// Numeric Display
const int NUM_DISPLAY_DIO = 16;
const int NUM_DISPLAY_CLK = 17;

// Buzzer
const int BUZZER_PIN = 2;

// LEDS
const int GREEN_LED = 12;
const int RED_LED = 13;
const int FLICKER_DELAY = 500;

// Reset
const int RESET_BUTTON = 32;

// Delays
const unsigned long BASE_DELAY = 350;
const unsigned long DELAY_FETCH = 15 * 60000;

Wifi wifi = Wifi::getInstance();
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
HTTPClient http;
CustomServer server(SERVER_PORT);
LiquidCrystal_I2C lcd(0x27, DISPLAY_WIDTH, DISPLAY_HEIGHT);
TM1637Display numericDisplay(NUM_DISPLAY_CLK, NUM_DISPLAY_DIO);
CoinSound sound(BUZZER_PIN);
Preferences preferences;
bool hasConfiguration = false;
String ethWallet = "";
unsigned long prevMillis = millis() + DELAY_FETCH;
double prevPendingPayout = 0;

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

String formatColumn(String content, int size)
{
  const int spacesNeeded = size - content.length();
  const bool spacesNeedesIsPair = !(spacesNeeded % 2);
  String spacesBefore;
  String spacesAfter;
  for (int i = 0; i < spacesNeeded / 2; i++)
  {
    spacesBefore += ' ';
    spacesAfter += ' ';
  }

  if (!spacesNeedesIsPair)
    spacesAfter += ' ';

  return spacesBefore + content + spacesAfter;
}

void printEthBalance(double eth, double usdEth, double arsEth)
{
  const String ethAmount = String(eth, COIN_DECIMALS) + ' ' + ETH;
  const String usdAmount = String(eth * usdEth, FIAT_DECIMALS) + ' ' + USD;
  const String arsAmount = String(eth * arsEth, FIAT_DECIMALS) + ' ' + ARS;

  lcd.print(formatColumn(ethAmount, ETH_COL));
  lcd.write(SEPARATOR_CHAR_ID);
  lcd.print(formatColumn(usdAmount, USD_COL));
  lcd.write(SEPARATOR_CHAR_ID);
  lcd.print(formatColumn(arsAmount, ARS_COL));
  lcd.write(SEPARATOR_CHAR_ID);
}

double weiToEth(String profitStr)
{
  const int expIndex = profitStr.indexOf('e');
  double profit = profitStr.substring(0, expIndex).toDouble();
  int exp = ETH_WEI_DECIMALS - profitStr.substring(expIndex + 1).toInt();
  for (int i = 0; i < exp; i++)
  {
    profit = profit / 10;
  }

  return profit;
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

void getEthPrice(double prices[2])
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

void getPoolStats(double savedResponse[3])
{
  String fetchURL = MINER_URL + CURRENT_STATS;
  fetchURL.replace(":WALLET", ethWallet);
  http.begin(fetchURL.c_str());
  const int httpResponseCode = http.GET();
  if (httpResponseCode >= 200 && httpResponseCode < 400)
  {
    Json response(http.getString(), "data");
    const double ethPerDay = response.getAttribute<double>("coinsPerMin") * 60 * 24;           // Min to Day
    const double averageHashrate = response.getAttribute<double>("averageHashrate") / 1000000; // H to MH
    const double unpaid = weiToEth(response.getAttribute<String>("unpaid"));
    savedResponse[0] = ethPerDay;
    savedResponse[1] = unpaid;
    savedResponse[2] = averageHashrate;
    http.end();
    return;
  }

  http.end();
  savedResponse[0] = 0;
  savedResponse[2] = 0;
  savedResponse[3] = 0;
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
    Json payouts(http.getString(), "data");
    double amount = 0;
    for (int i = 0; i < payouts.size(); i++)
    {
      Json payoutItem(payouts.getArrayItem<String>(i));
      const unsigned long paidOn = payoutItem.getAttribute<unsigned long>("paidOn");
      const double payoutAmount = weiToEth(payoutItem.getAttribute<String>("amount"));
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
  Wire.begin(DISPLAY_SDA, DISPLAY_SCL);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(RESET_BUTTON, INPUT);
  lcd.begin(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  lcd.createChar(SEPARATOR_CHAR_ID, SEPARATOR);
  lcd.backlight();
  numericDisplay.setBrightness(1);
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
      lcd.clear();
      lcd.home();
      lcd.print(UPDATING);
      prevMillis = millis();
      timeClient.update();
      double poolStats[3];
      double prices[2];
      getEthPrice(prices);
      getPoolStats(poolStats);
      double monthlyProfit = getMonthlyProfit();
      numericDisplay.clear();
      numericDisplay.showNumberDec((int)poolStats[2], true);
      lcd.clear();
      lcd.home();
      //  Daily estimated
      // printEthBalance(poolStats[0], prices[0], prices[1]);
      // Unpaid
      const double pendingPayout = poolStats[1];
      printEthBalance(pendingPayout, prices[0], prices[1]);
      lcd.setCursor(0, 1);
      // Monthly consolidated
      printEthBalance(monthlyProfit, prices[0], prices[1]);
      const boolean wasPaymentMade = prevPendingPayout > pendingPayout;
      if (wasPaymentMade)
        sound.dispatchMarioCoinSound();
      prevPendingPayout = pendingPayout;
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
