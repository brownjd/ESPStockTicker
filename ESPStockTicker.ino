#include <Arduino.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <elapsedMillis.h>

#include "ESPStockTicker.h"
//serve static html from PROGMEM
//so we don't need a separate data files dir
//use node.js gulp to regenerate from html files in data dir
#include "data/index.html.gz.h"
#include "data/wifi.html.gz.h"
#include "FS.h"

void setup()
{
  Serial.begin(9600);
  yield();
  delay(2000);
  yield();
  Serial.printf_P(PSTR("\n\n*** Last reset reason: %s ***\n\n"), ESP.getResetReason().c_str());
  Serial.println(F("setup()...starting"));
  SPIFFS.begin();

  cleanupDataFiles();

  initScreen();
  yield();
  connectWifi();
  yield();
  startSoftAP();
  yield();
  setupWebServer();
  yield();
  Serial.println(F("setup()...done"));
}

void loop()
{
  httpServer.handleClient();
  yield();
  startSoftAP();
  yield();
  httpServer.handleClient();
  
  if(wifiMulti.run() == WL_CONNECTED)
  {
    if(sinceFWUpdate >= MAX_FW_INTERVAL)
    {
      checkAvailableFirmwareVersion();
      yield();
      if (compareFWVersion())
      {
        printStatusMsg(F("Updating firmware..."));
        updateFirmware();
      }
      sinceFWUpdate = 0;
    }
    
    if(sinceStockAPIUpdate >= MAX_STOCK_API_INTERVAL)
    {
      yield();
      httpServer.handleClient();
      queryPrices();
      
      yield();
      httpServer.handleClient();
      queryChartInfo();

      sinceStockAPIUpdate = 0;
    }

    if(sinceFedAPIUpdate >= MAX_FED_API_INTERVAL)
    {
      if(SHOW_TBILLS)
      {
        yield();
        httpServer.handleClient();
        queryFed(FED_HOST, TBILL_URL, TBILL_HIST_FILE);
        
      }
      if(SHOW_OIL)
      {
        yield();
        httpServer.handleClient();
        queryFed(FED_HOST, OIL_URL, OIL_HIST_FILE);
      }

      sinceFedAPIUpdate = 0;
    }

    if(sinceCoinAPIUpdate >= MAX_COIN_API_INTERVAL)
    {
      if(SHOW_BITCOIN)
      {
        yield();
        httpServer.handleClient();
        queryCoinCurrent();
        yield();
        httpServer.handleClient();
        queryCoinHistorical();
        yield();
        httpServer.handleClient();
      }
      
      sinceCoinAPIUpdate = 0;
    }
    
    if(sincePrint >= MAX_PAGE_INTERVAL)
    {
      Serial.printf_P(PSTR("\tHeap Fragmentation: %d\n"), ESP.getHeapFragmentation());
      Serial.printf_P(PSTR("\tHeap MaxFreeBlockSize: %d\n"),ESP.getMaxFreeBlockSize());
      
      //page can change sincePrint, so need to do this at the outset
      sincePrint = 0;
      yield();
      httpServer.handleClient();
      displayNextPage();
      yield();
      httpServer.handleClient();
    }
  }
}

void updatePrices()
{
  Serial.println(F("updatePrices()..."));

  char tickers[TICKER_COUNT][MAX_TICKER_SIZE];
  readTickerFile(tickers);

  //clear memory first
  for(int tickerNum = 0; tickerNum < MAX_STRINGS; tickerNum++)
  {
    price_list[tickerNum][0] = 0.0;
    price_list[tickerNum][1] = 0.0;
  }

  File f = SPIFFS.open(PRICING_FILE, "r");
  int size = f.size();
  Serial.printf_P(PSTR("\tPrice file size: %d\n"), size);
  
  if(size > 0)
  {  
    DynamicJsonDocument jsonQuotes(3000);
    DeserializationError err = deserializeJson(jsonQuotes, f);
    
    if(!err)
    {
      for(int tickerNum = 0; tickerNum < TICKER_COUNT; tickerNum++)
      {
        yield();
        char *ticker = tickers[tickerNum];
        
        if(ticker[0])
        {
          JsonObject quoteItem = jsonQuotes[ticker][F("quote")];      
          price_list[tickerNum][0] = quoteItem[F("latestPrice")];;
          price_list[tickerNum][1] = quoteItem[F("changePercent")];;
        }
      }
    }
    else
    {
      const char* s  = "Pricing data error.";
      Serial.printf_P(PSTR("\t%s %s\n"), s, err.c_str());
      printStatusMsg(s);

      f.seek(0, SeekSet);
      Serial.println(f.readString());
      sinceStockAPIUpdate = MAX_STOCK_API_INTERVAL;
    }
  }
  f.close();
  
  Serial.println(F("updatePrices()...done"));
}

void updateChartInfo()
{
  Serial.println(F("updateChartInfo()..."));

  //clear out memory
  for(int i = 0; i < MAX_STRINGS; i++)
  {
    price_list[i][0] = 0.0f;
  }
  
  File f = SPIFFS.open(CHART_FILE, "r");
  Serial.printf_P(PSTR("\tChart file size: %d\n"), f.size());
  bool parseError = false;
  if(f.size() > 0)
  {
    f.find('[');
    // Parsing one data point at a time because the list can get 
    // long and we don't have the headroom to load all the JSON
    // in memory at once
    DynamicJsonDocument jsonDataPoint(150);
    yield();
    for(int i = 0; i < MAX_CHART_POINTS && f.available(); i++)
    {
      //Serial.printf_P(PSTR("\ti: %d\n"), i);
      yield();
      
      DeserializationError err = deserializeJson(jsonDataPoint, f);

      yield();
      if(!err)
      {          
          price_list[i][0] = jsonDataPoint[F("average")];
      }
      else
      {
        parseError = true;
        Serial.printf_P(PSTR("\tInvalid data point i: %d %s\n"), i, err.c_str());
      }
      yield();
      f.find(',');
    }
  }
  else
  {
    printStatusMsg(F("Chart data empty."));
  }

  if(parseError)
  {
    f.seek(0, SeekSet);
    while(f.available())
    {
      Serial.print((char)f.read());
      yield();
    }
    Serial.print('\n');
    //Serial.println(f.readString());
    sinceStockAPIUpdate = MAX_STOCK_API_INTERVAL;
  }
  
  f.close();

  f = SPIFFS.open(KEY_STATS_FILE, "r");
  if(f.size() > 0)
  {
    DynamicJsonDocument jsonObj(400);
    DeserializationError err = deserializeJson(jsonObj, f);
    if(!err)
    {
      yearlow = jsonObj[F("week52low")];
      yearhigh = jsonObj[F("week52high")];
      movingAvg = jsonObj[F("day200MovingAvg")];
    }
    else
    {
      Serial.printf_P(PSTR("\tKey stats error: %s\n"), err.c_str());
      f.seek(0, SeekSet);
      Serial.println(f.readString());
      sinceStockAPIUpdate = MAX_STOCK_API_INTERVAL;
    }
  }
  else
  {
    printStatusMsg(F("Key stats empty."));
  }
  
  f.close();
      
  Serial.println(F("updateChartInfo()...done"));
}

void updateFedInfo(const int max_data_points, const char* file_name)
{
  Serial.println(F("updateFedInfo()..."));

  //clear out memory
  for(int i = 0; i < MAX_STRINGS; i++)
  {
    string_list[i][0] = '\0';
    price_list[i][0] = 0.0f;
  }
  
  File f = SPIFFS.open(file_name, "r");
  Serial.printf_P(PSTR("\tFed file size: %d\n"), f.size());
  
  //Serial.println(f.readString());
  //f.seek(0, SeekSet);
  
  if(f.size() > 0)
  {
    char temp[20] = "";
    const char tok[2] = ",";

    //toss first line header info
    f.readBytesUntil('\n', temp, 20);
    Serial.printf("\tmax_Data_points: %d\n", max_data_points);
    for(int i = 0; f.available() && i < max_data_points; i++)
    {
      yield();
      int size = f.readBytesUntil('\n', temp, 20);
      temp[size] = '\0';
      //Serial.printf("\tupdateFedInfo() i: %d size: %d temp: %s\n", i, size, temp);
      if(size > 10)
      {
        const char *date = strtok(temp, tok);
        const char *price = strtok(NULL, tok);
 
        parseDate(string_list[i], date);
        if(price != NULL)
        {
          price_list[i][0] = atof(price);
        }
        else
        {
          Serial.printf_P(PSTR("\tupdateFedInfo(): invalid data point: %s\n"), temp);
        }
        
        //Serial.printf("\tupdateFedInfo() i: %d date: %s  price: %.1f\n", i, string_list[i], price_list[i][0]);
      }
      else
      {
        Serial.printf_P(PSTR("\tupdateFedInfo(): invalid data point size: %s\n"), temp);
      }
    }
  }
  else
  {
    printStatusMsg(F("Fed data empty."));
  }

  f.close();      
  Serial.println(F("updateFedInfo()...done"));
}

void updateCoinInfo()
{
  Serial.println(F("updateCoinInfo()..."));

  //clear out memory
  for(int i = 0; i < MAX_STRINGS; i++)
  {
    string_list[i][0] = '\0';
    price_list[i][0] = 0.0f; 
  }
  
  File f = SPIFFS.open(COIN_HIST_FILE, "r");
  Serial.printf_P(PSTR("\tCoin chart file size: %d\n"), f.size());
  
  if(f.size() > 0)
  {
    DynamicJsonDocument root(3000);
    DeserializationError err = deserializeJson(root, f);
    if(!err)
    {
      JsonObject bpi = root[F("bpi")];
      
      int i = 0;
      //char debugBuf[100];
      for(JsonObject::iterator it=bpi.begin(); it!=bpi.end() && i < MAX_COINS; ++it)
      {
        yield();
        const char* date = it->key().c_str();
        parseDate(string_list[i], date);
        price_list[i][0] = it->value().as<float>();
        //Serial.printf("\string_list[%d]: %s price_list[%d]: %.2f\n", i, string_list[i], i, price_list[i][0]);
         
        i++;
      }

      //historical api only has yesterday's close, so tack on today's current price from current price api call
      price_list[i][0] = coinprice;
      strcpy(string_list[i], coindate);
      //Serial.printf("\string_list[%d]: %s price_list[%d]: %.2f\n", i, string_list[i], i, price_list[i][0]);
    }
    else
    {
      const char* s  = "coin data error. ";
      Serial.printf_P(PSTR("\t%s %s\n"), s, err.c_str());
      printStatusMsg(s);
      f.seek(0, SeekSet);
      Serial.println(f.readString());
      sinceCoinAPIUpdate = MAX_COIN_API_INTERVAL;
    }
  }
  
  f.close();
  
  Serial.println(F("updateCoinInfo()...done"));
}

void parseDate(char target[], const char *toParse)
{
  //Serial.print("parseDate ");
  int year;
  int month;
  int day;
  int size = strlen(toParse);
  //Serial.printf("strlen: %d ", size);

  if(size >= 10)
  {
    sscanf(toParse, "%d-%d-%d", &year, &month, &day);
    //Serial.printf("\tmonth: %d day: %d ", month, day);
    
    char dateStr [6];
    snprintf(dateStr, 6, "%d/%d", month, day); 
    strlcpy(target, dateStr, 6);
    target[6] = '\0';
  }
  else
  {
    Serial.printf_P(PSTR("\tparseDate: unable to parse: %s\n"), toParse);
  }
  //Serial.println("...done");
}

void printFreeMem(char *str)
{
  char msg[50];
  sprintf(msg, "%s() Free heap: %d", str, ESP.getFreeHeap());
  Serial.println(msg);
}
