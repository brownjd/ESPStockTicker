 
#ifdef ARDUINO_ESP8266_NODEMCU
  ST7735_REV tft = ST7735_REV(TFT_CS, TFT_DC, TFT_RST);
#endif
#ifdef ARDUINO_ESP8266_ESP12
  Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
#endif

int yPos = 0;

void initScreen()
{
  
#ifdef ARDUINO_ESP8266_ESP12
  tft.begin();
#endif

#ifdef ARDUINO_ESP8266_NODEMCU
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
#endif
#ifdef ARDUINO_ESP8266_ESP12
  tft.setRotation(3);
#endif
  tft.fillScreen(TFT_BLACK);
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
    tft.fillScreen(TFT_BLACK);
  tft.setTextWrap(true);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(FONT_SIZE);
  tft.setFont();

  tft.fillRect(0, y-5, SCREEN_WIDTH, 15, TFT_BLACK);
  tft.setCursor(0, y);
  tft.print(msg);
  tft.setTextWrap(false);
}

void printStatusMsg(String msg, bool clrScreen)
{
  printMsg(msg, 0, SCREEN_HEIGHT - STATUS_MSG_HEIGHT, clrScreen);
  Serial.printf("Status message: %s\n", msg.c_str());
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
    String s = WiFi.localIP().toString();
    sprintf(msg, "V%.2f  %s", VERSION, s.c_str());
    printStatusMsg(msg, clrScreen);
    //Serial.println(msg);
  }
  else
  {
    printStatusMsg(F("Wifi disconnected"), clrScreen);
  }
}

void displayNextPage()
{
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

  else if(page == 1)
  {
    //no need to do anything
  }
  
  else if(page == 2)
  {
    updateChartInfo();
    yield();
    char tickers[TICKER_COUNT][MAX_TICKER_SIZE];
    int num_tickers = readTickerFile(tickers);
    printChart(tickers[0]);
    page++;
  }

  //tbill page
  else if(page == 3)
  {
    page++;
    if(SHOW_TBILLS)
    {
      updateFedInfo(MAX_TBILLS, TBILL_HIST_FILE);
      yield();
      printHistoricalChart(MAX_TBILLS, TBILL_LABEL);
    }
    else sincePrint = MAX_PAGE_INTERVAL;
  }
  
  //oil page
  else if(page == 4)
  {
    page++;
    if(SHOW_OIL)
    { 
      updateFedInfo(MAX_OIL_PRICES, OIL_HIST_FILE);
      yield();
      printHistoricalChart(MAX_OIL_PRICES, OIL_LABEL);
    }
    else sincePrint = MAX_PAGE_INTERVAL;
  }

  //coin page
  else if(page == 5)
  {
    page++;

    if(SHOW_BITCOIN)
    {
      updateCoinInfo();
      yield();
      printHistoricalChart(MAX_COINS, COIN_LABEL);
    }
    else sincePrint = MAX_PAGE_INTERVAL;
  }

  httpServer.handleClient();  
  yield();

  //this is handled differently because tickers can span
  //more than one page.
  if(page < 2)
  {
    //if it returns false, we printed an empty page, so flip to next;
    if(!printTickers()) 
      sincePrint = MAX_PAGE_INTERVAL;
  }

  //last page, loop
  if(page == 6)
  {
    page = 0;
  }
}

bool printTickers()
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

  char tickers[TICKER_COUNT][MAX_TICKER_SIZE];
  
  bool printed = false;

  int num_tickers = readTickerFile(tickers);
  
  while(rowToPrint < TICKER_COUNT && rowToPrint < lastRowToPrint) 
  {
    if(printTicker(tickers, rowToPrint)) 
      printed = true;
    rowToPrint++;
  }
  page++;
  return printed;
}

bool printTicker(char tickers[][MAX_TICKER_SIZE], int tickerNum)
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
    tft.setFont(&MONOSPACED_FONT);
    tft.setTextColor(TFT_WHITE);
    //slight adjustment to tighten box depending on font size
    tft.fillRect(0, boxYPos, SCREEN_WIDTH, ROW_HEIGHT, TFT_BLACK); 
  
    if(strlen(ticker) > 0)
    {
      double price = price_list[tickerNum][0];
      double change = price_list[tickerNum][1];

      int triangleBase = -100;
      int triangleTop = -100;
      int triangleColor = TFT_GREY;
      
      if(change > 0.0)
      {
        triangleBase = fontYPos - 2;
        triangleTop = fontYPos - 6;
        triangleColor = TFT_GREEN;
        
      }
      else if(change < 0.0) 
      {
        triangleBase = fontYPos - 6;
        triangleTop = fontYPos - 2;
        triangleColor = TFT_RED;
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

void printChart(char *ticker_name)
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
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(FONT_SIZE);

  //draw the horizontal lines and price labels
  const int yMin = CHART_Y_ORIGIN; 
  const int yMax = ((yMin + CHART_Y_HEIGHT) / CHART_Y_SPACING) * CHART_Y_SPACING + CHART_Y_ORIGIN;;
  const int xMin = CHART_X_ORIGIN;
  const int xMax = CHART_X_ORIGIN + CHART_X_WIDTH;
  const int wid = CHART_X_WIDTH;
  const int len = yMax - yMin;
  
  //hour label Y position
  int textPosY = yMax + 5;
  
  //draw the vertical lines
  //9:30 left line
  tft.drawFastVLine(xMin, yMin, len, TFT_WHITE);
  
  //10 - 4 pm
  int hour = 10;
  for(int x = xMin + (CHART_X_SPACING/2); x <= xMax; x += CHART_X_SPACING)
  {
    tft.drawFastVLine(x, yMin, len, TFT_WHITE);
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
    float price = price_list[i][0];
    //Serial.println(price);
    if(price > 0.0f)
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

  //Serial.printf("+++ yMin: %d, yMax: %d, CHART_Y_SPACING: %d+++ \n", yMin, yMax, CHART_Y_SPACING);
  for(int y = yMin; y <= yMax; y += CHART_Y_SPACING)
  {
    //Serial.printf("*** Printing horizontal line at pixel: %d ***\n", y);
    //draw the line
    tft.drawFastHLine(xMin, y, wid, TFT_WHITE);

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
  tft.drawFastHLine(xMin, scaledAvg, wid, TFT_YELLOW);

  //52 week low
  if(high - yearhigh > 0.1f)
  {
    float scaledLow = mmap(yearlow, low, high, yMax, yMin); 
    tft.drawFastHLine(xMin, scaledLow, wid, TFT_RED);
  }
  
  //52 week high
  if(yearlow - low > 0.1f)
  {
    float scaledHigh = mmap(yearhigh, low, high, yMax, yMin); 
    tft.drawFastHLine(xMin, scaledHigh, wid, TFT_GREEN);
  }

  //grab first chart data point
  float price0 = price_list[0][0];
  int x0 = mmap(0, 0, MAX_CHART_POINTS, xMin, xMax);
  float scaled0 = mmap(price0, low, high, yMax, yMin);
  
  for(int i = 1; i < MAX_CHART_POINTS; i++)
  {
    yield();
    //we need two points to draw a line
    if(price0 > 0)
    {
      const float price1 = price_list[i][0];
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
        const int color = (price0 > movingAvg || price1 > movingAvg) ? TFT_GREEN : TFT_RED;
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

  tft.setFont(&MONOSPACED_FONT); 
  tft.setTextColor(TFT_WHITE);
  tft.getTextBounds(ticker_name, x, y, &xbounds, &ybounds, &width, &height);
  tft.fillRect(xbounds-2, ybounds-2, width+3, height+3, TFT_BLACK);
  tft.setCursor(x, y);
  tft.print(ticker_name);
  
  printWifiInfo(false); 
}

void printHistoricalChart(int max_data_points, const char *title)
{
  Serial.println("Starting printHistoricalChart()...");
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(FONT_SIZE);

  //use these, not the header contants
  const int yMin = CHART_Y_ORIGIN + HISTORICAL_CHART_VERTICAL_OFFSET;
  const int yMax = ((CHART_Y_ORIGIN + CHART_Y_HEIGHT) / CHART_Y_SPACING) * CHART_Y_SPACING + 1;
  //Serial.printf("yMax after: %d\n", yMax);
  const int xMin = CHART_X_ORIGIN;
  const int xMax = xMin + CHART_X_WIDTH;
  /*
  
  char debugBuf[100];
  for(int i = 0; i < max_data_points && string_list[i][0] != '\0'; i++)
  {
    sprintf(debugBuf, "string_list[%d]: %s price_list[%d]: %.2f", i, string_list[i], i, price_list[i][0]);
    Serial.println(debugBuf);
    yield();
  }
  Serial.print("yMin: ");
  Serial.print(yMin);
  Serial.print(" yMax: ");
  Serial.println(yMax);
  */
  
  //draw the vertical lines
  int len = yMax - yMin;
  
  //origin
  tft.drawFastVLine(xMin, yMin, len, TFT_WHITE);

  //midpoint
  tft.drawFastVLine(xMin + (CHART_X_WIDTH/2), yMin, len, TFT_WHITE);
  
  //right side
  tft.drawFastVLine(xMax, yMin, len, TFT_WHITE);

  //figure out min and max prices
  float low = price_list[0][0];
  float high = price_list[0][0];
  int totalDataPoints = 0;
  while(totalDataPoints < max_data_points && string_list[totalDataPoints][0] != '\0')
  {
    float price = price_list[totalDataPoints][0];
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
  //Serial.printf("+++ yMin: %d, yMax: %d, CHART_Y_SPACING: %d+++ \n", yMin, yMax, CHART_Y_SPACING);
  for(int y = yMin; y <= yMax; y += CHART_Y_SPACING)
  {
    //Serial.printf("*** Printing horizontal line at pixel: %d ***\n", y);
    //draw the line
    tft.drawFastHLine(xMin, y, wid, TFT_WHITE);
    
    //draw the price label
    //reverse order of high low to invert scale
    float price = mmap(y, yMin, yMax, high, low);
    
    //slight offset because of font goofiness
    tft.setCursor(0, y-3);
    //thousands don't have decimals
    if(price > 999)
      sprintf(label, "%.0f", price);
    else if (price > 99)
      sprintf(label, "%.1f", price);
    else
      sprintf(label, "%.2f", price);
    
    tft.print(label);
  }
  
  tft.setFont();
  tft.setTextSize(FONT_SIZE);
  
  //print month labels
  //date Y position
  int textPosY = yMax + 5;
  tft.setCursor(xMin, textPosY);
  tft.print(string_list[0]);

  int midPt = mmap(totalDataPoints/2, 0, totalDataPoints, xMin, xMax);
  tft.setCursor(midPt-5, textPosY);
  tft.print(string_list[totalDataPoints/2]);
  tft.setCursor(xMax - 17 - (2 * strlen(string_list[totalDataPoints])), textPosY);
  tft.print(string_list[totalDataPoints]);

  //grab first chart data point
  float price0 = price_list[0][0];
  int x0 = mmap(0, 0, totalDataPoints, xMin, xMax);
  float scaled0 = mmap(price0, low, high, yMax, yMin);

  for(int i = 1; i <= totalDataPoints; i++)
  {
    yield();
    //we need two points to draw a line
    if(price0 > 0)
    {
      const float price1 = price_list[i][0];
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
        tft.drawLine(x0, scaled0, x1, scaled1, TFT_GREEN);
        
        price0 = price1;
        scaled0 = scaled1;
        x0 = x1;
      }
      else
      {
        Serial.print(i);
        Serial.print(F(" Skipping bogus price value: "));
        Serial.println(price1);
      }
    }
  }

  //print the header, final closing price.
  tft.setTextSize(HISTORICAL_CHART_FONT_SIZE);
  tft.setTextColor(TFT_GREEN);

  char str[30];
  sprintf(str, "%s %s %.2f", title, string_list[totalDataPoints], price_list[totalDataPoints][0]);
  tft.setCursor(0, 5);
  tft.print(str);
 
  printWifiInfo(false); 
  Serial.println("Starting printHistoricalChart()...done");
}

float mmap(float x, float x_min, float x_max, float y_min, float y_max)
{
  return (x - x_min) * (y_max - y_min) / (x_max - x_min) + y_min;
}
