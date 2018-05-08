void setupWebServer()
{
  httpServer.serveStatic("/", SPIFFS, "/index.html");
  httpServer.serveStatic("/wifi", SPIFFS, "/wifi.html");
  //httpServer.serveStatic("/log", SPIFFS, "/log.txt");
  httpServer.on("/gettickers", getTickers);
  httpServer.on("/settickers", setTickers);
  httpServer.on("/setwifi", setWifi);
  httpServer.on("/getwifi", getWifi);
  httpServer.begin();
}

void setTickers()
{
  Serial.println(F("setTickers()..."));
  Serial.print(F("setTickers: free heap: "));
  Serial.println(ESP.getFreeHeap());
  Serial.print(F("args: "));
  Serial.println(httpServer.args());
  
  String tickerName = "ticker";
  int i;
  for(i = 0; i < httpServer.args() && i < TICKER_COUNT; i++)
  {
    String value = httpServer.arg(i);
    value.toUpperCase();

    //we're expecting an array of values with the same arg name
    if(httpServer.argName(i) == tickerName)
    {  
      tickers[i][0] = '\0';
      strncpy(tickers[i], value.c_str(), MAX_TICKER_SIZE);
    }
  }

  //clear out any empty slots
  for(; i < TICKER_COUNT; i++)
  {
    tickers[i][0] = '\0';
  }

  writeTickerFile();
  httpServer.send(200, F("text/html"), F("<html><head><script>window.location.replace(\"/\");</script></head><body/></html>"));
  //force a refresh
  sinceAPIUpdate = MAX_API_INTERVAL;
  Serial.print(F("setTickers: free heap: "));
  Serial.println(ESP.getFreeHeap());
  Serial.println(F("setTickers()...done"));
}

void getTickers()
{
  Serial.println(F("getTickers()..."));

  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  
  root[F("ip")] = WiFi.localIP().toString();
  root[F("hostname")] = String(hostName);
  
  JsonArray &ticArr = root.createNestedArray("tickers");
  for(int i = 0; i < TICKER_COUNT; i++)
  {
    Serial.print(F("Ticker: "));
    Serial.println(tickers[i]);
    ticArr.add(tickers[i]);
  }

  //check to see if websever implements Print
  String json;
  root.printTo(json);
  
  httpServer.send(200, F("application/json"), json);
  Serial.println(F("getTickers()...done"));
}

void setWifi()
{
  Serial.println(F("setWifi()..."));
  Serial.print(F("num args: "));
  Serial.println(httpServer.args());
  
  char wifis[MAX_WIFI_NETWORKS][2][96];
  int networks = readWifiInfo(wifis);
  
  if(httpServer.hasArg(F("delete")))
  {
    int pos = atoi(httpServer.arg(F("delete")).c_str());
    Serial.print(F("Deleting wifi pos: "));
    Serial.println(pos);
      
    if(pos >= 0 && pos < MAX_WIFI_NETWORKS)
    {
      //clear out that row
      wifis[pos][0][0] = '\0';
      wifis[pos][1][0] = '\0';
    }
  }

  if(httpServer.hasArg(F("ssid")) && 
     httpServer.hasArg(F("pass")) && 
     httpServer.hasArg(F("pos")))
  {

    String ssid = httpServer.arg(F("ssid"));
    String pass = httpServer.arg(F("pass"));
    int pos = atoi(httpServer.arg(F("pos")).c_str());
    
    Serial.print(F("Assigning new ssid for pos: "));
    Serial.println(pos);
    
    if(pos < MAX_WIFI_NETWORKS)
    {
      strncpy(wifis[pos][0], ssid.c_str(), 32);
      strncpy(wifis[pos][1], pass.c_str(), 64); 
    }
  }

  writeWifiFile(wifis);
  httpServer.send(200, "text/html", "<html><head><script>window.location.replace(\"/wifi\");</script></head><body/></html>");
  
  Serial.println("setWifi()...done");
}

void getWifi()
{
  Serial.println(F("getWifi()..."));

  StaticJsonBuffer<500> jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();

  char wifis[MAX_WIFI_NETWORKS][2][96];
  readWifiInfo(wifis);
  
  root[F("MaxWifiNetworks")] = MAX_WIFI_NETWORKS;
  JsonArray &networks = root.createNestedArray(F("networks"));
  for(int i = 0; i < MAX_WIFI_NETWORKS; i++)
  {
    Serial.println(wifis[i][0]);  
    networks.add(wifis[i][0]);
  }

  //check to see if websever implements Print
  String json;
  root.printTo(json);
  
  httpServer.send(200, F("application/json"), json);
  Serial.println(F("getWifi()...done"));
}

void queryPrices()
{
  Serial.println(F("queryPrices()..."));
 
  //BASE_URL = "/1.0/stock/market/batch?filter=symbol,latestPrice,changePercent&types=quote&displayPercent=true&symbols=";
  quoteRequestBuffer[0] = '\0';
  
  //insert prefix
  strcpy(quoteRequestBuffer, BASE_URL);

  bool tickersFound = false;
  //iterate over tickers looking for non-empty entries
  for (int i = 0; i < TICKER_COUNT; i++)
  {
    if (strlen(tickers[i]) > 0)
    {
      tickersFound = true;
      strcat(quoteRequestBuffer, tickers[i]);
      strcat(quoteRequestBuffer, ",");
    }
  }
    
  //we found at least one
  if (tickersFound)
  {
    Serial.print(F("Pricing API GET URL: "));
    Serial.println(quoteRequestBuffer);
   
    //tack on the rest of the HTTP GET request
    strcat(quoteRequestBuffer, HTTP_GET_SUFFIX);

    persistJSONRequest(quoteRequestBuffer, PRICING_FILE);
  }
  else
  {
    String s  = F("No tickers defined.");
    Serial.println(s);
    printStatusMsg(s);
  }
  
  Serial.println(F("queryPrices()...done")); 
}

void updatePrices()
{
  Serial.println(F("updatePrices()..."));
  File f = SPIFFS.open(PRICING_FILE, "r");
  Serial.print(F("Price file size: "));
  Serial.println(f.size());
  Serial.print("Free heap before pricing: ");
  Serial.println(ESP.getFreeHeap());
  DynamicJsonBuffer jsonBuffer(ESP.getFreeHeap() - WORKING_HEAP);
  JsonObject &jsonQuotes = jsonBuffer.parseObject(f);
  Serial.print("Free heap after pricing: ");
  Serial.println(ESP.getFreeHeap());
  //jsonQuotes.printTo(Serial);
  
  for(int tickerNum = 0; tickerNum < TICKER_COUNT; tickerNum++)
  {
    yield();
    char *ticker = tickers[tickerNum];
    prices[tickerNum][0] = 0.0;
    prices[tickerNum][1] = 0.0;
    
    if(ticker[0])
    {
      //Serial.print(F("Retrieving pricing for: "));
      //Serial.println(ticker);
      JsonObject &quoteItem = jsonQuotes[ticker][F("quote")];
      //quoteItem.printTo(Serial);
      if(quoteItem.success())
      {
        const char* symbol = quoteItem[F("symbol")];
        float price = quoteItem[F("latestPrice")];
        float change = quoteItem[F("changePercent")];
    
        prices[tickerNum][0] = price;
        prices[tickerNum][1] = change;
    
        //Serial.print(symbol);
        //Serial.print(F(" "));
        //Serial.print(price);
        //Serial.print(F(" "));
        //Serial.println(change);
      }
      else
      {
        String s  = F("JSON deserialization error.");
        Serial.println(s);
        printStatusMsg(s);
        Serial.print("With ticker: ");
        Serial.println(ticker);
        sinceAPIUpdate = MAX_API_INTERVAL;
        //f.seek(0, SeekSet);
        //Serial.println(f.readString());
      }
    }
  }
  f.seek(0, SeekSet);
  Serial.println(f.readString());
  f.close();
  
  Serial.println(F("updatePrices()...done"));
}

void queryChartInfo()
{
  Serial.println(F("queryChartInfo()..."));
  
  //String url = F("GET /1.0/stock/%s/chart/1d?chartInterval=%d&filter=minute,average");
  quoteRequestBuffer[0] = '\0';

  sprintf(quoteRequestBuffer, BASE_CHART_URL, tickers[0], CHART_INTERVAL);
  //insert prefix
  //strcpy(quoteRequestBuffer, url.c_str());

  Serial.print(F("API Chart GET URL: "));
  Serial.println(quoteRequestBuffer);

  //tack on the rest of the HTTP GET request
  strcat(quoteRequestBuffer, HTTP_GET_SUFFIX);

  persistJSONRequest(quoteRequestBuffer, CHART_FILE);
  
  Serial.println(F("queryChartInfo()...done")); 
}


void updateChartInfo()
{
  Serial.println(F("updateChartInfo()..."));

  File f = SPIFFS.open(CHART_FILE, "r");
  Serial.print(F("Chart file size: "));
  Serial.println(f.size());
  Serial.print(F("Free heap before chart: "));
  Serial.println(ESP.getFreeHeap());
  
  DynamicJsonBuffer jsonBuffer(ESP.getFreeHeap() - WORKING_HEAP);
  JsonArray &jsonChart = jsonBuffer.parseArray(f);
  
  Serial.print(F("Free heap after chart: "));
  Serial.println(ESP.getFreeHeap());
  yield();
  
  if(jsonChart.success())
  {    
    int size = jsonChart.size();
    
    for(int i = 0; i < MAX_CHART_POINTS && i < size; i++)
    {
      yield();
      const char *ticker = tickers[i];
      JsonVariant timeSlot = jsonChart[i];
    
      //const char* symbol = tickerItem["symbol"];
      //String minute = timeSlot[F("minute")];
      float price = timeSlot[F("average")];
  
      chartinfo[i] = price;
  
      //Serial.print(i);
      //Serial.print(F(" "));
      //Serial.print(minute);
      //Serial.print(F(" "));
      //Serial.println(price);
    }
  }
  else
  {
    String s  = F("JSON deserialization error.");
    Serial.println(s);
    printStatusMsg(s);
    sinceAPIUpdate = MAX_API_INTERVAL;
    //f.seek(0, SeekSet);
    //Serial.println(f.readString());
  }
  
  f.seek(0, SeekSet);
  Serial.println(f.readString());
  f.close();
      
  Serial.println(F("updateChartInfo()...done"));
}

bool persistJSONRequest(const char *buf, const char* filename)
{
  WiFiClientSecure client;
  client.setTimeout(CLIENT_TIMEOUT);
  
  if(client.connect(HOST, HTTPS_PORT))
  {
    yield();
      //make the request
    client.print(buf);

    //dispense with the headers we don't want
    while (client.connected())
    {
      yield();
      String line = client.readStringUntil('\n');
      if (line == "\r")
      {
        break;
      }
    }

    //get the data    
    int size = client.available();
    Serial.print(F("API response size: "));
    Serial.println(size);
    
    if (size)
    {
      File f = SPIFFS.open(filename, "w");
      f.print(client.readString());
      f.close();
    }
    else
    {
      String s = F("No data in the API response.");
      Serial.println(s);
      printStatusMsg(s);
      sinceAPIUpdate = MAX_API_INTERVAL;
      return false;
    }
  }
  else
  {
    Serial.print(F("Remote host unreachable: "));
    Serial.println(HOST);
    printStatusMsg(F("Remote host unreachable."));
    sinceAPIUpdate = MAX_API_INTERVAL;
    return false;
  }
  return true;
}



