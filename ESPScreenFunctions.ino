ST7735_REV tft = ST7735_REV(TFT_CS, TFT_DC, TFT_RST);
int yPos = 0;

void initScreen()
{
  //i should check for a tell-tale register value, but checking for the mac id of the
  //esp it is soldered to will have to suffice.
  //Serial.println(WiFi.macAddress());
  
  bool yellow = false;
  for(int i = 0; i < YELLOW_TAB_SIZE; i++)
  {
    if(strcmp(WiFi.macAddress().c_str(), YELLOW_TABS[i]) == 0)
    {
      //means we have a goofed up yellow tab screen that
      //requires special orientation handling
      yellow = true;
      //Serial.println("YELLOW_TAB: true");
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
      //Serial.println("FLIPPED: true");
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

  httpServer.handleClient();
  
  if(page == 0)
  {
    //only update on page 0
    updatePrices();
  }
  
  else if(page == 2)
  {
    updateChartInfo();
    yield();
    httpServer.handleClient();
    printChart();
    page++;
  }

  //tbill page
  else if(page == 3)
  {
    updateTBillInfo();
    yield();
    httpServer.handleClient();
    printTBill();
    page++;
  }

  //coin page
  else if(page == 4)
  {
    updateCoinInfo();
    yield();
    httpServer.handleClient();
    printCoins();
    page++;
  }

  httpServer.handleClient();  
  yield();

  if(page < 2)
  {
    int rowToPrint = page * DISPLAY_ROWS;
    
    if(rowToPrint > TICKER_COUNT)
    {
      Serial.println("rowToPrint > TICKER_COUNT - this shouldn't happen.");
      //we've got past the end. Reset to beginning
      rowToPrint = 0;
      page = 0;
    }

    int lastRowToPrint = (page + 1) * DISPLAY_ROWS;

    //handle the case where we might print a partial page if total rows isn't easily divisible by pages
    lastRowToPrint = (lastRowToPrint > TICKER_COUNT) ? TICKER_COUNT : lastRowToPrint;

    bool printed = false;
    while(rowToPrint < TICKER_COUNT && rowToPrint < lastRowToPrint) 
    {
      if(printTicker(rowToPrint)) printed = true;
      rowToPrint++;
    }
    page++;
    //we printed an empty page, so flip to next;
    if(!printed) sincePrint = MAX_PRINT_INTERVAL;
  }

  //last page, loop
  if(page == 5)
  {
    page = 0;
  }
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
  ///Testing
  /*
  //clear out memory
  for(int i = 0; i < MAX_CHART_POINTS; i++)
  {
    chartinfo[i] = 0.0f;
  }

  strcpy(tickers[0], "TEST");
 
  chartinfo[0] = 1.0f;
  chartinfo[1] = 1.25f;
  chartinfo[2] = 1.5f;
  chartinfo[3] = 1.4f;
  chartinfo[4] = 1.2f;
  chartinfo[5] = 4.0f;

  yearlow = 0.50f;
  yearhigh = 4.0f;
  movingAvg = 1.25f;
  */
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);

  //draw the horizontal lines and price labels
  const int yMin = CHART_Y_ORIGIN;
  const int yMax = CHART_Y_ORIGIN + CHART_Y_HEIGHT;
  const int xMin = CHART_X_ORIGIN;
  const int xMax = CHART_X_ORIGIN + CHART_X_WIDTH;
  const int wid = CHART_X_WIDTH;
  const int len = CHART_Y_HEIGHT;
  
  //hour label Y position
  int textPosY = yMax + 5;
  
  //draw the vertical lines
  //9:30 left line
  tft.drawFastVLine(xMin, yMin, len, ST7735_WHITE);
  
  //10 - 4 pm
  int hour = 10;
  for(int x = xMin + (CHART_X_SPACING/2); x <= xMax; x += CHART_X_SPACING)
  {
    tft.drawFastVLine(x, yMin, len, ST7735_WHITE);
    int posX = (hour >= 10) ? x-4 : x-3;
    tft.setCursor(posX, textPosY);
    tft.print((hour > 12) ? hour - 12 : hour);
    hour++;
  }

  //figure out min and max prices
  float low = yearlow;
  float high = yearhigh;
  for(int i = 0; i < MAX_CHART_POINTS; i++)
  {
    float price = chartinfo[i];
    if(price > 0)
    { 
      if(low - price > 0.1f) low = price;
      if(price - high > 0.1f) high = price;
    }
  }
  /*
  float spread = (high - low) * .10;
  high += spread;
  low -= spread;
  */
  /*
  Serial.print("low: ");
  Serial.print(low);
  Serial.print(" high: ");
  Serial.println(high);
  
  Serial.print("52 week low: ");
  Serial.print(yearlow);
  Serial.print( "52 week high: ");
  Serial.print(yearhigh);
  Serial.print(" 200 day avg: ");
  Serial.println(movingAvg);
  */
  char label [10];

  char priceShort [] = "%.0f";
  char priceLong [] = "%.1f";
  
  for(int y = yMin; y <= yMax; y += CHART_Y_SPACING)
  {
    //draw the line
    tft.drawFastHLine(xMin, y, wid, ST7735_WHITE);

    //draw the price label - y incrememnts going down 
    //so need to reverse high and low
    float price = mmap(y, yMin, yMax, high, low);
    /*
    Serial.print("y: ");
    Serial.print(y);
    Serial.print(" price label: ");
    Serial.println(price);
    */
    //slight offset because of font goofiness
    tft.setCursor(0, y-3);
    
    //thousands don't have decimals
    if(price < 1000)
      sprintf(label, priceLong, price);
    else
      sprintf(label, priceShort, price);
    
    tft.print(label);
  }

  //calc the moving avg so we can print red or green later
  float scaledAvg = mmap(movingAvg, low, high, yMax, yMin); 
  tft.drawFastHLine(xMin, scaledAvg, wid, ST7735_DIMYELLOW);

  //52 week low
  if(high - yearhigh > 0.1f)
  {
    float scaledLow = mmap(yearlow, low, high, yMax, yMin); 
    tft.drawFastHLine(xMin, scaledLow, wid, ST7735_RED);
  }
  
  //52 week high
  if(yearlow - low > 0.1f)
  {
    float scaledHigh = mmap(yearhigh, low, high, yMax, yMin); 
    tft.drawFastHLine(xMin, scaledHigh, wid, ST7735_GREEN);
  }

  //grab first chart data point
  float price0 = chartinfo[0];
  int x0 = mmap(0, 0, MAX_CHART_POINTS, xMin, xMax);
  float scaled0 = mmap(price0, low, high, yMax, yMin);
  
  for(int i = 1; i < MAX_CHART_POINTS; i++)
  {
    yield();
    //we need two points to draw a line
    if(price0 > 0)
    {
      const float price1 = chartinfo[i];
      if(price1 > 0)
      {
        const float scaled1 = mmap(price1, low, high, yMax, yMin);
        /*
        Serial.print(i);
        Serial.print(" ");
        Serial.print(" price0: ");
        Serial.print(price0);
        Serial.print(" scaled0: ");
        Serial.println(scaled0);
        Serial.print("   price1: ");
        Serial.print(price1);
        Serial.print(" scaled1: ");
        Serial.println(scaled1);
        Serial.print("   movingAvg: ");
        Serial.print(movingAvg);
        Serial.print(" scaledAvg: ");
        Serial.println(scaledAvg);
        */
        const int x1 = mmap(i, 0, MAX_CHART_POINTS, xMin, xMax);
        const int color = (price0 > movingAvg || price1 > movingAvg) ? ST7735_GREEN : ST7735_RED;
        tft.drawLine(x0, scaled0, x1, scaled1, color);
        
        price0 = price1;
        scaled0 = scaled1;
        x0 = x1;
      }
      //zeroes are expected as the array is initalized to all zeroes.
      else if(price1 < 0)
      {
        Serial.print(i);
        Serial.print(F(" Skipping bogus price: "));
        Serial.println(price1);
      }
    }
  }

  //print the ticker
  int16_t x = xMax - (CHART_X_SPACING*2);
  int16_t y = yMin + (CHART_Y_SPACING/1.3);
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
  
  printWifiInfo(false); 
}

void printTBill()
{
  // Testing
  /*
  strcpy(tbilldates[0], "1/2");
  strcpy(tbilldates[1], "1/3");
  strcpy(tbilldates[2], "1/4");
  tbilldates[3][0] = '\0';
  tbillyields[0] = 1.0f;
  tbillyields[1] = 2.0f;
  tbillyields[2] = 3.0f;
  */
  
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);

  //use these, not the header contants
  const int yMin = CHART_Y_ORIGIN + 20;
  const int yMax = yMin + CHART_Y_HEIGHT - 20;
  const int xMin = CHART_X_ORIGIN;
  const int xMax = xMin + CHART_X_WIDTH;

  /*
  Serial.print("yMin: ");
  Serial.print(yMin);
  Serial.print(" yMax: ");
  Serial.println(yMax);
  */
  
  //draw the vertical lines
  int len = yMax-yMin;
  
  //origin
  tft.drawFastVLine(xMin, yMin, len, ST7735_WHITE);

  //midpoint
  tft.drawFastVLine(xMin + (CHART_X_WIDTH/2), yMin, len, ST7735_WHITE);
  
  //right side
  tft.drawFastVLine(xMax, yMin, len, ST7735_WHITE);
  
  const char priceShort [] = "%.1f";
  const char priceLong [] = "%.2f";

  //figure out min and max prices
  float low = tbillyields[0];
  float high = tbillyields[0];
  int totalDataPoints = 0;
  while(totalDataPoints < MAX_TBILLS && tbilldates[totalDataPoints][0] != '\0')
  {
    float yield = tbillyields[totalDataPoints];
    if(yield > 0)
    {  
      if(yield < low) low = yield;
      if(yield > high) high = yield;
    }
    totalDataPoints++;
  }

  //this is not the array size. since we use this nunmber a lot for scaling,
  //just decrement it instead of subtracting one every time we use it.
  totalDataPoints--;
  /*
  Serial.print("Total data points: ");
  Serial.println(totalDataPoints);
  
  Serial.print("low: ");
  Serial.print(low);
  Serial.print(" high: ");
  Serial.println(high);
  */
  char label [10];
  //draw the horizontal lines and price labels
  int wid = xMax - xMin;
  
  for(int y = yMin; y <= yMax; y += CHART_Y_SPACING)
  {
    //draw the line
    tft.drawFastHLine(xMin, y, wid, ST7735_WHITE);
    
    //draw the price label
    //reverse order of high low to invert scale
    float price = mmap(y, yMin, yMax, high, low);
    
    //slight offset because of font goofiness
    tft.setCursor(0, y-3);
    
    //thousands don't have decimals
    if(price < 1000)
      sprintf(label, priceLong, price);
    else
      sprintf(label, priceShort, price);
    
    tft.print(label);
  }

  //date Y position
  int textPosY = yMax + 5;
  tft.setFont();
  tft.setTextSize(1);
  //print month labels
  tft.setCursor(xMin-5, textPosY);
  tft.print(tbilldates[0]);

  int midPt = mmap(totalDataPoints/2, 0, totalDataPoints, xMin, xMax);
  tft.setCursor(midPt-5, textPosY);
  tft.print(tbilldates[totalDataPoints/2]);

  tft.setCursor(xMax - 20, textPosY);
  tft.print(tbilldates[totalDataPoints]);

  //grab first chart data point
  float price0 = tbillyields[0];
  int x0 = mmap(0, 0, totalDataPoints, xMin, xMax);
  float scaled0 = mmap(price0, low, high, yMax, yMin);

  for(int i = 1; i <= totalDataPoints; i++)
  {
    yield();
    //we need two points to draw a line
    if(price0 > 0)
    {
      const float price1 = tbillyields[i];
      if(price1 > 0)
      {
        const float scaled1 = mmap(price1, low, high, yMax, yMin);
        /*
        Serial.print(i);
        Serial.print(" ");
        Serial.print(" price0: ");
        Serial.print(price0);
        Serial.print(" scaled0: ");
        Serial.println(scaled0);
        Serial.print("  price1: ");
        Serial.print(price1);
        Serial.print(" scaled1: ");
        Serial.println(scaled1);
        */
        const int x1 = mmap(i, 0, totalDataPoints, xMin, xMax);
        tft.drawLine(x0, scaled0, x1, scaled1, ST7735_GREEN);
        
        price0 = price1;
        scaled0 = scaled1;
        x0 = x1;
      }
      else
      {
        Serial.print(i);
        Serial.print(F(" Skipping bogus tbill value: "));
        Serial.println(price1);
      }
    }
  }

  //print the header, final closing price.
  tft.setTextSize(1);
  tft.setTextColor(ST7735_GREEN);

  char str[30];
  sprintf(str, "%s %s %.2f%%", "10 Year TBill", tbilldates[totalDataPoints], tbillyields[totalDataPoints]);
  tft.setCursor(0, 5);
  tft.print(str);
 
  printWifiInfo(false); 
}

void printCoins()
{
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);

  //use these, not the header contants
  const int yMin = CHART_Y_ORIGIN + 20;
  const int yMax = yMin + CHART_Y_HEIGHT - 20;
  const int xMin = CHART_X_ORIGIN;
  const int xMax = xMin + CHART_X_WIDTH;
  /*
  Serial.print("coindate: ");
  Serial.print(coindate);
  Serial.print(" coinprice: ");
  Serial.println(coinprice);
  
  char debugBuf[100];
  for(int i = 0; i < MAX_COINS && coindates[i][0] != '\0'; i++)
  {
    sprintf(debugBuf, "coindates[%d]: %s coinprices[%d]: %.2f", i, coindates[i], i, coinprices[i]);
    Serial.println(debugBuf);
  }
  Serial.print("yMin: ");
  Serial.print(yMin);
  Serial.print(" yMax: ");
  Serial.println(yMax);
  */
  
  //draw the vertical lines
  int len = yMax-yMin;
  
  //origin
  tft.drawFastVLine(xMin, yMin, len, ST7735_WHITE);

  //midpoint
  tft.drawFastVLine(xMin + (CHART_X_WIDTH/2), yMin, len, ST7735_WHITE);
  
  //right side
  tft.drawFastVLine(xMax, yMin, len, ST7735_WHITE);
  
  const char priceShort [] = "%.0f";
  const char priceLong [] = "%.1f";

  //figure out min and max prices
  float low = coinprices[0];
  float high = coinprices[0];
  int totalDataPoints = 0;
  while(totalDataPoints < MAX_COINS && coindates[totalDataPoints][0] != '\0')
  {
    float price = coinprices[totalDataPoints];
    if(price > 0)
    {  
      if(price < low) low = price;
      if(price > high) high = price;
    }
    totalDataPoints++;
  }

  //this is not the array size. since we use this nunmber a lot for scaling,
  //just decrement it instead of subtracting one every time we use it.
  totalDataPoints--;
  /*
  Serial.print("Total data points: ");
  Serial.println(totalDataPoints);
  
  Serial.print("low: ");
  Serial.print(low);
  Serial.print(" high: ");
  Serial.println(high);
  */
  char label [10];
  //draw the horizontal lines and price labels
  int wid = xMax - xMin;
  
  for(int y = yMin; y <= yMax; y += CHART_Y_SPACING)
  {
    //draw the line
    tft.drawFastHLine(xMin, y, wid, ST7735_WHITE);
    
    //draw the price label
    //reverse order of high low to invert scale
    float price = mmap(y, yMin, yMax, high, low);
    
    //slight offset because of font goofiness
    tft.setCursor(0, y-3);
    
    //thousands don't have decimals
    if(price < 1000)
      sprintf(label, priceLong, price);
    else
      sprintf(label, priceShort, price);
    
    tft.print(label);
  }

  tft.setFont();
  tft.setTextSize(1);
  
  //print month labels
  //date Y position
  int textPosY = yMax + 5;
  tft.setCursor(xMin, textPosY);
  tft.print(coindates[0]);

  int midPt = mmap(totalDataPoints/2, 0, totalDataPoints, xMin, xMax);
  tft.setCursor(midPt-5, textPosY);
  tft.print(coindates[totalDataPoints/2]);

  tft.setCursor(xMax - 17 - strlen(coindates[totalDataPoints]), textPosY);
  tft.print(coindates[totalDataPoints]);

  //grab first chart data point
  float price0 = coinprices[0];
  int x0 = mmap(0, 0, totalDataPoints, xMin, xMax);
  float scaled0 = mmap(price0, low, high, yMax, yMin);

  for(int i = 1; i <= totalDataPoints; i++)
  {
    yield();
    //we need two points to draw a line
    if(price0 > 0)
    {
      const float price1 = coinprices[i];
      if(price1 > 0)
      {
        const float scaled1 = mmap(price1, low, high, yMax, yMin);
        /*
        Serial.print(i);
        Serial.print(" ");
        Serial.print(" price0: ");
        Serial.print(price0);
        Serial.print(" scaled0: ");
        Serial.println(scaled0);
        Serial.print("  price1: ");
        Serial.print(price1);
        Serial.print(" scaled1: ");
        Serial.println(scaled1);
        */
        const int x1 = mmap(i, 0, totalDataPoints, xMin, xMax);
        tft.drawLine(x0, scaled0, x1, scaled1, ST7735_GREEN);
        
        price0 = price1;
        scaled0 = scaled1;
        x0 = x1;
      }
      else
      {
        Serial.print(i);
        Serial.print(F(" Skipping bogus coin value: "));
        Serial.println(price1);
      }
    }
  }

  //print the header, final closing price.
  tft.setTextSize(1);
  tft.setTextColor(ST7735_GREEN);

  char str[30];
  sprintf(str, "%s %s %.2f", "Bitcoin Price", coindate, coinprice);
  tft.setCursor(0, 5);
  tft.print(str);
 
  printWifiInfo(false); 
}


float mmap(float x, float x_min, float x_max, float y_min, float y_max)
{
  return (x - x_min) * (y_max - y_min) / (x_max - x_min) + y_min;
}

