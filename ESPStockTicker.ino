#include <Arduino.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <Adafruit_GFX.h>
#include <elapsedMillis.h>
#include <TimeLib.h>

#include "ESPStockTicker.h"
//serve static html from PROGMEM
//so we don't need a separate data files dir
//use node.js gulp to regenerate from html files in data dir
#include "data/index.html.gz.h"
#include "data/wifi.html.gz.h"
#include "FS.h"

void setup()
{
  Serial.begin(115200);
  yield();
  delay(2000);
  yield();
  Serial.printf_P(PSTR("\n\n*** Last reset reason: %s ***\n\n"), ESP.getResetReason().c_str());
  Serial.println(F("setup()...starting"));
  SPIFFS.begin();

  cleanupDataFiles();

  readSettings();

  initScreen();
  yield();
  connectWifi();
  yield();
  startSoftAP();
  yield();
  setupWebServer();
  yield();
  Serial.println(F("setup()...done"));

  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
}

void loop()
{
  httpServer.handleClient();
  yield();
  startSoftAP();
  yield();
  ArduinoOTA.handle();
  yield();
  httpServer.handleClient();
 
  if(wifiMulti.run() == WL_CONNECTED)
  {
    if(sinceTimeUpdate >= MAX_TIME_INTERVAL)
    {
      if(displayQuery())
      {
        Serial.println(F("\tScreen On"));  
        digitalWrite(2, HIGH);
      }
      else
      {
        Serial.println(F("\tScreen Off"));
        digitalWrite(2, LOW);
      }
      sinceTimeUpdate = 0;
    }
    
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
      if(SETTINGS[SETTINGS_SHARES])
      {
        yield();
        httpServer.handleClient();
        queryPrices();
        
        yield();
        httpServer.handleClient();
        queryChartInfo();
      }
      sinceStockAPIUpdate = 0;
    }
    
    if(sinceFedAPIUpdate >= MAX_FED_API_INTERVAL)
    {
      if(SETTINGS[SETTINGS_TBILLS])
      {
        yield();
        httpServer.handleClient();
        queryFed(FED_HOST, TBILL_URL, TBILL_HIST_FILE);
        
      }

      yield();
      httpServer.handleClient();
      
      if(SETTINGS[SETTINGS_OIL])
      {
        yield();
        httpServer.handleClient();
        queryFed(FED_HOST, OIL_URL, OIL_HIST_FILE);
      }

      sinceFedAPIUpdate = 0;
    }

    if(sinceCoinAPIUpdate >= MAX_COIN_API_INTERVAL)
    {
      if(SETTINGS[SETTINGS_BITCOIN])
      {
        yield();
        httpServer.handleClient();
        if(queryCoinCurrent())
        {
          yield();
          httpServer.handleClient();
          queryCoinHistorical();
        }
        yield();
        httpServer.handleClient();
      }
      
      sinceCoinAPIUpdate = 0;
    }

    if(sinceOctoPiUpdate >= MAX_OCTOPI_API_INTERVAL)
    {
      if(SETTINGS[SETTINGS_PRINT])
      {
        yield();
        httpServer.handleClient();
        queryOctoPiPrinterStatus();
        yield();
        httpServer.handleClient();
        queryOctoPiJobStatus();
        yield();
        httpServer.handleClient();
      }
      
      sinceOctoPiUpdate = 0;
    }

    //page refresh
    if(sincePrint >= MAX_PAGE_INTERVAL)
    {
      printFreeMem("");
      
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
  for(int tickerNum = 0; tickerNum < TICKER_COUNT; tickerNum++)
  {
    price_list[tickerNum][0] = 0.0;
    price_list[tickerNum][1] = 0.0;
    ticker_name_list[tickerNum][0] = '\0';
  }

  File f = SPIFFS.open(PRICING_FILE, "r");
  int size = f.size();
  Serial.printf_P(PSTR("\tPrice file size: %d\n"), size);
  
  if(size > 0)
  {  
    DynamicJsonDocument jsonQuotes(4000);
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
          price_list[tickerNum][0] = quoteItem[F("latestPrice")];
          price_list[tickerNum][1] = quoteItem[F("changePercent")];
          //Serial.printf("Getting company name for: %s\n", ticker);
          const char* name = quoteItem[F("companyName")];
          //Serial.printf("company name is: %s\n", name);
          strncpy(ticker_name_list[tickerNum], name, TICKER_CO_SIZE-1);
          ticker_name_list[tickerNum][TICKER_CO_SIZE] = '\0';
        }
      }
    }
    else
    {
      const char* s  = "Pricing data error.";
      Serial.printf_P(PSTR("%s%s %s\n"), ERROR_MSG_INDENT, s, err.c_str());
      printStatusMsg(s);

      f.seek(0, SeekSet);
      Serial.println(f.readString());
      sinceStockAPIUpdate = MAX_STOCK_API_INTERVAL;
    }
  }
  f.close();
  
  Serial.println(F("updatePrices()...done"));
}

bool updateChartInfo()
{
  Serial.println(F("updateChartInfo()..."));
  bool ret = true;
  
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
      //Serial.printf_P(PSTR("%si: %d\n"), ERROR_MSG_INDENT, i);
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
        Serial.printf_P(PSTR("%sInvalid data point i: %d %s\n"), ERROR_MSG_INDENT, i, err.c_str());
      }
      yield();
      f.find(',');
    }
  }
  else
  {
    ret = false;
    sinceStockAPIUpdate = MAX_STOCK_API_INTERVAL;
    Serial.printf_P(PSTR("%sChart data file size: %d\n"), ERROR_MSG_INDENT, f.size());
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
    ret = false;
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
      Serial.printf_P(PSTR("%sKey stats error: %s\n"), ERROR_MSG_INDENT, err.c_str());
      f.seek(0, SeekSet);
      Serial.println(f.readString());
      sinceStockAPIUpdate = MAX_STOCK_API_INTERVAL;
      ret = false;
    }
  }
  else
  {
    Serial.printf_P(PSTR("%sKey stats file size: %d\n"), ERROR_MSG_INDENT, f.size());
    printStatusMsg(F("Key stats empty."));
    ret = false;
  }
  
  f.close();
  Serial.println(F("updateChartInfo()...done"));
  return ret;
}

bool updateFedInfo(const int max_data_points, const char* file_name)
{
  Serial.println(F("updateFedInfo()..."));
  bool ret = true;
  
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
      //Serial.printf("%supdateFedInfo() i: %d size: %d temp: %s\n", ERROR_MSG_INDENT, i, size, temp);
      if(size > 10)
      {
        const char *date = strtok(temp, tok);
        const char *price = strtok(NULL, tok);
 
        if(parseDate(string_list[i], date))
        {
          if(price != NULL)
          {
            price_list[i][0] = atof(price);
          }
          else
          {
            Serial.printf_P(PSTR("%supdateFedInfo(): invalid data point: %s\n"), ERROR_MSG_INDENT, temp);
            ret = false;
          }
        }
        else
        {
           Serial.printf_P(PSTR("%supdateFedInfo(): unable to parse date: %s"), ERROR_MSG_INDENT, date);
           ret = false;
        }
        
        //Serial.printf("%supdateFedInfo() i: %d date: %s  price: %.1f\n", ERROR_MSG_INDENT, i, string_list[i], price_list[i][0]);
      }
      else
      {
        Serial.printf_P(PSTR("%supdateFedInfo(): invalid data point size: %s\n"), ERROR_MSG_INDENT, temp);
        ret = false;
      }
    }
  }
  else
  {
    Serial.printf_P(PSTR("%sFed data file size: %d\n"), ERROR_MSG_INDENT, f.size());
    printStatusMsg(F("Error: Fed data file."));
    sinceFedAPIUpdate = MAX_FED_API_INTERVAL;
    ret = false;
  }

  f.close();      
  Serial.println(F("updateFedInfo()...done"));
  return ret;
}

bool updateCoinInfo()
{
  Serial.println(F("updateCoinInfo()..."));
  bool ret = true;
  
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
    DynamicJsonDocument doc(3000);
    DeserializationError err = deserializeJson(doc, f);
    if(!err)
    {
      JsonObject bpi = doc[F("bpi")];
      
      int i = 0;
      //char debugBuf[100];
      for(JsonObject::iterator it=bpi.begin(); it!=bpi.end() && i < MAX_COINS; ++it)
      {
        yield();
        const char* date = it->key().c_str();
        if(parseDate(string_list[i], date))
        {
          price_list[i][0] = it->value().as<float>();
        }
        else
        {
          Serial.printf_P(PSTR("%sUpdate coin info date parse error.\n"), ERROR_MSG_INDENT);
        }
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
      sinceCoinAPIUpdate = MAX_COIN_API_INTERVAL;
      ret = false;
      const char* s  = "coin data error. ";
      Serial.printf_P(PSTR("%s%s %s\n"), ERROR_MSG_INDENT, s, err.c_str());
      printStatusMsg(s);
      f.seek(0, SeekSet);
      Serial.println(f.readString());
    }
  }
  
  f.close();
  
  Serial.println(F("updateCoinInfo()...done"));
  return ret;
}

bool updatePrinterStatus(bool *operational, bool *printing, bool *paused, float *bed_temp, float *tool_temp)
{
  Serial.println(F("updatePrinterStatus()..."));
  bool ret = true;
  File f = SPIFFS.open(OCTOPI_PRINTER_FILE, "r");
  Serial.printf_P(PSTR("\tPrinter Status file size: %d\n"), f.size());

  *operational = false;
  *printing = false;
  *paused = false;
  *bed_temp = 0;
  *tool_temp = 0;
  
  if(f.size() > 0)
  {
    DynamicJsonDocument doc(3000);
    DeserializationError err = deserializeJson(doc, f);
    if(!err)
    {
      const char* state_text = doc["state"]["text"]; // "Operational"

      JsonObject temperature_bed = doc["temperature"]["bed"];
      *bed_temp = temperature_bed["actual"]; // 21.25
      //int temperature_bed_offset = temperature_bed["offset"]; // 0
      //int temperature_bed_target = temperature_bed["target"]; // 0

      JsonObject temperature_tool0 = doc["temperature"]["tool0"];
      *tool_temp = temperature_tool0["actual"]; // 21.28
      //int temperature_tool0_offset = temperature_tool0["offset"]; // 0
      //int temperature_tool0_target = temperature_tool0["target"]; // 0

      //Serial.printf("\tPrinter is %s\n\tNozzle temp is %0.1f\n\tBed temp is %0.1f\n", state_text, tool_temp, bed_temp);

      JsonObject state_flags = doc["state"]["flags"];
  
      //bool state_flags_cancelling = state_flags["cancelling"]; // false
      //bool state_flags_closedOrError = state_flags["closedOrError"]; // false
      //bool state_flags_error = state_flags["error"]; // false
      //bool state_flags_finishing = state_flags["finishing"]; // false
      *operational = state_flags["operational"]; // true
      *paused = state_flags["paused"]; // false
      //bool state_flags_pausing = state_flags["pausing"]; // false
      *printing = state_flags["printing"]; // false
      //bool state_flags_ready = state_flags["ready"]; // true
      //bool state_flags_resuming = state_flags["resuming"]; // false
      //bool state_flags_sdReady = state_flags["sdReady"]; // false

    }
    else
    {
      switch (err.code()) {
        case DeserializationError::InvalidInput:
          //printMsg("Printer is not operational.");
          //this is not an error. The api returns a non-json compliant string when it is not operational.
          //ret should be true in this case;
          break;
        case DeserializationError::EmptyInput:
        case DeserializationError::IncompleteInput:
        case DeserializationError::NoMemory:
        case DeserializationError::NotSupported:
        case DeserializationError::TooDeep:
        default:
          printStatusMsg(F("getOctoPiPrinterStatus parse error. "));
          Serial.printf_P(PSTR("%s%s\n"), ERROR_MSG_INDENT, err.c_str());
          sinceFedAPIUpdate = MAX_OCTOPI_API_INTERVAL;
          ret = false;
      }
    }
  }
  else
  {
    sinceOctoPiUpdate = MAX_OCTOPI_API_INTERVAL;
    ret = false;
  }

  Serial.println(F("updatePrinterStatus()...done"));
  return ret;
}

bool updateJobStatus(char* file_name, int file_name_size, float* progress_completion, int* print_time, int* print_time_left)
{
  Serial.println(F("updateJobStatus()..."));
  bool ret = true;
  printStatusMsg("Checking OctoPi Job Status.");

  *progress_completion = 0;
  *print_time = 0;
  *print_time_left = 0;

  File f = SPIFFS.open(OCTOPI_JOB_FILE, "r");
  Serial.printf_P(PSTR("\tPrinter Job Status file size: %d\n"), f.size());
  
  if(f.size() > 0)
  {
    DynamicJsonDocument doc(3000);
    DeserializationError err = deserializeJson(doc, f);
    if(!err)
    {
      JsonObject job = doc["job"];
      JsonObject job_file = job["file"];
      //long job_file_date = job_file["date"]; // 1606792469
      //const char* job_file_display = job_file["display"]; // "Parker_Vector_Barrel - Long_0.15mm_PLA_MINI_37m.gcode"
      
      //const char* job_file_origin = job_file["origin"]; // "local"
      //const char* job_file_path = job_file["path"]; // "Parker_Vector_Barrel_-_Long_0.15mm_PLA_MINI_37m.gcode"
      //long job_file_size = job_file["size"]; // 1770465
      if(!job_file["name"].isNull())
      {
        const char* job_file_name = job_file["name"]; // "Parker_Vector_Barrel_-_Long_0.15mm_PLA_MINI_37m.gcode"
        strncpy(file_name, job_file_name, file_name_size);
      }

      //const char* job_user = job["user"]; // "_api"

      JsonObject progress = doc["progress"];
      *progress_completion = progress["completion"]; // 73.692843405546
      //long progress_filepos = progress["filepos"]; // 1304706
      *print_time = progress["printTime"]; // 1783
      *print_time_left = progress["printTimeLeft"]; // 597
      //const char* progress_printTimeLeftOrigin = progress["printTimeLeftOrigin"]; // "estimate"

      //const char* state = doc["state"]; // "Printing"
      
      ret = true;
    }
    else
    {
      Serial.printf_P(PSTR("%s%s\n"), ERROR_MSG_INDENT, err.c_str());
      switch (err.code()) {
        //case DeserializationError::InvalidInput:
        //case DeserializationError::EmptyInput:
        //case DeserializationError::IncompleteInput:
        //case DeserializationError::NoMemory:
        //case DeserializationError::NotSupported:
        //case DeserializationError::TooDeep:
        //  ret = false;
        //  break;
        default:
          printStatusMsg(F("updateJobStatus parse error. "));
          Serial.printf_P(PSTR("%s%s\n"), ERROR_MSG_INDENT, err.c_str());
          sinceOctoPiUpdate = MAX_OCTOPI_API_INTERVAL;
          ret = false;
          break;
      }
    }
  }
  else
  {
    sinceOctoPiUpdate = MAX_OCTOPI_API_INTERVAL;
    ret = false;
  }

  Serial.println(F("updateJobStatus()...done"));
  return ret;
}

bool parseDate(char target[], const char *toParse)
{
  bool ret = true;
  //Serial.print("parseDate ");
  int year;
  int month;
  int day;
  int size = strlen(toParse);
  //Serial.printf("strlen: %d ", size);

  if(size >= 10)
  {
    sscanf(toParse, "%d-%d-%d", &year, &month, &day);
    //Serial.printf("%smonth: %d day: %d ", ERROR_MSG_INDENT, month, day);
    
    char dateStr [6];
    snprintf(dateStr, 6, "%d/%d", month, day); 
    strlcpy(target, dateStr, 6);
    target[6] = '\0';
  }
  else
  {
    Serial.printf_P(PSTR("%sparseDate: unable to parse: %s\n"), ERROR_MSG_INDENT, toParse);
    return false;
  }
  //Serial.println("...done");
  return ret;
}

void printFreeMem(char *str)
{
  Serial.printf_P(PSTR("%s%sFree heap: %d\n"), ERROR_MSG_INDENT, str, ESP.getFreeHeap());
  Serial.printf_P(PSTR("%s%sHeap Fragmentation: %d\n"), ERROR_MSG_INDENT, str, ESP.getHeapFragmentation());
  Serial.printf_P(PSTR("%s%sHeap MaxFreeBlockSize: %d\n"), ERROR_MSG_INDENT, str, ESP.getMaxFreeBlockSize());
}
