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
  root[F("macaddress")] = WiFi.macAddress();
  root[F("version")] = VERSION;
  
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
 
  //PRICING_CHART_URL = "/1.0/stock/market/batch?filter=symbol,latestPrice,changePercent&types=quote&displayPercent=true&symbols=";
  quoteRequestBuffer[0] = '\0';
  
  //insert prefix
  strcpy(quoteRequestBuffer, PRICING_CHART_URL);

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
    strcat(quoteRequestBuffer, IEX_GET_SUFFIX);

    if(!bufferToFile(IEX_HOST, quoteRequestBuffer, PRICING_FILE))
    {
      sinceAPIUpdate = MAX_API_INTERVAL;
    }
  }
  else
  {
    String s  = F("No tickers defined.");
    Serial.println(s);
    printStatusMsg(s);
  }
  
  Serial.println(F("queryPrices()...done")); 
}

void queryChartInfo()
{
  Serial.println(F("queryChartInfo()..."));
  
  //String url = F("GET /1.0/stock/%s/chart/1d?chartInterval=%d&filter=minute,average");
  quoteRequestBuffer[0] = '\0';

  sprintf(quoteRequestBuffer, BASE_CHART_URL, tickers[0], CHART_INTERVAL);

  Serial.print(F("API Chart GET URL: "));
  Serial.println(quoteRequestBuffer);

  //tack on the rest of the HTTP GET request
  strcat(quoteRequestBuffer, IEX_GET_SUFFIX);

  if(!bufferToFile(IEX_HOST, quoteRequestBuffer, CHART_FILE))
  {
    sinceAPIUpdate = MAX_API_INTERVAL;
  }
  
  Serial.println(F("queryChartInfo()...done")); 
}

void checkAvailableFirmwareVersion()
{
  Serial.println(F("checkAvailableFirmwareVersion()..."));

  quoteRequestBuffer[0] = '\0';
  strcat(quoteRequestBuffer, FW_VERSION_URL);
  strcat(quoteRequestBuffer, FW_GET_SUFFIX);

  Serial.print(F("Firmware version GET URL: "));
  Serial.println(quoteRequestBuffer);
  
  bufferToFile(FIRMWARE_HOST, quoteRequestBuffer, FW_REMOTE_VERSION_FILE);
  
  Serial.println(F("checkAvailableFirmwareVersion()...done"));
}


bool bufferToFile(const char *host, const char *buf, const char* filename)
{
  WiFiClientSecure client;
  client.setTimeout(CLIENT_TIMEOUT);
  
  if(client.connect(host, HTTPS_PORT))
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
    Serial.print(F("HTTP response size: "));
    Serial.println(size);
    
    if (size)
    {
      File f = SPIFFS.open(filename, "w");
      f.print(client.readString());
      f.close();
    }
    else
    {
      String s = F("Empty HTTP response.");
      Serial.println(s);
      printStatusMsg(s);
      
      return false;
    }
  }
  else
  {
    Serial.print(F("Remote host unreachable: "));
    Serial.println(host);
    printStatusMsg(F("Remote host unreachable."));
    sinceAPIUpdate = MAX_API_INTERVAL;
    return false;
  }
  return true;
}




