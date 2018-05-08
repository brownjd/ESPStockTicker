int readWifiInfo(char wifis[][2][96])
{
  Serial.println(F("readWifiInfo()..."));
  File f = SPIFFS.open(WIFI_FILE, "r");
  int wifiNo = 0;
  char temp[96] = "";
  const char s[2] = " ";
  
  while(f.available() && wifiNo < MAX_WIFI_NETWORKS)
  {
    int size = f.readBytesUntil('\n', temp, 96);
    if(size > 1)
    {
      char *ssid = strtok(temp, s);
      char *pass = strtok(NULL, s);

      if(ssid != NULL && pass != NULL)
      {
        strncpy(wifis[wifiNo][0], ssid, 32);
        strncpy(wifis[wifiNo][1], pass, 64);
        Serial.print(wifis[wifiNo][0]);
        Serial.print(F(" : "));
        Serial.println(wifis[wifiNo][1]);
        wifiNo++;
      }
    }
    temp[0] = '\0';
  }
  f.close();
  
  Serial.println(F("readWifiInfo()...done"));
  return wifiNo;
}

void writeWifiFile(char wifis[][2][96])
{
  Serial.println(F("writeWifiFile()..."));
  File f = SPIFFS.open(WIFI_FILE, "w");
  for(int i = 0; i < MAX_WIFI_NETWORKS; i++)
  {
    if(strlen(wifis[i][0]) && strlen(wifis[i][1]))
    {
      //don't use println because it includes extra special characters
      f.print(wifis[i][0]);
      f.print(' ');
      f.print(wifis[i][1]);
      Serial.print(wifis[i][0]);
      Serial.print(" : ");
      Serial.println(wifis[i][1]);
    }
    f.print('\n');
  }
  
  f.close();
  Serial.println(F("writeWifiFile()...done"));
}


void readTickerFile()
{
  Serial.println(F("Listing files..."));
  Dir dir = SPIFFS.openDir("/");
  while (dir.next())
  {
    //Serial.println("Filename: %s", dir.fileName());
    File f = dir.openFile("r");
    //Serial.println("File size: %d", f.size());
    //Serial.println(f.readString());
    f.close();
  }
  
  Serial.println(F("Reading ticker file..."));
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
      strncpy(tickers[tickerNo], temp, MAX_TICKER_SIZE);
      Serial.print("Ticker: ");
      Serial.println(tickers[tickerNo]);
      tickerNo++;
    }
    
    temp[0] = '\0';
  }

  f.close();
  Serial.println(F("Reading ticker file...done"));
}

void writeTickerFile()
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
      Serial.print(F("Ticker: "));
      Serial.println(tickers[i]);
    }
  }
  
  f.close();
  Serial.println(F("Writing ticker file...done"));
}

