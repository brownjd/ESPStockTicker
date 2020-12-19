void cleanupDataFiles()
{
  //remove temp data
  SPIFFS.remove(CHART_FILE);
  SPIFFS.remove(PRICING_FILE);
  SPIFFS.remove(KEY_STATS_FILE);
  SPIFFS.remove(TBILL_HIST_FILE);
  SPIFFS.remove(OIL_HIST_FILE);
  SPIFFS.remove(OCTOPI_PRINTER_FILE);
  SPIFFS.remove(OCTOPI_JOB_FILE);
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

void readKeyFile(char key[], const char* keyFile)
{
  Serial.println(F("\tReading Key File..."));
  Serial.printf_P(PSTR("\tKeyFile: %s\n"), keyFile);
  
  File f = SPIFFS.open(keyFile, "r");
  char temp[KEY_LEN] = {""};
  if(f.available())
  {
    int size = f.readBytes(temp, KEY_LEN);
    if(size > 1)
    {
      strlcpy(key, temp, KEY_LEN);
    }
  }
  Serial.printf_P(PSTR("\tKey: %s\n"), key);
  Serial.println(F("\tReading Key File...done"));
}

void writeKeyFile(char iexKey[], const char* keyFile)
{
  Serial.println(F("\Writing Key File..."));
  Serial.printf_P(PSTR("\tKeyFile: %s\n"), keyFile);
  Serial.printf_P(PSTR("\tKey: %s\n"), iexKey);
  File f = SPIFFS.open(keyFile, "w");
  
  int size = f.print(iexKey);
  if(size < KEY_LEN)
  {
    Serial.printf_P(PSTR("\tKey size error: %d\n"), size);
  }
  f.close();
  Serial.println(F("\Writing Key File...done"));
}

void readSettings()
{
  Serial.println(F("\tReading Settings File..."));

  bool settings_exists = true;
  
  File f = SPIFFS.open(SETTINGS_FILE, "r");
  char temp[SETTINGS_NUM] = {""};
  
  if(f.available())
  {
    int size = f.readBytesUntil('\n', temp, SETTINGS_NUM);
    if(size < SETTINGS_NUM)
    {
      Serial.println(F("\tEmpty settings file..."));
      settings_exists = false;
    }
  }
  else
  {
    Serial.println(F("\tNo settings file..."));
    settings_exists = false;
  }

  temp[SETTINGS_NUM] = '\0';

  Serial.printf_P(PSTR("\tSettings: %s\n"), temp);

  for(int i = 0; i < SETTINGS_NUM; i++)
  {
    if(!settings_exists)
    {
      SETTINGS[i] = false;
    }
    else
    {
      SETTINGS[i] = temp[i] == '1' ? true : false;
    }
  }
  
  Serial.println(F("\tReading Settings File...done"));
}

void writeSettings()
{
  Serial.println(F("\Writing Settings File..."));
  File f = SPIFFS.open(SETTINGS_FILE, "w");
  char temp [SETTINGS_NUM];

  for(int i = 0; i < SETTINGS_NUM; i++)
  {
    temp[i] = SETTINGS[i] ? '1' : '0';
  }
  temp[SETTINGS_NUM] = '\0';
  
  Serial.printf_P(PSTR("\tSettings: %s\n"), temp);
  
  int size = f.print(temp);
  if(size < SETTINGS_NUM)
  {
    Serial.printf_P(PSTR("\tSettings size error: %d\n"), size);
  }
  f.print('\n');
  f.close();
  Serial.println(F("\tWriting Setting File...done"));
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
