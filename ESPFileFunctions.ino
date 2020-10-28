void cleanupDataFiles()
{
  //remove temp data
  SPIFFS.remove(CHART_FILE);
  SPIFFS.remove(PRICING_FILE);
  SPIFFS.remove(KEY_STATS_FILE);
  SPIFFS.remove(TBILL_HIST_FILE);
  SPIFFS.remove(OIL_HIST_FILE);
  SPIFFS.remove(FW_REMOTE_VERSION_FILE);
}

int readWifiInfo(char wifis[][2][96])
{
  Serial.println(F("readWifiInfo()..."));
  File f = SPIFFS.open(WIFI_FILE, "r");
  int wifiNo = 0;
  char temp[96] = "";
  const char s[2] = "\t";
  
  while(f.available() && wifiNo < MAX_WIFI_NETWORKS)
  {
    int size = f.readBytesUntil('\n', temp, 96);
    temp[size] = '\0';
    if (size > 1)
    {
      //Serial.printf_P(PSTR("%stemp: %s\n"), ERROR_MSG_INDENT, temp);
      char *ssid = strtok(temp, s);
      char *pass = strtok(NULL, s);

      if(ssid != NULL && pass != NULL)
      {
        //Serial.printf_P(PSTR("%sbefore %s : %s\n"), ERROR_MSG_INDENT, ssid, pass);
        strlcpy(wifis[wifiNo][0], ssid, 32);
        strlcpy(wifis[wifiNo][1], pass, 64);
        //Serial.printf_P(PSTR("%safter  %s : %s\n"), ERROR_MSG_INDENT, wifis[wifiNo][0], wifis[wifiNo][1]);
        wifiNo++;
      }
    }
    temp[0] = '\0';
  }
  f.close();
  int ret = wifiNo;
  
  while(wifiNo < MAX_WIFI_NETWORKS)
  {
    wifis[wifiNo][0][0] = '\0';
    wifis[wifiNo][1][0] = '\0';
    wifiNo++;
  }
  
  Serial.println(F("readWifiInfo()...done"));
  return ret;
}

void writeWifiFile(char wifis[][2][96])
{
  Serial.println(F("writeWifiFile()..."));
  File f = SPIFFS.open(WIFI_FILE, "w");
  for(int i = 0; i < MAX_WIFI_NETWORKS; i++)
  {
    if(strlen(wifis[i][0]) && strlen(wifis[i][1]))
    {
      Serial.printf_P(PSTR("\t%s : %s\n"), wifis[i][0], wifis[i][1]);
      char temp[96] = "";
      strcpy(temp, wifis[i][0]);
      strcat(temp, "\t");
      strcat(temp, wifis[i][1]);
      strcat(temp, "\n");
      //Serial.printf_P(PSTR("%stemp: %s"), ERROR_MSG_INDENT, temp);
      f.print(temp);
    }
  }
  
  f.close();
  Serial.println(F("writeWifiFile()...done"));
}


int readTickerFile(char tickers[][MAX_TICKER_SIZE])
{
  /*
  Serial.println(F("Listing files..."));
  Dir dir = SPIFFS.openDir("/");
  while (dir.next())
  {
    Serial.print("Filename: ");
    Serial.println(dir.fileName());
    File f = dir.openFile("r");
    Serial.print("File size: ");
    Serial.println(f.size());
    Serial.println(f.readString());
    f.close();
    }
  */

  for(int i = 0; i < TICKER_COUNT; i++)
  {
    tickers[i][0] = '\0';
  }
  
  Serial.println(F("\tReading ticker file..."));
  File f = SPIFFS.open(TICKER_FILE, "r");

  int tickerNo = 0;
  char temp[MAX_TICKER_SIZE] = {""};
  
  while(f.available() && tickerNo < TICKER_COUNT)
  {
    int size = f.readBytesUntil('\n', temp, MAX_TICKER_SIZE);
    if(size > 1)
    {
      //strip off the newline
      temp[size] = '\0';
      strlcpy(tickers[tickerNo], temp, MAX_TICKER_SIZE);
      //Serial.printf_P(PSTR("%sTicker: %s\n", tickers[tickerNo]), ERROR_MSG_INDENT);
      tickerNo++;
    }
    
    temp[0] = '\0';
  }

  f.close();
  return tickerNo;
  Serial.println(F("Reading ticker file...done"));
}

void writeTickerFile(char tickers[][MAX_STRING_LEN])
{
  Serial.println(F("Writing ticker file..."));
  File f = SPIFFS.open(TICKER_FILE, "w");
  for(int i = 0; i < TICKER_COUNT; i++)
  {
    if(strlen(tickers[i]) > 0)
    {
      //don't use println because it includes extra special characters
      f.print(tickers[i]);
      f.print('\n');
      Serial.printf_P(PSTR("\tTicker: %s\n"), tickers[i]);
    }
  }
  
  f.close();
  Serial.println(F("Writing ticker file...done"));
}

void readIEXKeyFile(char iexKey[])
{
  Serial.println(F("\tReading IEX Key File..."));
  File f = SPIFFS.open(IEX_KEY_FILE, "r");
  char temp[IEX_KEY_LEN] = {""};
  if(f.available())
  {
    int size = f.readBytes(temp, IEX_KEY_LEN);
    if(size > 1)
    {
      strlcpy(iexKey, temp, IEX_KEY_LEN);
    }
  }
  Serial.printf_P(PSTR("\tKey: %s\n"), iexKey);
  Serial.println(F("\tReading IEX Key File...done"));
}

void writeIEXKeyFile(char iexKey[])
{
  Serial.println(F("\Writing IEX Key File..."));
  Serial.printf_P(PSTR("\tKey: %s\n"), iexKey);
  File f = SPIFFS.open(IEX_KEY_FILE, "w");
  
  int size = f.print(iexKey);
  if(size < IEX_KEY_LEN)
  {
    Serial.printf_P(PSTR("\tKey size error: %d\n"), size);
  }
  f.close();
  Serial.println(F("\Writing IEX Key File...done"));
}

bool compareFWVersion()
{
  Serial.println(F("compareFWVersion..."));
  bool ret = false;
  File remote = SPIFFS.open(FW_REMOTE_VERSION_FILE, "r");
  if(remote.size())
  {
    String rStr = remote.readString();
    float remoteFWVersion = rStr.toFloat();
    Serial.printf_P(PSTR("\tLocal version: %0.2f, remote version: %0.2f\n"), VERSION, remoteFWVersion);

    ret = remoteFWVersion - VERSION >= 0.009f; 
  }
  else
  {
    Serial.println("ERROR: remote file size 0");
  }
  Serial.println(F("compareFWVersion...done"));
  return ret;
}
