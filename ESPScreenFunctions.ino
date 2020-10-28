 
#ifdef ARDUINO_ESP8266_NODEMCU
  ST7735_REV tft = ST7735_REV(TFT_CS, TFT_DC, TFT_RST);
#endif
#ifdef ARDUINO_ESP8266_ESP12

  Adafruit_SPITFT *tft = NULL;
  
  
#endif

int yPos = 0;

void initScreen()
{
  
#ifdef ARDUINO_ESP8266_ESP12
  //This will check for either an HX8357 or ILI9341 based on the chip id
  //I had to create a dummy tft class to leverage the SPI methods to query
  //the chip. Once that is known, then I can assign the tft pointer to the
  //appropriate subclass.
  Generic_SPI_TFT g_tft = Generic_SPI_TFT(TFT_CS, TFT_DC);
  g_tft.begin(0);
  uint8_t x = g_tft.readcommand8(0x04);
  Serial.printf("Chip ID (0x04): 0x%02X\n", x);
  
  if(x == 0xFF || x == 0x80)
  {
    Serial.println("Using HX8347");
    tft = new Adafruit_HX8357(TFT_CS, TFT_DC);
    
    SCREEN_WIDTH = HX8357_TFTHEIGHT;
    SCREEN_HEIGHT = HX8357_TFTWIDTH;
  }
  else 
  {
    Serial.println("Using ILI9341");
    tft = new Adafruit_ILI9341(TFT_CS, TFT_DC);
    
    SCREEN_WIDTH = ILI9341_TFTHEIGHT;
    SCREEN_HEIGHT = ILI9341_TFTWIDTH;
  }

  //calculate the number of rows we can fit in usable area
  DISPLAY_ROWS = (SCREEN_HEIGHT - STATUS_MSG_HEIGHT) / TICKER_ROW_HEIGHT;
  //top border of chart - distance from origin
  CHART_Y_ORIGIN = 3;
  //total height of chart
  CHART_Y_HEIGHT = SCREEN_HEIGHT - 40;
  //how many pixels between vertical bars
  CHART_Y_SPACING = CHART_Y_HEIGHT / 6;
  //vertical position of chart labels - hour
  CHART_Y_TICKER_POS = CHART_Y_HEIGHT - 40;

  COLUMN_2 = SCREEN_WIDTH - 120;    // ARROW - FXED
  COLUMN_3 = SCREEN_WIDTH - 80;     // % CHANGE - FIXED
  COLUMN_4 = SCREEN_WIDTH - 30;     // SYMBOL - SLIDES

  CHART_X_WIDTH = SCREEN_WIDTH - CHART_X_ORIGIN - 5;
  //how many pixels between horizontal bars
  //const float CHART_X_SPACING = 40;
  //not 7.5, because of the leading 9:30 we draw separately
  CHART_X_SPACING = CHART_X_WIDTH / 6.5;
  
  tft->begin(0);
#endif

#ifdef ARDUINO_ESP8266_NODEMCU
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
      tft->initR(INITR_YELLOWTAB);
      Serial.println("YELLOW_TAB: true");
    }
  }

  for(int i = 0; i < YELLOW_TAB_NON_REVERSED_SIZE; i++)
  {
    if(strcmp(WiFi.macAddress().c_str(), YELLOW_TABS_NON_REVERSED[i]) == 0)
    {
      //means we have a goofed up yellow tab screen that
      //requires special orientation handling
      yellow = true;
      tft->initR(INITR_YELLOWTAB_NON_REVERSED) ;
      Serial.println("YELLOW_TAB_NON_REVERSED: true");
    }
  }

  // Use this initializer if you're using a 1.8" TFT
  // For some reason, a batch of them I received were printing
  // mirrored backwards text, so I had to special case them 
  // by deriving from Adafruit's ST7735 class. Hence the new
  // YELLOWTAB type and ST7735_REV class 
  if(!yellow) tft->initR(INITR_BLACKTAB);

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
  
  (flipped) ? tft->setRotation(3) : tft->setRotation(1);
#endif
#ifdef ARDUINO_ESP8266_ESP12
  tft->setRotation(3);
#endif
  tft->fillScreen(TFT_BLACK);
  tft->setTextWrap(false);
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
    tft->fillScreen(TFT_BLACK);
  tft->setTextWrap(true);
  tft->setTextColor(TFT_WHITE);
  tft->setTextSize(STATUS_FONT_SIZE);
  tft->setFont(STATUS_FONT);

  tft->fillRect(0, y, SCREEN_WIDTH, STATUS_MSG_HEIGHT, TFT_BLACK);
  tft->setCursor(0, y + STATUS_MSG_VERTICAL_OFFSET);
  tft->print(msg);
  tft->setTextWrap(false);

  Serial.printf_P(PSTR("\tprintMsg: %s\n"), msg.c_str());
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
  int status = WiFi.status();
  if(status == WL_CONNECTED)
  {
    char msg [strlen(hostName) + 20];
    String s = WiFi.localIP().toString();
    sprintf(msg, "V%.2f  %s", VERSION, s.c_str());
    printStatusMsg(msg, clrScreen);
    //Serial.println(msg);
  }
  else
  {
    printStatusMsg(F("WiFi disconnected"), clrScreen);
    Serial.printf_P(PSTR("\tWiFi status: %d\n"), status);
  }
}

void displayNextPage()
{
  Serial.println(F("displayNextPage()..."));
  Serial.printf_P(PSTR("\tPage: %d\n"), page);

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
    page++;
    if(updateChartInfo())
    {
      yield();
      char tickers[TICKER_COUNT][MAX_TICKER_SIZE];
      int num_tickers = readTickerFile(tickers);
      if(printTickerChart(tickers[0]))
      {
        printWifiInfo();
      }
    }
  }

  //tbill page
  else if(page == 3)
  {
    page++;
    if(SHOW_TBILLS)
    {
      if(updateFedInfo(MAX_TBILLS, TBILL_HIST_FILE))
      {
        yield();
        if(printHistoricalChart(MAX_TBILLS, TBILL_LABEL))
        {
           printWifiInfo();
        }
      }      
      else sincePrint = MAX_PAGE_INTERVAL;
    }
  }
  
  //oil page
  else if(page == 4)
  {
    page++;
    if(SHOW_OIL)
    { 
      if(updateFedInfo(MAX_OIL_PRICES, OIL_HIST_FILE))
      {
        yield();
        if(printHistoricalChart(MAX_OIL_PRICES, OIL_LABEL))
        {
          printWifiInfo();
        }
      }
      else sincePrint = MAX_PAGE_INTERVAL;
    }
  }

  //coin page
  else if(page == 5)
  {
    page++;

    if(SHOW_BITCOIN)
    {
      if(updateCoinInfo())
      {
        yield();
        if(printHistoricalChart(MAX_COINS+1, COIN_LABEL))
        {
          printWifiInfo();
        }
      }
      else sincePrint = MAX_PAGE_INTERVAL;
    }
  }

  httpServer.handleClient();  
  yield();

  //this is handled differently because tickers can span
  //more than one page.
  if(page < 2)
  {
    //clear off any cruft
    tft->fillScreen(TFT_BLACK);
    
    if(printTickers())
      printWifiInfo(); 
    //if it returns false, we printed an empty page, so flip to next;
    else
      sincePrint = MAX_PAGE_INTERVAL;
  }

  //last page, loop
  if(page == 6)
  {
    page = 0;
  }
  Serial.println(F("displayNextPage()...done"));
}

bool printTickers()
{
  Serial.println(F("printTickers()..."));
  int rowToPrint = page * DISPLAY_ROWS;
    
  if(rowToPrint > TICKER_COUNT)
  {
    Serial.println(F("rowToPrint > TICKER_COUNT - this shouldn't happen."));
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
  Serial.println(F("printTickers()...done"));
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
  
    int boxYPos = row * TICKER_ROW_HEIGHT-2;
    int fontYPos = (boxYPos+TICKER_ROW_HEIGHT-4);
  
    tft->setTextWrap(false); 
    tft->setFont(TICKER_FONT);
    tft->setTextSize(TICKER_FONT_SIZE);
    tft->setTextColor(TFT_WHITE);
    //slight adjustment to tighten box depending on font size
    tft->fillRect(0, boxYPos, SCREEN_WIDTH, TICKER_ROW_HEIGHT, TFT_BLACK); 
  
    if(strlen(ticker) > 0)
    {
      double price = price_list[tickerNum][0];
      double change = price_list[tickerNum][1];

      int triangleBase = -100;
      int triangleTop = -100;
      int triangleColor = TFT_GREY;
    
      if(change > 0.0)
      {
        triangleBase = fontYPos + TICKER_TRIANGLE_BASE_ADJ;
        triangleTop = fontYPos + TICKER_TRIANGLE_TOP_ADJ;
        triangleColor = TFT_GREEN;
        
      }
      else if(change < 0.0) 
      {
        triangleBase = fontYPos + TICKER_TRIANGLE_TOP_ADJ;
        triangleTop = fontYPos + TICKER_TRIANGLE_BASE_ADJ;
        triangleColor = TFT_RED;
      }

      tft->setCursor(COLUMN_0, fontYPos); 
      tft->print(ticker);

      tft->setCursor(COLUMN_1, fontYPos);
      tft->print(price);

      tft->fillTriangle(COLUMN_2, triangleBase, COLUMN_2+TICKER_TRIANGLE_WIDTH, triangleTop, COLUMN_2+(TICKER_TRIANGLE_WIDTH*2), triangleBase, triangleColor);

      tft->setCursor(COLUMN_3, fontYPos);    
      tft->print(fabs(change));
      
      tft->setCursor(COLUMN_4, fontYPos);
      tft->print("%");
      
      ret = true;
    }
  }
  
  //Serial.println(F("printTicker()...done"));
  return ret;
}

bool printTickerChart(char *ticker_name)
{
  Serial.println(F("printerTickerChart()..."));
  bool ret = true;
  tft->fillScreen(TFT_BLACK);
  
  //draw the horizontal lines and price labels
  const int yMin = CHART_Y_ORIGIN; 
  const int yMax = ((CHART_Y_ORIGIN + CHART_Y_HEIGHT) / CHART_Y_SPACING) * CHART_Y_SPACING + yMin;
  const int xMin = CHART_X_ORIGIN;
  int xMax = CHART_X_ORIGIN + CHART_X_WIDTH;
  const int wid = CHART_X_WIDTH;
  const int len = yMax - yMin;

  tft->setTextColor(TFT_WHITE);
  tft->setFont(TICKER_CHART_LABEL_FONT);
  tft->setTextSize(TICKER_CHART_LABEL_FONT_SIZE);
  
  //hour label Y position
  int textPosY = yMax + TICKER_CHART_LABEL_VERTICAL_OFFSET_X_AXIS;
  
  //draw the vertical lines
  //9:30 left line
  tft->drawFastVLine(xMin, yMin, len, TFT_WHITE);
  
  //10 - 4 pm
  int hour = 10;
  Serial.printf("xMax: %d\n", xMax);
  int x = 0;
  for(x = xMin + (CHART_X_SPACING/2); x <= xMax; x += CHART_X_SPACING)
  {
    Serial.printf("x: %d\n", x);
    tft->drawFastVLine(x, yMin, len, TFT_WHITE);
    int posX = (hour >= 10) ? x + TICKER_CHART_LABEL_HORIZONTAL_OFFSET_X_AXIS - 1 : x + TICKER_CHART_LABEL_HORIZONTAL_OFFSET_X_AXIS;
    tft->setCursor(posX, textPosY);
    tft->print((hour > 12) ? hour - 12 : hour);
    hour++;
  }

  //in case the math doesn't work out to get right to the edge.
  xMax = x - CHART_X_SPACING;

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
  
  printPriceLines(xMin, xMax, yMin, yMax, low, high, TFT_WHITE, CHART_Y_SPACING, TICKER_CHART_LABEL_VERTICAL_OFFSET_Y_AXIS);
  
  //calc the moving avg so we can print red or green later
  float scaledAvg = mmap(movingAvg, low, high, yMax, yMin); 
  tft->drawFastHLine(xMin, scaledAvg, wid, TFT_YELLOW);

  //52 week low
  if(high - yearhigh > 0.1f)
  {
    float scaledLow = mmap(yearlow, low, high, yMax, yMin); 
    tft->drawFastHLine(xMin, scaledLow, wid, TFT_RED);
  }
  
  //52 week high
  if(yearlow - low > 0.1f)
  {
    float scaledHigh = mmap(yearhigh, low, high, yMax, yMin); 
    tft->drawFastHLine(xMin, scaledHigh, wid, TFT_GREEN);
  }

  printGraphLine(MAX_CHART_POINTS, xMin, xMax, yMin, yMax, low, high, movingAvg);

  //print the ticker
  int16_t x_pos = xMax - (CHART_X_SPACING*2);
  int16_t y_pos = yMin + (CHART_Y_SPACING/1.3);
  int16_t xbounds;
  int16_t ybounds;
  uint16_t width;
  uint16_t height;

  tft->setFont(TICKER_CHART_TITLE_FONT); 
  tft->setTextSize(TICKER_CHART_TITLE_FONT_SIZE);
  tft->setTextColor(TFT_WHITE);
  tft->getTextBounds(ticker_name, x_pos, y_pos, &xbounds, &ybounds, &width, &height);
  tft->fillRect(xbounds-2, ybounds-2, width+3, height+3, TFT_BLACK);
  tft->setCursor(x_pos, y_pos);
  tft->print(ticker_name);
  
  Serial.println(F("printerTickerChart()...done"));
  return ret;
}

bool printHistoricalChart(int max_data_points, const char *title)
{
  Serial.println(F("printHistoricalChart()..."));
  bool ret = true;
  tft->fillScreen(TFT_BLACK);

  //use these, not the header contants
  const int yMin = CHART_Y_ORIGIN + HISTORICAL_CHART_VERTICAL_OFFSET;
  const int yMax = (((SCREEN_HEIGHT - HISTORICAL_CHART_VERTICAL_OFFSET - STATUS_MSG_HEIGHT) / CHART_Y_SPACING) * CHART_Y_SPACING) + yMin;
  const int xMin = CHART_X_ORIGIN;
  const int xMax = xMin + CHART_X_WIDTH;
  const int len = yMax - yMin;
  //Serial.printf("CHART_Y_ORIGIN: %d HISTORICAL_CHART_VERTICAL_OFFSET: %d HISTORICAL_CHART_Y_HEIGHT: %d CHART_Y_SPACING: %d\n", 
  //  CHART_Y_ORIGIN, HISTORICAL_CHART_VERTICAL_OFFSET, HISTORICAL_CHART_Y_HEIGHT, CHART_Y_SPACING);
  //Serial.printf("yMin: %d yMax: %d len: %d xMin: %d xMax %d\n", yMin, yMax, len, xMin, xMax);

  tft->setTextColor(TFT_WHITE);
  tft->setTextSize(HISTORICAL_LABEL_FONT_SIZE);
  tft->setFont(HISTORICAL_LABEL_FONT);
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
  
  //origin
  tft->drawFastVLine(xMin, yMin, len, TFT_WHITE);

  //midpoint
  tft->drawFastVLine(xMin + (CHART_X_WIDTH/2), yMin, len, TFT_WHITE);
  
  //right side
  tft->drawFastVLine(xMax, yMin, len, TFT_WHITE);

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

  //Serial.printf("%sTotal data points: %d\n", ERROR_MSG_INDENT, totalDataPoints);
  /*
  Serial.print("low: ");
  Serial.print(low);
  Serial.print(" high: ");
  Serial.println(high);
  */

  printPriceLines(xMin, xMax, yMin, yMax, low, high, TFT_WHITE, CHART_Y_SPACING, HISTORICAL_LABEL_VERTICAL_OFFSET_Y_AXIS);
  
  //print month labels
  //date Y position
  int textPosY = yMax + HISTORICAL_LABEL_VERTICAL_OFFSET_X_AXIS;

  //min date
  //left aligned
  tft->setCursor(xMin, textPosY);
  tft->print(string_list[0]);

  int16_t x1, y1;
  uint16_t w, h;
  
  //mid point date
  int midPt = mmap(totalDataPoints/2, 0, totalDataPoints, xMin, xMax);
  tft->getTextBounds(string_list[totalDataPoints/2], SCREEN_WIDTH, SCREEN_HEIGHT, &x1, &y1, &w, &h);
  tft->setCursor(midPt - w/2, textPosY);
  tft->print(string_list[totalDataPoints/2]);

  //max date
  //calculate the width of the string in pixels by drawing off screen and then subtract
  //from the right margin to place the text
  tft->getTextBounds(string_list[totalDataPoints-1], SCREEN_WIDTH, SCREEN_HEIGHT, &x1, &y1, &w, &h);
  //Serial.printf("bounds x: %d, w: %d\n", x1, w);
  tft->setCursor(xMax - w, textPosY);
  tft->print(string_list[totalDataPoints-1]);

  printGraphLine(totalDataPoints, xMin, xMax, yMin, yMax, low, high, 0);
  
  //print the header, final closing price.
  tft->setFont(HISTORICAL_TITLE_FONT);
  tft->setTextSize(HISTORICAL_TITLE_FONT_SIZE);
  tft->setTextColor(TFT_GREEN);

  char str[30];
  sprintf(str, "%s %s %.2f", title, string_list[totalDataPoints-1], price_list[totalDataPoints-1][0]);
  tft->setCursor(HISTORICAL_TITLE_POS_X, HISTORICAL_TITLE_POS_Y);
  tft->print(str);
 
  Serial.println(F("printHistoricalChart()...done"));
  return ret;
}

void printPriceLines(const int xMin, const int xMax, const int yMin, const int yMax, float low, float high, uint16_t lineColor, int ySpacing, int labelOffset)
{
  char label [10];

  //draw the horizontal lines and price labels
  int wid = xMax - xMin;
  //Serial.printf("+++ yMin: %d, yMax: %d, CHART_Y_SPACING: %d+++ \n", yMin, yMax, CHART_Y_SPACING);
  for(int y = yMin; y <= yMax; y += ySpacing)
  {
    //Serial.printf("%s*** Printing horizontal line at pixel: %d ***\n", ERROR_MSG_INDENT, y);
    //draw the line
    tft->drawFastHLine(xMin, y, wid, lineColor);

    //draw the price label
    //reverse order of high low to invert scale
    float price = mmap(y, yMin, yMax, high, low);

    //slight offset because of font goofiness
    tft->setCursor(0, y + labelOffset);
    //thousands don't have decimals
    if(price > 999)
      sprintf(label, "%.0f", price);
    else if (price > 99)
      sprintf(label, "%.1f", price);
    else
      sprintf(label, "%.2f", price);

    tft->print(label);
  }
}

void printGraphLine(int totalDataPoints, const int xMin, const int xMax, const int yMin, const int yMax, float low, float high, float movingAverage)
{
  //grab first chart data point
  float price0 = price_list[0][0];
  int x0 = mmap(0, 0, totalDataPoints-1, xMin, xMax);
  float scaled0 = mmap(price0, low, high, yMax, yMin);

  for(int i = 1; i < totalDataPoints; i++)
  {
    yield();

    //we need two points to draw a line
    const float price1 = price_list[i][0];
    const int x1 = mmap(i, 0, totalDataPoints-1, xMin, xMax);
    const float scaled1 = mmap(price1, low, high, yMax, yMin);

    if(price0 > 0)
    {
      if(price1 > 0)
      {
        //Serial.printf_P(PSTR("%si: %d price0: %2f scaled0: %2f price1: %2f scaled1: %2f movingAverage: %2f\n"), ERROR_MSG_INDENT, i, price0, scaled0, price1, scaled1, movingAverage);
        const int color = (price0 > movingAverage || price1 > movingAverage) ? TFT_GREEN : TFT_RED;
        tft->drawLine(x0, scaled0, x1, scaled1, color);

        price0 = price1;
        scaled0 = scaled1;
        x0 = x1;
      }
      //zeroes are expected as the array is initalized to all zeroes.
      else if(price1 < 0)
      {
        Serial.printf_P(PSTR("%si: %d skipping invalid price1: %f\n"), ERROR_MSG_INDENT, i, price1);
      }
    }
    //zeroes are expected as the array is initalized to all zeroes.
    else if(price0 < 0)
    {
      Serial.printf_P(PSTR("i: %d skipping invalid price0: %f\n"), ERROR_MSG_INDENT, i, price0);
      price0 = price1;
      scaled0 = scaled1;
      x0 = x1;
    }
  }
}

float mmap(float x, float x_min, float x_max, float y_min, float y_max)
{
  return (x - x_min) * (y_max - y_min) / (x_max - x_min) + y_min;
}
