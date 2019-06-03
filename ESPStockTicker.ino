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
#include <elapsedMillis.h>

#include "ESPStockTicker.h"
//serve static html from PROGMEM
//so we don't need a separate data files dir
//use node.js gulp to regenerate from html files in data dir
#include "index.html.gz.h"
#include "wifi.html.gz.h"
#include "FS.h"

void setup()
{
  Serial.begin(9600);
  yield();
  delay(2000);
  yield();
  Serial.printf("\n\n*** Last reset reason: %s ***\n\n", ESP.getResetReason().c_str());
  Serial.println(F("setup()...starting"));
  SPIFFS.begin();

  cleanupDataFiles();

  initScreen();
  yield();
  setupIPHandlers();
  yield();
  connectWifi();
  yield();
  setupWebServer();
  yield();
  //setupOTA();
  //yield();
  Serial.println(F("setup()...done"));
}

void loop()
{
  //ArduinoOTA.handle();
  httpServer.handleClient();
  yield();

  if(ipchanged)
  {
    Serial.println("ipchanged...");
    printWifiInfo(true);
    ipchanged = false;
  }
    
  if (WiFi.status() == WL_CONNECTED)
  {
    //not in soft ap mode
    WiFi.softAPdisconnect(true);
    
    if (sinceFWUpdate >= MAX_FW_INTERVAL)
    {
      checkAvailableFirmwareVersion();
      yield();
      if (compareFWVersion())
      {
        printStatusMsg(F("Updating firmware..."));
        if (!ESPhttpUpdate.update(FW_BIN_URL))
        {
          if(ESPhttpUpdate.getLastError() == 11)
          {
            printMsg(F("\n\n\n\n\n Please reboot to update."), true);
          }
          printStatusMsg(F("Failed to update firmware."));
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
      if(SHOW_TBILLS)
      {
        queryFed(FED_HOST, TBILL_URL, TBILL_HIST_FILE);
        yield();
        httpServer.handleClient();
      }
      if(SHOW_BITCOIN)
      {
        queryCoinCurrent();
        yield();
        httpServer.handleClient();
        queryCoinHistorical();
        yield();
        httpServer.handleClient();
        
      }
      if(SHOW_OIL)
      {
        queryFed(FED_HOST, OIL_URL, OIL_HIST_FILE);
        yield();
        httpServer.handleClient();
      }
      sinceAPIUpdate = 0;
    }
    
    if(sincePrint >= MAX_PAGE_INTERVAL)
    {
      Serial.printf("Heap Fragmentation: %d\n", ESP.getHeapFragmentation());
      Serial.printf("Heap MaxFreeBlockSize: %d\n",ESP.getMaxFreeBlockSize());
      
      //page can change sincePrint, so need to do this at the outset
      sincePrint = 0;
      yield();
      httpServer.handleClient();
      displayNextPage();
      yield();
      httpServer.handleClient();
    }
  }

  //we don't want to wait around forever in adhoc mode
  if(WiFi.getMode() == WIFI_AP && sinceAPStart >= MAX_AP_INTERVAL )
  {
    Serial.println(F("Wifi mode is WIFI_AP."));
    Serial.println(F("Rebooting due to time limit for SoftAP."));
    ESP.restart();
  }
}

void updatePrices()
{
  Serial.println(F("updatePrices()..."));

  char tickers[TICKER_COUNT][MAX_TICKER_SIZE];
  readTickerFile(tickers);

  //clear memory first
  for(int tickerNum = 0; tickerNum < TICKER_COUNT; tickerNum++)
  {
    price_list[tickerNum][0] = 0.0;
    price_list[tickerNum][1] = 0.0;
  }

  File f = SPIFFS.open(PRICING_FILE, "r");
  Serial.print(F("Price file size: "));
  Serial.println(f.size());
  if(f.size() > 0)
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
          JsonObject quoteItem = jsonQuotes[ticker]["quote"];      
          price_list[tickerNum][0] = quoteItem[F("latestPrice")];;
          price_list[tickerNum][1] = quoteItem[F("changePercent")];;
        }
      }
    }
    else
    {
      String s  = F("Pricing data error. ");
      Serial.print(s);
      Serial.println(err.c_str());
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
    price_list[i][0] = 0.0f;
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
    DynamicJsonDocument jsonDataPoint(150);
    yield();
    for(int i = 0; i < MAX_CHART_POINTS && f.available(); i++)
    {
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
        String s  = F("Chart data error. ");
        Serial.print(s);
        Serial.println(err.c_str());
        printStatusMsg(s);
      }
      yield();
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
      String s  = F("Key stats error. ");
      Serial.print(s);
      Serial.println(err.c_str());
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

void updateFedInfo(const int max_data_points, const char* file_name)
{
  Serial.println(F("updateFedInfo()..."));

  //clear out memory
  for(int i = 0; i < max_data_points; i++)
  {
    string_list[i][0] = '\0';
    price_list[i][0] = 0.0f;
  }
  
  File f = SPIFFS.open(file_name, "r");
  Serial.print(F("Fed file size: "));
  Serial.println(f.size());
  if(f.size() > 0)
  {
    char temp[20] = "";
    const char tok[2] = ",";

    //toss first line header info
    f.readBytesUntil('\n', temp, 20);
    
    for(int i = 0; f.available(); i++)
    {
      int size = f.readBytesUntil('\n', temp, 20);
      temp[size] = '\0';
      //Serial.println(temp);
      if(size > 1)
      {
        const char *date = strtok(temp, tok);
        const char *yield = strtok(NULL, tok);

        parseDate(string_list[i], date);
        price_list[i][0] = atof(yield); 
        /*
        Serial.print("date: ");
        Serial.print(tbilldates[i]);
        Serial.print(" yield: ");
        Serial.println(tbillyields[i]);
        */
      }
      else
      {
        String s = F("Fed data error.");
        Serial.println(s);
        printStatusMsg(s);
      }
    }
  }
  else
  {
    String s  = F("Fed data empty.");
    Serial.println(s);
    printStatusMsg(s);
  }

  f.close();      
  Serial.println(F("updateTBillInfo()...done"));
}

void updateCoinInfo()
{
  Serial.println(F("updateCoinInfo()..."));

  //clear out memory
  for(int i = 0; i < MAX_COINS; i++)
  {
    string_list[i][0] = '\0';
    price_list[i][0] = 0.0f; 
  }
  
  File f = SPIFFS.open(COIN_HIST_FILE, "r");
  Serial.print(F("Coin chart file size: "));
  Serial.println(f.size());
  
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
        /*
        sprintf(debugBuf, "coindates[%d]: %s coinprices[%d]: %.2f", i, coindates[i], i, coinprices[i]);
        Serial.println(debugBuf); 
        */ 
        i++;
      }

      //historical api only has yesterday's close, so tack on today's current price from current price api call
      price_list[i][0] = coinprice;
      strcpy(string_list[i], coindate);
      /*
      sprintf(debugBuf, "--coindates[%d]: %s coinprices[%d]: %.2f--", i, coindates[i], i, coinprices[i]);
      Serial.println(debugBuf);
      */  
    }
    else
    {
      String s  = F("coin data error. ");
      Serial.print(s);
      Serial.println(err.c_str());
      printStatusMsg(s);
      f.seek(0, SeekSet);
      Serial.println(f.readString());
      sinceAPIUpdate = MAX_API_INTERVAL;
    }
  }
  
  f.close();
  
  Serial.println(F("updateCoinInfo()...done"));
}

void parseDate(char target[], const char *toParse)
{
  int year;
  int month;
  int day;
  sscanf(toParse, "%d-%d-%d", &year, &month, &day);
  /*
  Serial.print("month: ");
  Serial.println(month);
  Serial.print("day: ");
  Serial.println(day);
  */
  char dateStr [6];
  snprintf(dateStr, 6, "%d/%d", month, day); 
  strncpy(target, dateStr, 6);
  target[6] = '\0';
}

void printFreeMem(char *str)
{
  char msg[50];
  sprintf(msg, "%s() Free heap: %d", str, ESP.getFreeHeap());
  Serial.println(msg);
}
