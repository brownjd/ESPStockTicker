void setupWebServer()
{
  httpServer.on(F("/"), root);
  httpServer.on(F("/wifi"), wifi);
  httpServer.on(F("/gettickers"), getTickers);
  httpServer.on(F("/settickers"), setTickers);
  httpServer.on(F("/setwifi"), setWifi);
  httpServer.on(F("/getwifi"), getWifi);
  httpServer.begin();
}

//This is so cool
void root()
{
  httpServer.sendHeader(F("Content-Encoding"), F("gzip"), true);
  httpServer.send_P(200, "text/html", index_html_gz, index_html_gz_len);
}

void wifi()
{
  httpServer.sendHeader(F("Content-Encoding"), F("gzip"), true);
  httpServer.send_P(200, "text/html", wifi_html_gz, wifi_html_gz_len);
}


void setTickers()
{
  Serial.println(F("setTickers()..."));
  Serial.printf_P(PSTR("\targs: %d\n"), httpServer.args());
  
  SHOW_BITCOIN = String(F("true")).equals(httpServer.arg(F("bitcoin")));
  SHOW_TBILLS = String(F("true")).equals(httpServer.arg(F("tbills")));
  SHOW_OIL = String(F("true")).equals(httpServer.arg(F("oil")));

  char tickers[TICKER_COUNT][MAX_STRING_LEN];
  
  String tickerName = "ticker";
  int ticker_count = 0;
  for(int i = 0; i < httpServer.args(); i++)
  {
    String value = httpServer.arg(i);
    value.toUpperCase();
    value.trim();

    //we're expecting an array of values with the same arg name
    if(httpServer.argName(i) == tickerName)
    {  
      tickers[ticker_count][0] = '\0';
      strlcpy(tickers[ticker_count], value.c_str(), MAX_TICKER_SIZE);
      ticker_count++;
    }
  }

  //clear out any empty slots
  for(; ticker_count < TICKER_COUNT; ticker_count++)
  {
    tickers[ticker_count][0] = '\0';
  }

  writeTickerFile(tickers);
  httpServer.send(200, F("text/html"), F("<html><head><script>window.location.replace(\"/\");</script></head><body/></html>"));
  
  //force a refresh
  //2 second delay is a workaround for https://github.com/esp8266/Arduino/issues/2734
  sinceStockAPIUpdate = MAX_STOCK_API_INTERVAL - 2000;
 
  Serial.println(F("setTickers()...done"));
}

void getTickers()
{
  Serial.println(F("getTickers()..."));

  DynamicJsonDocument doc(1000);
  
  doc[F("ip")] = WiFi.localIP().toString();
  doc[F("hostname")] = String(hostName);
  doc[F("macaddress")] = WiFi.macAddress();
  doc[F("version")] = VERSION;
  doc[F("bitcoin")] = SHOW_BITCOIN;
  doc[F("tbills")] = SHOW_TBILLS;
  doc[F("oil")] = SHOW_OIL;

  char tickers[TICKER_COUNT][MAX_TICKER_SIZE];
  int num_tickers = readTickerFile(tickers);
  
  JsonArray ticArr = doc.createNestedArray(F("tickers"));
  for(int i = 0; i < TICKER_COUNT; i++)
  {
    Serial.printf_P(PSTR("\tTicker: %s\n"), tickers[i]);
    ticArr.add(tickers[i]);
  }

  //check to see if websever implements Print
  String json;
  serializeJson(doc, json);
  
  httpServer.send(200, F("application/json"), json);
  Serial.println(F("getTickers()...done"));
}

void setWifi()
{
  Serial.println(F("setWifi()..."));
  Serial.printf_P(PSTR("\tnum args: %d\n"), httpServer.args());
  
  char wifis[MAX_WIFI_NETWORKS][2][96];
  int networks = readWifiInfo(wifis);
  
  if(httpServer.hasArg(F("delete")))
  {
    int pos = atoi(httpServer.arg(F("delete")).c_str());
    Serial.printf_P(PSTR("\tDeleting wifi pos: %d"), pos);
      
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
    
    Serial.printf_P(PSTR("\tAssigning new ssid for pos: %d %s : %s\n"), pos, ssid.c_str(), pass.c_str());
    
    if(pos < MAX_WIFI_NETWORKS)
    {
      strlcpy(wifis[pos][0], ssid.c_str(), 32);
      strlcpy(wifis[pos][1], pass.c_str(), 64); 
      Serial.printf_P(PSTR("\tAssigned new ssid for pos: %d %s : %s\n"), pos, wifis[pos][0], wifis[pos][1]);
    }
  }

  writeWifiFile(wifis);
  httpServer.send_P(200, PSTR("text/html"), PSTR("<html><head><script>window.location.replace(\"/wifi\");</script></head><body/></html>"));
  Serial.println(F("setWifi()...done"));
}

void getWifi()
{
  Serial.println(F("getWifi()..."));

  DynamicJsonDocument root(500);

  char wifis[MAX_WIFI_NETWORKS][2][96];
  readWifiInfo(wifis);
  
  root[F("MaxWifiNetworks")] = MAX_WIFI_NETWORKS;
  JsonArray networks = root.createNestedArray("networks");
  for(int i = 0; i < MAX_WIFI_NETWORKS; i++)
  {
    if(strlen(wifis[i][0]))
    {
      Serial.printf_P(PSTR("\tgetWifi: %s : %s\n"), wifis[i][0], wifis[i][1]);  
      networks.add(wifis[i][0]);
    }
  }

  //check to see if websever implements Print
  String json;
  serializeJson(root, json);
  
  httpServer.send(200, F("application/json"), json);
  Serial.println(F("getWifi()...done"));
}

void queryPrices()
{
  Serial.println(F("queryPrices()..."));
  printStatusMsg("Updating prices.");

  char tickers[TICKER_COUNT][MAX_TICKER_SIZE];
  readTickerFile(tickers);
  
  //PRICING_CHART_URL = "/1.0/stock/market/batch?filter=symbol,latestPrice,changePercent&types=quote&displayPercent=true&symbols=";
  requestBuffer[0] = '\0';

  char tickerBuf[150] = {""}; 
  
  bool tickersFound = false;
  //iterate over tickers looking for non-empty entries
  for (int i = 0; i < TICKER_COUNT; i++)
  {
    if (strlen(tickers[i]) > 0)
    {
      tickersFound = true;
      strcat(tickerBuf, tickers[i]);
      strcat(tickerBuf, ",");
    }
  }
    
  //we found at least one
  if (tickersFound)
  {
    sprintf(requestBuffer, PRICING_CHART_URL, tickerBuf);
    
    //Serial.printf_P(PSTR("\tPricing API GET URL: %s\n"), requestBuffer);
   
    if(!bufferToFile(IEX_HOST, requestBuffer, PRICING_FILE))
    {
      sinceStockAPIUpdate = MAX_STOCK_API_INTERVAL;
    }
  }
  else
  {
    printStatusMsg(F("No tickers defined."));
  }
  
  Serial.println(F("queryPrices()...done")); 
}

void queryChartInfo()
{
  Serial.println(F("queryChartInfo()..."));
  printStatusMsg("Updating chart data.");
  //String url = F("GET /1.0/stock/%s/chart/1d?chartInterval=%d&filter=minute,average");
  requestBuffer[0] = '\0';

  char tickers[TICKER_COUNT][MAX_TICKER_SIZE];
  readTickerFile(tickers);

  sprintf(requestBuffer, BASE_CHART_URL, tickers[0], CHART_INTERVAL);

  //Serial.printf_P(PSTR("\tAPI Chart GET URL: %s\n"), requestBuffer);

  if(!bufferToFile(IEX_HOST, requestBuffer, CHART_FILE))
  {
    sinceStockAPIUpdate = MAX_STOCK_API_INTERVAL;
  }

  requestBuffer[0] = '\0';

  sprintf(requestBuffer, KEY_STATS_URL, tickers[0]);

  //Serial.printf_P(PSTR("\tAPI Key Stats Average GET URL: %s\n"), requestBuffer);

  if(!bufferToFile(IEX_HOST, requestBuffer, KEY_STATS_FILE))
  {
    sinceStockAPIUpdate = MAX_STOCK_API_INTERVAL;
  }
  
  Serial.println(F("queryChartInfo()...done")); 
}

void queryFed(const char* host, const char* url, const char *file_name)
{  
  Serial.println(F("queryFed()..."));
  printStatusMsg(F("Updating fed data."));
  
  //String url = "https://fred.stlouisfed.org/graph/fredgraph.csv?mode=fred&id=DCOILWTICO&cosd=2014-05-20&fq=Daily"

  //Serial.printf_P(PSTR("\tTime GET URL: %s\n"), TIME_URL);
  
  WiFiClient client;
  if(getConnection(&client, TIME_HOST, HTTP_PORT, TIME_URL))
  {
    DynamicJsonDocument jsonDoc(400);
    DeserializationError err = deserializeJson(jsonDoc, client);
    if(!err)
    {
      const char *dateTimeStr = jsonDoc[F("currentDateTime")];
      
      int year;
      char dateStr [11];

      //beginning of the current year
      //snprintf(dateStr, 11, "%d-%02d-%s", year, month, "01");
      sscanf(dateTimeStr, "%d-", &year);
      snprintf(dateStr, 11, "%d-%s-%s", year, "01", "01"); 

      //create tbill url with proper begin date
      requestBuffer[0] = '\0';
      sprintf(requestBuffer, url, dateStr);

      //Serial.printf_P(PSTR("\tOil GET URL: %s\n"), requestBuffer);
           
      if(!bufferToFile(host, requestBuffer,file_name))
      {
        sinceFedAPIUpdate = MAX_FED_API_INTERVAL;
      }
    }
    else
    {      
      printStatusMsg(F("Time parse error. "));
      Serial.printf_P(PSTR("\t%s\n"), err.c_str());
      sinceFedAPIUpdate = MAX_FED_API_INTERVAL;
    }
  }
  
  Serial.println(F("queryFed()...done"));
}

void queryCoinHistorical()
{  
  Serial.println(F("queryCoinHistorical()..."));
  printStatusMsg("Updating bitcoin hist data.");
  
  //const char* COIN_HIST_URL = "https://api.coindesk.com/v1/bpi/historical/close.json HTTP/1.0\r\nHost: api.coindesk.com\r\nUser-Agent: ESP8266\r\nConnection: close\r\n\r\n";

  //Serial.printf_P(PSTR("\tCOIN Historical GET URL: %s\n"), COIN_HIST_URL);
           
  if(!bufferToFile(COIN_HOST, COIN_HIST_URL, COIN_HIST_FILE))
  {
    Serial.print(F("\tBC hist error."));
    sinceCoinAPIUpdate = MAX_COIN_API_INTERVAL;
  }
  
  Serial.println(F("queryCoinHistorical()...done"));
}

void queryCoinCurrent()
{  
  Serial.println(F("queryCoinCurrent()..."));
  printStatusMsg("Updating bitcoin data.");
  
  //const char* COIN_CURR_URL = "https://api.coindesk.com/v1/bpi/currentprice/USD.json HTTP/1.0\r\nHost: api.coindesk.com\r\nUser-Agent: ESP8266\r\nConnection: close\r\n\r\n";

  //Serial.printf_P(PSTR("\tBitCoin GET URL: %s\n"), COIN_CURR_URL);
  
  WiFiClientSecure client;
  //FIX suggested by https://github.com/esp8266/Arduino/issues/4826#issuecomment-491813938 that worked. Seems like a bug to me.
  client.setInsecure();
  if(getConnection(&client, COIN_HOST, HTTPS_PORT, COIN_CURR_URL))
  {
    DynamicJsonDocument root(1000);
    DeserializationError err = deserializeJson(root, client);
    
    if(!err)
    {
      JsonObject time = root[F("time")];
      parseDate(coindate, time[F("updatedISO")]);
      
      JsonObject bpiUSD = root[F("bpi")][F("USD")];
      coinprice = bpiUSD[F("rate_float")];
      
      //Serial.println(coindate);
      //Serial.println(coinprice);
      
    }
    else
    {
      Serial.printf_P(PSTR("\tBT curr error: %s\n"), err.c_str());
      sinceCoinAPIUpdate = MAX_COIN_API_INTERVAL;
    }
  }
  
  Serial.println(F("queryCoinCurrent()...done"));
}

void checkAvailableFirmwareVersion()
{
  Serial.println(F("checkAvailableFirmwareVersion()..."));
  printStatusMsg(F("Checking for updates."));
  requestBuffer[0] = '\0';
  strcat(requestBuffer, FW_VERSION_URL);
  strcat(requestBuffer, FW_GET_SUFFIX);

  //Serial.printf_P(PSTR("\tFirmware version GET URL: %s\n"), requestBuffer);
  
  bufferToFile(FIRMWARE_HOST, requestBuffer, FW_REMOTE_VERSION_FILE);
  
  Serial.println(F("checkAvailableFirmwareVersion()...done"));
}

void updateFirmware()
{
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

bool bufferToFile(const char *host, const char *buf, const char* filename)
{
  //Serial.println(F("bufferToFile()..."));
  
  WiFiClientSecure client;
  //FIX suggested by https://github.com/esp8266/Arduino/issues/4826#issuecomment-491813938 that worked. Seems like a bug to me.
  client.setInsecure();
  getConnection(&client, host, HTTPS_PORT, buf);

  bool ret = client.available() ? true: false;
  
  File f = SPIFFS.open(filename, "w");
  //Serial.println("#######################");
  while(client.available())
  {
    yield();
    //int c = client.read();
    //Serial.print((char)c);
    //f.write(c);
    f.write(client.read());
  }
  //Serial.println("#######################");
  f.close();

  //Serial.println(F("bufferToFile()...done"));
  return ret;
}

bool getConnection(WiFiClient *client, const char *host, const int port, const char *buf)
{ 
  //Serial.println(F("getConnection()..."));
  bool ret = true;
  client->setTimeout(CLIENT_TIMEOUT);
  
  yield();
  if(client->connect(host, port))
  {
    yield();
    //make the request
    client->print(buf);

    //from https://arduinojson.org/v5/example/http-client/
    // Check HTTP status
    char status[32] = {0};
    client->readBytesUntil('\r', status, sizeof(status));
    if (! (strcmp(status, "HTTP/1.0 200 OK") || (strcmp(status, "HTTP/1.1 200 OK"))))
    {
      Serial.printf_P(PSTR("\tUnexpected response: %s\n"), status);
      return false;
    }

    // Skip HTTP headers
    char endOfHeaders[] = "\r\n\r\n";
    if (!client->find(endOfHeaders)) 
    {
      Serial.println(F("Invalid response"));
      return false;
    }
  }
  else
  {
    Serial.printf_P(PSTR("\tRemote host unreachable: %s : %d\n"), host, port);
    printStatusMsg(F("Remote host unreachable."));
    ret = false;
  }
  
  //Serial.println(F("getConnection()...done"));
  return ret;
}
