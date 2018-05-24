ST7735_REV tft = ST7735_REV(TFT_CS, TFT_DC, TFT_RST);
int yPos = 0;

void initScreen()
{
  //i should check for a tell-tale register value, but checking for the mac id of the
  //esp it is soldered to will have to suffice.
  Serial.println(WiFi.macAddress());
  
  bool yellow = false;
  for(int i = 0; i < YELLOW_TAB_SIZE; i++)
  {
    if(strcmp(WiFi.macAddress().c_str(), YELLOW_TABS[i]) == 0)
    {
      //means we have a goofed up yellow tab screen that
      //requires special orientation handling
      yellow = true;
      Serial.println("YELLOW_TAB: true");
    }
  }

  // Use this initializer if you're using a 1.8" TFT
  // For some reason, a batch of them I received were printing
  // mirrored backwards text, so I had to special case them 
  // by deriving from Adafruit's ST7735 class. Hence the new
  // YELLOWTAB type and ST7735_REV class 
  (yellow) ? tft.initR(INITR_YELLOWTAB) : tft.initR(INITR_BLACKTAB);

  bool flipped = false;
  for(int i = 0; i < ROTATION_SIZE; i++)
  {
    if(strcmp(WiFi.macAddress().c_str(), ROTATION_OVERRIDES[i]) == 0)
    {
      //means we have a wrisky type screen instead of hitlego
      //instead of orientation 1, we do 3.
      flipped = true;
      Serial.println("FLIPPED: true");
    }
  }
  
  (flipped) ? tft.setRotation(3) : tft.setRotation(1);
  tft.fillScreen(ST7735_BLACK);
  tft.setTextWrap(false);
}

void printMsg(String msg)
{
  printMsg(msg, 0, yPos, false);
  yPos += STATUS_MSG_HEIGHT * MSG_LINE_SPACING; //16 for textSize 1
  if(yPos >= SCREEN_WIDTH) yPos = 0;
}

void printMsg(String msg, bool clrScreen)
{
  printMsg(msg, 0, 0, clrScreen);
  yPos = STATUS_MSG_HEIGHT * MSG_LINE_SPACING; //16 for textSize 1
}

void printMsg(String msg, int x, int y, bool clrScreen)
{
  if(clrScreen)
    tft.fillScreen(ST7735_BLACK);
  tft.setTextWrap(true);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.setFont();

  tft.fillRect(0, y-5, SCREEN_WIDTH, 15, ST7735_BLACK);
  tft.setCursor(0, y);
  tft.print(msg);
  tft.setTextWrap(false);
}

void printStatusMsg(String msg, bool clrScreen)
{
  printMsg(msg, 0, SCREEN_HEIGHT - STATUS_MSG_HEIGHT, clrScreen);
}

void printStatusMsg(String msg)
{
  printStatusMsg(msg, false);
}

void printWifiInfo()
{
  printWifiInfo(false);
}

void printWifiInfo(bool clrScreen)
{
  if(WiFi.status() == WL_CONNECTED)
  {
    char msg [strlen(hostName) + 20];

    sprintf(msg, "V%.2f  http://%s.local  ", VERSION, hostName);
    printStatusMsg(msg, clrScreen);
  }
  else
  {
    printStatusMsg(F("Wifi disconnected"), clrScreen);
  }
}

void printTickers()
{
  //Serial.println(F("printTickers()..."));
  printWifiInfo();
  Serial.print(F("Page: "));
  Serial.println(page);

  page = (page < 0 ) ? 0 : page;
  
  yield();
  
  int rowToPrint = page * DISPLAY_ROWS;
  
  if(rowToPrint > TICKER_COUNT)
  {
    Serial.println("rowToPrint > TICKER_COUNT - this shouldn't happen.");
    //we've got past the end. Reset to beginning
    rowToPrint = 0;
    page = 0;
  }

  if(page == 0)
  {
    //only update on page 0
    updatePrices();
    yield();
    updateChartInfo();
    yield();
  }

  int lastRowToPrint = (page + 1) * DISPLAY_ROWS;

  //handle the case where we might print a partial page if total rows isn't easily divisible by pages
  lastRowToPrint = (lastRowToPrint > TICKER_COUNT) ? TICKER_COUNT : lastRowToPrint;

  //now make sure we're not printing past the last calculated display row
  if(rowToPrint >= lastRowToPrint)
  {
    //means we reached the end, display the chart and reset
    printChart();
    rowToPrint = 0;
    page = 0;
    lastRowToPrint =  DISPLAY_ROWS;
  }
  else
  {
    while(rowToPrint < TICKER_COUNT && rowToPrint < lastRowToPrint) 
    {
      printTicker(rowToPrint);
      rowToPrint++;
    }
    page++;
  }
  //Serial.println(F("printTickers()...done"));
}

bool printTicker(int tickerNum)
{
  //Serial.println(F("printTicker()..."));
  //Serial.print(F("Ticker no: "));
  //Serial.println(tickerNum);
  bool ret = false;
  
  if(tickerNum < TICKER_COUNT)
  {
    const char *ticker = tickers[tickerNum];
  
    int row = tickerNum % DISPLAY_ROWS;
  
    int boxYPos = row * ROW_HEIGHT-2;
    int fontYPos = (boxYPos+ROW_HEIGHT-4);
  
    tft.setTextWrap(false); 
    tft.setFont(&Monospaced_plain_11);
    tft.setTextColor(ST7735_WHITE);
    //slight adjustment to tighten box depending on font size
    tft.fillRect(0, boxYPos, SCREEN_WIDTH, ROW_HEIGHT, ST7735_BLACK); 
  
    if(strlen(ticker) > 0)
    {
      double price = prices[tickerNum][0];
      double change = prices[tickerNum][1];

      int triangleBase = -100;
      int triangleTop = -100;
      int triangleColor = ST7735_GREY;
      
      if(change > 0.0)
      {
        triangleBase = fontYPos - 2;
        triangleTop = fontYPos - 6;
        triangleColor = ST7735_GREEN;
        
      }
      else if(change < 0.0) 
      {
        triangleBase = fontYPos - 6;
        triangleTop = fontYPos - 2;
        triangleColor = ST7735_RED;
      }

      tft.setCursor(COLUMN_0, fontYPos); 
      tft.print(ticker);

      tft.setCursor(COLUMN_1, fontYPos);
      tft.print(price);

      tft.fillTriangle(COLUMN_2, triangleBase, COLUMN_2+4, triangleTop, COLUMN_2+8, triangleBase, triangleColor);

      tft.setCursor(COLUMN_3, fontYPos);    
      tft.print(fabs(change));
      
      tft.setCursor(COLUMN_4, fontYPos);
      tft.print("%");
      
      ret = true;
    }
  }
  
  //Serial.println(F("printTicker()...done"));
  return ret;
}

void printChart()
{
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  
  //figure out min and max prices
  float low = yearlow;
  float high = yearhigh;
  for(int i = 0; i < MAX_CHART_POINTS; i++)
  {
    float price = chartinfo[i];
    if(price > 0)
    { 
      if(low == -1 ) low = price;
      if(high == -1) high = price;
      
      if(price < low) low = price;
      if(price > high) high = price;
    }
  }

  float spread = (high - low) * .20;
  high += spread;
  low -= spread;

  /*
  Serial.print("low: ");
  Serial.print(low);
  Serial.print(" high: ");
  Serial.println(high);
  */
  
  //hour label Y position
  int textPosY = CHART_Y_ORIGIN + CHART_Y_HEIGHT + 5;
  
  //draw the vertical lines
  //9:30 left line
  tft.drawFastVLine(CHART_X_ORIGIN, CHART_Y_ORIGIN, CHART_Y_HEIGHT, ST7735_GREY);
  //10 - 4 pm
  int hour = 10;
  
  for(int x = CHART_X_ORIGIN + (CHART_X_SPACING/2); x <= CHART_X_WIDTH + CHART_X_ORIGIN; x += CHART_X_SPACING)
  {
    tft.drawFastVLine(x, CHART_Y_ORIGIN, CHART_Y_HEIGHT, ST7735_GREY);
    int posX = (hour >= 10) ? x-4 : x-3;
    tft.setCursor(posX, textPosY);
    tft.print((hour > 12) ? hour - 12 : hour);
    hour++;
  }

  char priceShort [] = "%.0f";
  char priceLong [] = "%.1f";
  
  char label [10];
  //draw the horizontal lines and price labels
  for(int y = CHART_Y_ORIGIN; y <= CHART_Y_HEIGHT + CHART_Y_ORIGIN; y += CHART_Y_SPACING)
  {
    //Serial.print("horizontal: ");
    //Serial.println(y);
    tft.drawFastHLine(CHART_X_ORIGIN, y, CHART_X_WIDTH, ST7735_GREY);
    float val = mmap(CHART_Y_HEIGHT - y, CHART_Y_ORIGIN, CHART_Y_HEIGHT, low, high);
    //Serial.print("label: ");
    //Serial.println(val);
    tft.setCursor(0, y-3);
    if(val < 1000)
      sprintf(label, priceLong, val);
    else
      sprintf(label, priceShort, val);
    tft.print(label);
  }

  //print the moving avg
  float scaledAvg = CHART_Y_HEIGHT - mmap(movingAvg, low, high, CHART_Y_ORIGIN, CHART_Y_HEIGHT); 
  tft.drawFastHLine(CHART_X_ORIGIN, scaledAvg, CHART_X_WIDTH, ST7735_DIMYELLOW);

  //52 week low
  float scaledLow = CHART_Y_HEIGHT - mmap(yearlow, low, high, CHART_Y_ORIGIN, CHART_Y_HEIGHT); 
  tft.drawFastHLine(CHART_X_ORIGIN, scaledLow, CHART_X_WIDTH, ST7735_WHITE);

  //52 week high
  float scaledHigh = CHART_Y_HEIGHT - mmap(yearhigh, low, high, CHART_Y_ORIGIN, CHART_Y_HEIGHT); 
  tft.drawFastHLine(CHART_X_ORIGIN, scaledHigh, CHART_X_WIDTH, ST7735_WHITE);

  //print the ticker
  int16_t x = CHART_X_ORIGIN + CHART_X_WIDTH - (CHART_X_SPACING*1.75);
  int16_t y = CHART_Y_ORIGIN + (CHART_Y_SPACING/1.25);
  int16_t xbounds;
  int16_t ybounds;
  uint16_t width;
  uint16_t height;

  tft.setFont(&Monospaced_plain_11); 
  tft.setTextColor(ST7735_WHITE);
  tft.getTextBounds(tickers[0], x, y, &xbounds, &ybounds, &width, &height);
  tft.fillRect(xbounds-2, ybounds-2, width+3, height+3, ST7735_BLACK);
  tft.setCursor(x, y);
  tft.print(tickers[0]);

  //grab first chart data point
  float price0 = chartinfo[0];
  int x0 = mmap(0, 0, MAX_CHART_POINTS, CHART_X_ORIGIN, CHART_X_ORIGIN+CHART_X_WIDTH);
  float scaled0 = CHART_Y_HEIGHT - mmap(price0, low, high, CHART_Y_ORIGIN, CHART_Y_HEIGHT);
  
  for(int i = 1; i < MAX_CHART_POINTS - 1; i++)
  {
    yield();
    //we need two points to draw a line
    if(price0 > 0)
    {
      float price1 = chartinfo[i+1];
      float scaled1 = CHART_Y_HEIGHT - mmap(price1, low, high, CHART_Y_ORIGIN, CHART_Y_HEIGHT);

      /*
      Serial.print(i);
      Serial.print(" ");
      Serial.print(" price0: ");
      Serial.print(price0);
      Serial.print(" scaled0: ");
      Serial.print(scaled0);
      Serial.print(" price1: ");
      Serial.print(price1);
      Serial.print(" scaled1: ");
      Serial.print(scaled1);
      Serial.print(" movingAvg: ");
      Serial.print(movingAvg);
      Serial.print(" scaledAvg: ");
      Serial.println(scaledAvg);
      */
      
      if(price1 > 0)
      {
        int x1 = mmap(i+1, 0, MAX_CHART_POINTS, CHART_X_ORIGIN, CHART_X_ORIGIN+CHART_X_WIDTH);
        if(price0 > movingAvg && price1 > movingAvg)
        {
          tft.drawLine(x0, scaled0, x1, scaled1, ST7735_GREEN);
        }
        else
        {
          tft.drawLine(x0, scaled0, x1, scaled1, ST7735_RED);
        }
        price0 = price1;
        scaled0 = scaled1;
        x0 = x1;
      }
    }
  }
  printWifiInfo(false); 
}

double mmap(double x, double in_min, double in_max, double out_min, double out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

