WiFiEventHandler gotIPEventHandler, disconnectedEventHandler;

void updateHostName()
{
  Serial.println(F("updateHostName()..."));
  if(WiFi.status() == WL_CONNECTED)
  {
    printMsg(F("Checking MDNS host names."), true);
    int n = MDNS.queryService(F("http"), F("tcp"));
    
    char testName [20];
    strncpy(testName, hostName, 20);
    
    for(int i = 0; i < n; i++)
    {
      printMsg(MDNS.hostname(i).c_str());
      if((MDNS.hostname(i).equals(testName)))
      {
        sprintf(testName, "%s-%d", testName, i);
      }
    }
    //set our new hostname
    strncpy(hostName, testName, 20);
    Serial.print(F("New hostname: "));
    Serial.println(hostName);
  }
  Serial.println(F("updateHostName()...done"));
}

void setupIPHandlers()
{
  Serial.println(F("setupIPHandlers()..."));
  gotIPEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event)
  {
    (void) event;
    
    Serial.println(F("onStationModeGotIP()..."));
    printWifiInfo(true);
    WiFi.softAPdisconnect(true);
    
    //pretty sure there is an MDNS callback
    //updateHostName();
    MDNS.notifyAPChange();
    Serial.println(F("onStationModeGotIP()...done"));
  });

  disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event)
  {
    (void) event;
    
    Serial.println(F("onStationModeDisconnected.().."));
    printWifiInfo(false);
    MDNS.notifyAPChange();
    Serial.println(F("onStationModeDisconnected()...done"));
  });
  
  Serial.println(F("setupIPHandlers()...done"));
}

void startSoftAP()
{
  Serial.println(F("startSoftAP()..."));
  if(WiFi.status() != WL_CONNECTED)
  {
    WiFi.mode(WIFI_AP);
    //Serial.print(F("Number of softAP connections: "));
    //Serial.println(WiFi.softAPgetStationNum());
      
    Serial.print(F("Starting softAP: "));
    Serial.println(SOFT_AP_NAME);
    
    WiFi.softAP(SOFT_AP_NAME);
    
    Serial.print(F("Soft-AP IPAddress: "));
    Serial.println(WiFi.softAPIP());
  
    char msg [170];
    printMsg(F("Wifi not connected."), true);
    sprintf(msg, "Connect to '%s'", SOFT_AP_NAME);
    printMsg(msg);
    printMsg(F("WIFI network. Then open"));
    sprintf(msg, "http://%s.local/wifi", hostName);
    printMsg(msg);
    printMsg(F("to configure settings."));
  }
  Serial.println(F("startSoftAP()...done"));
}

void connectWifi()
{
  Serial.println(F("connectWifi()..."));
  //clear saved credentials
  
  //[ssid][password]
  // 5 rows with 2 columns (64+32) wide
  char wifis[MAX_WIFI_NETWORKS][2][96];

  int networks = readWifiInfo(wifis);
  if(networks)
  {
    //no point in trying to connect if empty
    char msg [50];
    WiFi.mode(WIFI_STA);
    for(int i = 0; i < MAX_WIFI_NETWORKS && WiFi.status() != WL_CONNECTED; i++)
    {
      if(strlen(wifis[i][0]))
      {
        sprintf(msg, "Connecting to %s", wifis[i][0]);
        printMsg(msg, true);
        Serial.println(msg);
        WiFi.begin(wifis[i][0], wifis[i][1]);
        yield();
        for(int j = 0; (j < MAX_WIFI_TIMEOUT) && (WiFi.status() != WL_CONNECTED); j++) 
        {
          delay(1000);
          printMsg(".");
        }
      }
    }
  }
  setupMDNS();
  startSoftAP();
  Serial.println(F("connectWifi()...done"));
}

void setupMDNS()
{
  Serial.println(F("setupMDNS()..."));
  if(MDNS.begin(hostName))
  {
    //Check if our hostname is already in use
    updateHostName();

    //update wifi
    WiFi.hostname(hostName);

    Serial.print(F("hostname: "));
    Serial.println(hostName);
    
    //delay(2000);
    MDNS.begin(hostName);
    MDNS.addService(F("http"), F("tcp"), HTTP_PORT);
  }
  else
  {
    String s = F("Unable to start mDNS.");
    Serial.println(s);
    printMsg(s);
  }
  Serial.println(F("setupMDNS()...done"));
}

void setupOTA()
{
  ArduinoOTA.onStart([]() 
  {
    Serial.println(F("ArduinoOTA.onstart..."));
    char *type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "Sketch";
    else // U_SPIFFS
      type = "Filesystem";

    char msg [50];
    sprintf(msg, "OTA Updating: %s", type, true);
    printMsg(msg, true);
    Serial.println(F("ArduinoOTA.onstart...done"));
  });

  ArduinoOTA.onEnd([]() 
  {
    printMsg(F("OTA Updating: Done."));
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) 
  {
    int calc = (progress / (total / 100));
    char msg[20];
    if(calc % 10 == 0)
    {
      sprintf(msg, "Progress: %ld", calc);
      printStatusMsg(msg);
    }
  });
  
  ArduinoOTA.onError([](ota_error_t error) 
  {
    String s;
    if (error == OTA_AUTH_ERROR) s = F("OTA Error: Auth Failed");
    else if (error == OTA_BEGIN_ERROR) s = F("OTA Error: Begin Failed");
    else if (error == OTA_CONNECT_ERROR) s = F("OTA Error: Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) s = F("OTA Error: Receive Failed");
    else if (error == OTA_END_ERROR) s = F("OTA Error: End Failed");

    if(s)
    {
      //printMsg(s);
      //Serial.println(s);
    }
  });

  ArduinoOTA.setHostname(hostName);
  ArduinoOTA.begin();
}

