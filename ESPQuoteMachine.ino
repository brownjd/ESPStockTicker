#include <Arduino.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <ArduinoOTA.h>
#include "Monospaced_plain_11.h"
#include "FS.h"
#include <elapsedMillis.h>
#define ARDUINOJSON_USE_DOUBLE 0

#define ST7735_GREY   0x9EFF

const int MAX_WIFI_NETWORKS = 5;
const char *WIFI_FILE = "/wifi.txt";

//number of 500 ms increments to wait for a connection
const int MAX_WIFI_TIMEOUT = 25;
const int HTTP_PORT = 80;
const int HTTPS_PORT = 443;
const int OTA_PORT = 8080;
char hostName [20] = "esp";
const char SOFT_AP_NAME [] = "ESPSoftAP";

//stock ticker api and SSL fingerprint
const char* HOST = "api.iextrading.com";
//const char* SSL_FINGERPRINT = "D1 34 42 D6 30 58 2F 09 A0 8C 48 B6 25 B4 6C 05 69 A4 2E 4E";

//Screen Settings
const int MAX_TICKER_SIZE = 8;
const int SCREEN_WIDTH = 160;
const int SCREEN_HEIGHT = 128;
const int ROW_HEIGHT = 15;
const int STATUS_MSG_HEIGHT = 8;
//calculate the number of rows we can fit in usable area
const int DISPLAY_ROWS = (SCREEN_HEIGHT - STATUS_MSG_HEIGHT) / ROW_HEIGHT;
const int TICKER_COUNT = 16;
//screen is 160 x 128
//column value is left hand side or start of column
const int COLUMN_0 = 0;                     //   0 - 40  (40 px wide) SYMBOL - FIXED
const int COLUMN_1 = 40;                    //  40 - 105 (65 px wide) PRICE - SCALED
const int COLUMN_2 = SCREEN_WIDTH - 60;                   // 105 - 155 (50 px wide) % CHANGE - FIXED
const int COLUMN_3 = SCREEN_WIDTH - 48;
const int COLUMN_4 = SCREEN_WIDTH - 6;      // 155 - 160 ( 5 px wide) % SYMBOL - SLIDES

const float MSG_LINE_SPACING = 1.75;

const char* TICKER_FILE = "/tickers.txt";
const char* CHART_FILE = "/chart.json";
const char* PRICING_FILE = "/prices.json";

const char* BASE_URL = "GET /1.0/stock/market/batch?filter=symbol,latestPrice,changePercent&types=quote&displayPercent=true&symbols=";
const char* HTTP_GET_SUFFIX = " HTTP/1.0\r\nHost: api.iextrading.com\r\nUser-Agent: ESP8266\r\nConnection: close\r\n\r\n";
//interval is number of minutes between prices
const char* BASE_CHART_URL = "GET /1.0/stock/%s/chart/1d?chartInterval=%d&filter=average";
const int CHART_INTERVAL = 2;
//used to process stock selection form submission
//also used to create API GET request
const int GET_REQUEST_BUFFER_SIZE = 1000;
const int CLIENT_TIMEOUT = 2000; //http read in ms

const int CHART_X_ORIGIN = 31;
const float CHART_X_SPACING = 18.25;
//not 7.5, because of the leading 9:30 we draw separately
const float CHART_X_WIDTH = CHART_X_SPACING * 6.5;
const int CHART_Y_ORIGIN = 3;
const int CHART_Y_HEIGHT = 100;
const int CHART_Y_SPACING = 20;
const int CHART_Y_TICKER_POS = 118;

const int WORKING_HEAP = 5000;
const int MAX_API_INTERVAL = 60000;

//TFT pins
#define TFT_CS  D8
#define TFT_RST D4
#define TFT_DC  D3  /A0

//SCL ->  SCK   ->  D5
//SDA ->  MOSI  ->  D7
//VCC     +3.3V
//LED-    GND
//LED+    VIN +5V
//GND     GND

char tickers[TICKER_COUNT][MAX_TICKER_SIZE];
//PRICE, CHANGE
float prices[TICKER_COUNT][2];

const int MAX_CHART_POINTS = 390/CHART_INTERVAL;
float chartinfo[MAX_CHART_POINTS];

char quoteRequestBuffer [GET_REQUEST_BUFFER_SIZE] = {""};
ESP8266WebServer httpServer(HTTP_PORT);
elapsedMillis sincePrint;
elapsedMillis sinceAPIUpdate = MAX_API_INTERVAL;
elapsedMillis sinceHeapPrint;

int page = 0;

void setup()
{
  Serial.begin(115200);
  delay(2000);
  Serial.print(F("Begin setup: free heap: "));
  Serial.println(ESP.getFreeHeap());
  SPIFFS.begin();

  /*
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  
  Serial.print(F("Total bytes:"));
  Serial.println(fs_info.totalBytes);
  Serial.print(F("Used bytes:"));
  Serial.println(fs_info.usedBytes);
  Serial.print(F("maxOpenFiles:"));
  Serial.println(fs_info.maxOpenFiles);
  Serial.print(F("maxPathLength:"));
  Serial.println(fs_info.maxPathLength);
  */
  
  initScreen();
  setupIP();
  connectWifi();  
  setupMDNS();
  setupWebServer();
  setupOTA();
  readTickerFile();
  Serial.print(F("End setup: free heap: "));
  Serial.println(ESP.getFreeHeap());
}

bool reload = false;

void loop()
{
  yield();
  ArduinoOTA.handle();
  yield();
 
  if (WiFi.status() != WL_CONNECTED)
  {
    connectWifi();
  }
  else
  {
    if(sinceAPIUpdate >= MAX_API_INTERVAL)
    {
      yield();
      httpServer.handleClient();
      queryPrices();
      yield();
      httpServer.handleClient();
      queryChartInfo();
      sinceAPIUpdate = 0;
    }
    if(sincePrint >= 7500)
    {
      yield();
      updatePrices();
      yield();
      updateChartInfo();
      printTickers();
      httpServer.handleClient();
      //printChart();
      sincePrint = 0;
    }
  }
  if(sinceHeapPrint >= 2000)
  {
    sinceHeapPrint = 0;
    Serial.print(F("Loop: free heap: "));
    Serial.println(ESP.getFreeHeap());
  }
  
  yield();
  httpServer.handleClient();
}




