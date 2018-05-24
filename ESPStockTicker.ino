#include <Arduino.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <Adafruit_GFX.h>
#include "ST7735_REV.h"
#include "Monospaced_plain_11.h"
//serve static html from PROGMEM
//so we don't need a separate data filese
#include "index.html.gz.h"
#include "wifi.html.gz.h"
#include "FS.h"
#include <elapsedMillis.h>
#define ARDUINOJSON_USE_DOUBLE 0

//default triangle color - doubt we need it
#define ST7735_GREY   0xB5F6
#define ST7735_DIMYELLOW 0xFF36

#define VERSION 2.16
//list of mac addresses for ESPs soldered to screwed up Ebay screen that print backwards.
//i call them YELLOWTABS because of they had yellow tabs on the screen protectors
const int YELLOW_TAB_SIZE = 1;
const char YELLOW_TABS[YELLOW_TAB_SIZE][18] = { "60:01:94:74:4A:42" };

const int ROTATION_SIZE = 1;
const char ROTATION_OVERRIDES[ROTATION_SIZE][18] = { "DC:4F:22:1C:2A:98" };

const int MAX_WIFI_NETWORKS = 5;
const char *WIFI_FILE = "/wifi.txt";

//number of 500 ms increments to wait for a connection
const int MAX_WIFI_TIMEOUT = 20;
const int HTTP_PORT = 80;
const int HTTPS_PORT = 443;
const int OTA_PORT = 8080;
char hostName [20] = "esp";
const char SOFT_AP_NAME [] = "ESPSoftAP";

//stock ticker api and SSL fingerprint
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
const char* KEY_STATS_FILE = "keystats.json";
const char* FW_REMOTE_VERSION_FILE = "/version.remote";
const char* ROTATION_FILE = "/rotation.txt";

const char* IEX_HOST = "api.iextrading.com";
const char* PRICING_CHART_URL = "GET /1.0/stock/market/batch?filter=latestPrice,changePercent&types=quote&displayPercent=true&symbols=";
//interval is number of minutes between prices
const char* BASE_CHART_URL = "GET /1.0/stock/%s/chart/1d?chartInterval=%d&filter=average";
const char* KEY_STATS_URL = "GET /1.0/stock/%s/stats?filter=week52low,week52high,day200MovingAvg";
const char* IEX_GET_SUFFIX = " HTTP/1.0\r\nHost: api.iextrading.com\r\nUser-Agent: ESP8266\r\nConnection: close\r\n\r\n";

//used to process stock selection form submission
//also used to create API GET request
const int GET_REQUEST_BUFFER_SIZE = 1000;
//https client connection timeout for API calls
const int CLIENT_TIMEOUT = 2000; //http read in ms

//where to check for firmware version
const char* FIRMWARE_HOST = "raw.githubusercontent.com";
const char* FW_VERSION_URL = "GET /brownjd/ESPStockTicker/master/data/version.txt";
const char* FW_GET_SUFFIX = " HTTP/1.0\r\nHost: raw.githubusercontent.com\r\nUser-Agent: ESP8266\r\nConnection: close\r\n\r\n";
//OTA update does not support HTTPS, so had to use amazon for firmware hosting
const char* FW_BIN_URL = "http://espstockticker.s3-website-us-east-1.amazonaws.com/ESPStockTicker.ino.nodemcu.bin";

//left side of chart in pixels
const int CHART_X_ORIGIN = 35;
//how many pixels between horizontal bars
const float CHART_X_SPACING = 19;//18.25;
//not 7.5, because of the leading 9:30 we draw separately
const float CHART_X_WIDTH = CHART_X_SPACING * 6.5;
//top border of chart - distance from origin
const int CHART_Y_ORIGIN = 3;
//total height of chart
const int CHART_Y_HEIGHT = 100;
//how many pixels between vertical bars
const int CHART_Y_SPACING = 20;
//vertical position of chart labels - hour
const int CHART_Y_TICKER_POS = 118;

//how much working memory to leave after allocating JSON buffer
//JSON processing buffer is dynamically allocated on stack and freed after processing
//because there is not enough room to reliably make an HTTPS connection and process JSON in
//a single call.
const int WORKING_HEAP = 15000;
//how often to get new data in ms
const int MAX_API_INTERVAL = 60000;
//how often to check for new firmware in ms
const int MAX_FW_INTERVAL = 600000;
//how often to switch display pages in ms
const int MAX_PRINT_INTERVAL = 8500;

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

//holds list of tickers
char tickers[TICKER_COUNT][MAX_TICKER_SIZE];
//hols list of prices and percentage change - position corresponds to tickers array
//PRICE, CHANGE
float prices[TICKER_COUNT][2];

//how many minutes between data points to request
const int CHART_INTERVAL = 2;
//9:30 - 4 pm = 390 minutes / increment
const int MAX_CHART_POINTS = 390 / CHART_INTERVAL;
//list of prices
float chartinfo[MAX_CHART_POINTS];
//200 day moving average
float movingAvg = 0.0f;
float yearlow = 0.0f;
float yearhigh = 0.0f;

//re-used and cleared to make API calls for prices and chart info
char requestBuffer [GET_REQUEST_BUFFER_SIZE] = {""};
//for serving configuration pages
ESP8266WebServer httpServer(HTTP_PORT);
//keep track of when to switch pages
elapsedMillis sincePrint;
//keep track of when to refresh data
elapsedMillis sinceAPIUpdate = MAX_API_INTERVAL;
//keep track of when to check for updates
elapsedMillis sinceFWUpdate = MAX_FW_INTERVAL;

int page = 0;

void setup()
{
  Serial.begin(115200);
  delay(2000);
  SPIFFS.begin();

  //remove temp data
  SPIFFS.remove(CHART_FILE);
  SPIFFS.remove(PRICING_FILE);
  SPIFFS.remove(KEY_STATS_FILE);

  initScreen();
  setupIPHandlers();
  connectWifi();
  setupWebServer();
  setupOTA();
  readTickerFile();
  Serial.print(F("End setup: free heap: "));
  Serial.println(ESP.getFreeHeap());
}

void loop()
{
  yield();
  ArduinoOTA.handle();
  yield();
  httpServer.handleClient();

  if (WiFi.status() == WL_CONNECTED)
  {
    if (sinceFWUpdate >= MAX_FW_INTERVAL)
    {
      yield();
      httpServer.handleClient();
      checkAvailableFirmwareVersion();
      if (compareFWVersion())
      {
        printStatusMsg("Updating firmware...");
        if (!ESPhttpUpdate.update(FW_BIN_URL))
        {
          Serial.println(F("Failed to update firmware."));
          Serial.println(ESPhttpUpdate.getLastError());
          Serial.println(ESPhttpUpdate.getLastErrorString());
        }
      }
      sinceFWUpdate = 0;
    }

    if (sinceAPIUpdate >= MAX_API_INTERVAL)
    {
      yield();
      httpServer.handleClient();
      queryPrices();
      yield();
      httpServer.handleClient();
      queryChartInfo();
      yield();
      httpServer.handleClient();
      sinceAPIUpdate = 0;
    }
    if(sincePrint >= MAX_PRINT_INTERVAL)
    {
      yield();
      httpServer.handleClient();
      printTickers();
      yield();
      httpServer.handleClient();
      sincePrint = 0;
    }
  }
  
  yield();
  httpServer.handleClient();
}

void updatePrices()
{
  Serial.println(F("updatePrices()..."));

  //clear memory first
  for(int tickerNum = 0; tickerNum < TICKER_COUNT; tickerNum++)
  {
    prices[tickerNum][0] = 0.0;
    prices[tickerNum][1] = 0.0;
  }

  
  File f = SPIFFS.open(PRICING_FILE, "r");
  Serial.print(F("Price file size: "));
  Serial.println(f.size());
  if(f.size() > 0)
  {  
    DynamicJsonBuffer jsonBuffer(3000);
    JsonObject &jsonQuotes = jsonBuffer.parseObject(f);
    
    if(jsonQuotes.success())
    {
      for(int tickerNum = 0; tickerNum < TICKER_COUNT; tickerNum++)
      {
        yield();
        char *ticker = tickers[tickerNum];
        
        if(ticker[0])
        {
          JsonObject &quoteItem = jsonQuotes[ticker][F("quote")];
          if(quoteItem.success())
          {
            float price = quoteItem[F("latestPrice")];
            float change = quoteItem[F("changePercent")];
        
            prices[tickerNum][0] = price;
            prices[tickerNum][1] = change;
          }
          else
          {
            Serial.print("JSON deserialization error with ticker: ");
            Serial.println(ticker);
          }
        }
      }
    }
    else
    {
      String s  = F("Pricing data error.");
      Serial.println(s);
      printStatusMsg(s);
      Serial.print(F("With string: "));
      f.seek(0, SeekSet);
      Serial.println(f.readString());
      sinceAPIUpdate = MAX_API_INTERVAL;
    }
  }
  f.close();
  
  Serial.println(F("updatePrices()...done"));
}

void updateChartInfo()
{
  Serial.println(F("updateChartInfo()..."));

  //clear out memory
  for(int i = 0; i < MAX_CHART_POINTS; i++)
  {
    chartinfo[i] = 0;
  }
  
  File f = SPIFFS.open(CHART_FILE, "r");
  Serial.print(F("Chart file size: "));
  Serial.println(f.size());
  bool parseError = false;
  if(f.size() > 0)
  {
    f.find('[');
    // Parsing one data point at a time because the list can get 
    // long and we don't have the headroom to load all the JSON
    // in memory at once
    for(int i = 0; i < MAX_CHART_POINTS && f.available(); i++)
    {
      yield();
      DynamicJsonBuffer jsonBuffer(100);
      JsonObject &jsonDataPoint = jsonBuffer.parseObject(f);
    
      if(jsonDataPoint.success())
      {          
          chartinfo[i] = jsonDataPoint[F("average")];
      }
      else
      {
        parseError = true;
        String s  = F("Chart data error.");
        Serial.println(s);
        printStatusMsg(s);
      }
      f.find(',');
    }
  }
  else
  {
    String s  = F("Chart data empty.");
    Serial.println(s);
    printStatusMsg(s);
  }

  if(parseError)
  {
    f.seek(0, SeekSet);
    Serial.println(f.readString());
    sinceAPIUpdate = MAX_API_INTERVAL;
  }
  
  f.close();

  f = SPIFFS.open(KEY_STATS_FILE, "r");
  if(f.size() > 0)
  {
    DynamicJsonBuffer jsonBuffer(100);
    JsonObject &jsonObj = jsonBuffer.parseObject(f);
    if(jsonObj.success())
    {
      yearlow = jsonObj[F("week52low")];
      yearhigh = jsonObj[F("week52high")];
      movingAvg = jsonObj[F("day200MovingAvg")];
    }
    else
    {
      String s  = F("Key stats error.");
      Serial.println(s);
      printStatusMsg(s);
      f.seek(0, SeekSet);
      Serial.println(f.readString());
      sinceAPIUpdate = MAX_API_INTERVAL;
    }
  }
  else
  {
    String s  = F("Key stats empty.");
    Serial.println(s);
    printStatusMsg(s);
  }
  
  f.close();
      
  Serial.println(F("updateChartInfo()...done"));
}
