#define VERSION 2.36

//This supports an ST7735 with a custom
//PCB or an ILI9341 screen using the
//Adafruit Featherwing and Huzzah boards
//NodeMCU ARDUINO_ESP8266_NODEMCU -- this assumes an ST7735 display
//Huzzah ARDUINO_ESP8266_ESP12 -- this assumes an ILI9341 on a Featherwing
//these get set by the IDE at build time when you pick the board

#ifdef ARDUINO_ESP8266_NODEMCU
  #include "ST7735_REV.h"
  #define SCREEN_WIDTH ST7735_TFTHEIGHT_160
  #define SCREEN_HEIGHT ST7735_TFTHEIGHT_128
  
  #include "fonts/Monospaced_plain_11.h"
  #define MONOSPACED_FONT_11 Monospaced_plain_11

  #define TICKER_FONT &MONOSPACED_FONT_11
  #define TICKER_FONT_SIZE 1

  #define HISTORICAL_TITLE_FONT &MONOSPACED_FONT_11
  #define HISTORICAL_TITLE_FONT_SIZE 1
  #define HISTORICAL_LABEL_FONT
  #define HISTORICAL_LABEL_FONT_SIZE 1
  #define HISTORICAL_LABEL_VERTICAL_OFFSET_X_AXIS 6
  #define HISTORICAL_LABEL_VERTICAL_OFFSET_Y_AXIS -3
  #define HISTORICAL_TITLE_POS_X 0
  #define HISTORICAL_TITLE_POS_Y 10
  #define HISTORICAL_CHART_VERTICAL_OFFSET 15
  #define HISTORICAL_CHART_Y_HEIGHT 80
  
  #define TICKER_CHART_TITLE_FONT &MONOSPACED_FONT_11
  #define TICKER_CHART_TITLE_FONT_SIZE 1 
  #define TICKER_CHART_LABEL_FONT
  #define TICKER_CHART_LABEL_FONT_SIZE 1
  #define TICKER_CHART_LABEL_VERTICAL_OFFSET_X_AXIS 5
  #define TICKER_CHART_LABEL_VERTICAL_OFFSET_Y_AXIS -2
  #define TICKER_CHART_LABEL_HORIZONTAL_OFFSET_X_AXIS -3

  #define STATUS_FONT 
  #define STATUS_FONT_SIZE 1
  #define STATUS_MSG_HEIGHT 10
  #define STATUS_MSG_VERTICAL_OFFSET 2
  
  #define MSG_LINE_SPACING 1.75

  #define TICKER_ROW_HEIGHT 14
  #define TICKER_TRIANGLE_BASE_ADJ -2
  #define TICKER_TRIANGLE_TOP_ADJ -6
  #define TICKER_TRIANGLE_WIDTH 4
  

  //default triangle color - doubt we need it
  #define ST7735_GREY   0xB5F6
  #define ST7735_DIMYELLOW 0xFF36

  //TFT pins
  #define TFT_CS  D8
  #define TFT_RST D4
  #define TFT_DC  D3  /A0
  #define TFT_BLACK ST7735_BLACK
  #define TFT_WHITE ST7735_WHITE
  #define TFT_GREEN ST7735_GREEN
  #define TFT_RED ST7735_RED
  #define TFT_YELLOW ST7735_DIMYELLOW
  #define TFT_GREY ST7735_GREY

  //Bill "60:01:94:75:52:F0",

  //list of mac addresses for ESPs soldered to screwed up Ebay screen that print backwards.
  //i call them YELLOWTABS because of they had yellow tabs on the screen protectors
  const int YELLOW_TAB_SIZE = 2;
  const char YELLOW_TABS[YELLOW_TAB_SIZE][18] = { "60:01:94:74:4A:42", "68:C6:3A:9F:B6:61" };

  const int ROTATION_SIZE = 3;

  //These are the bright ones. We can probably just make this the default
  const char ROTATION_OVERRIDES[ROTATION_SIZE][18] = { "DC:4F:22:1C:2A:98", "B4:E6:2D:69:D5:0D", "BC:DD:C2:16:CA:4F" };

  //calculate the number of rows we can fit in usable area
  const int DISPLAY_ROWS = (SCREEN_HEIGHT - STATUS_MSG_HEIGHT) / TICKER_ROW_HEIGHT;
  
  //screen is 160 x 128
  //column value is left hand side or start of column
  const int COLUMN_0 = 0;                     //   0 - 40  (40 px wide) SYMBOL - FIXED
  const int COLUMN_1 = 40;                    //  40 - 105 (65 px wide) PRICE - SCALED
  const int COLUMN_2 = SCREEN_WIDTH - 60;                   // 105 - 155 (50 px wide) % CHANGE - FIXED
  const int COLUMN_3 = SCREEN_WIDTH - 48;
  const int COLUMN_4 = SCREEN_WIDTH - 6;      // 155 - 160 ( 5 px wide) % SYMBOL - SLIDES

  //left side of chart in pixels
  const int CHART_X_ORIGIN = 35;
  //how many pixels between horizontal bars
  const float CHART_X_SPACING = 19;
  //not 7.5, because of the leading 9:30 we draw separately
  const float CHART_X_WIDTH = CHART_X_SPACING * 6.5;
  //top border of chart - distance from origin
  const int CHART_Y_ORIGIN = 3;
  //total height of chart
  const int CHART_Y_HEIGHT = 100;
  //how many pixels between vertical bars
  const int CHART_Y_SPACING = 20;
  //vertical position of chart labels - hour
  const int CHART_Y_TICKER_POS = 118;

  const char* TBILL_LABEL = "10 Year";
  const char* OIL_LABEL = "WTI";
  const char* COIN_LABEL = "Bitcoin";
  
#endif

#ifdef ARDUINO_ESP8266_ESP12
  #include "Adafruit_ILI9341.h"
  #define SCREEN_WIDTH ILI9341_TFTHEIGHT
  #define SCREEN_HEIGHT ILI9341_TFTWIDTH
  
  #include "fonts/Monospaced_plain_12.h"
  #define MONOSPACED_FONT_12 Monospaced_plain_12

  #include "fonts/Monospaced_plain_15.h"
  #define MONOSPACED_FONT_15 Monospaced_plain_15
  
  #include "fonts/Monospaced_plain_18.h"
  #define MONOSPACED_FONT_18 Monospaced_plain_18

  #define TICKER_FONT &MONOSPACED_FONT_18
  #define TICKER_FONT_SIZE 1

  #define HISTORICAL_TITLE_FONT &MONOSPACED_FONT_18
  #define HISTORICAL_TITLE_FONT_SIZE 1
  #define HISTORICAL_LABEL_FONT &MONOSPACED_FONT_15
  #define HISTORICAL_LABEL_FONT_SIZE 1
  #define HISTORICAL_LABEL_VERTICAL_OFFSET_X_AXIS 20
  #define HISTORICAL_LABEL_VERTICAL_OFFSET_Y_AXIS 8
  #define HISTORICAL_TITLE_POS_X 0
  #define HISTORICAL_TITLE_POS_Y 20
  #define HISTORICAL_CHART_VERTICAL_OFFSET 30
  #define HISTORICAL_CHART_Y_HEIGHT 160
  

  #define TICKER_CHART_TITLE_FONT &MONOSPACED_FONT_18
  #define TICKER_CHART_TITLE_FONT_SIZE 1 
  #define TICKER_CHART_LABEL_FONT &MONOSPACED_FONT_15
  #define TICKER_CHART_LABEL_FONT_SIZE 1
  #define TICKER_CHART_LABEL_VERTICAL_OFFSET_X_AXIS 18
  #define TICKER_CHART_LABEL_VERTICAL_OFFSET_Y_AXIS 9
  #define TICKER_CHART_LABEL_HORIZONTAL_OFFSET_X_AXIS -5

  #define STATUS_FONT &MONOSPACED_FONT_12
  #define STATUS_FONT_SIZE 1
  #define STATUS_MSG_HEIGHT 16
  #define STATUS_MSG_VERTICAL_OFFSET 12
  
  #define MSG_LINE_SPACING 1.50

  //Pricing screen Settings
  #define TICKER_ROW_HEIGHT 27
  #define TICKER_TRIANGLE_BASE_ADJ -2
  #define TICKER_TRIANGLE_TOP_ADJ -10
  #define TICKER_TRIANGLE_WIDTH 8
  
  #define TFT_CS   0
  #define TFT_DC   15
  #define TFT_BLACK ILI9341_BLACK
  #define TFT_WHITE ILI9341_WHITE
  #define TFT_GREEN ILI9341_GREEN
  #define TFT_RED ILI9341_RED
  #define TFT_YELLOW ILI9341_YELLOW
  #define TFT_GREY ILI9341_LIGHTGREY

  //calculate the number of rows we can fit in usable area
  const int DISPLAY_ROWS = (SCREEN_HEIGHT - STATUS_MSG_HEIGHT) / TICKER_ROW_HEIGHT;
  
  //screen is 320 x 256
  //column value is left hand side or start of column
  const int COLUMN_0 = 0;                     // SYMBOL - FIXED
  const int COLUMN_1 = 100;                   // PRICE - FIXED
  const int COLUMN_2 = SCREEN_WIDTH - 120;    // ARROW - FXED
  const int COLUMN_3 = SCREEN_WIDTH - 80;     // % CHANGE - FIXED
  const int COLUMN_4 = SCREEN_WIDTH - 30;     // SYMBOL - SLIDES

  //Chart Settings
  //left side of chart in pixels
  const int CHART_X_ORIGIN = 55;
  //how many pixels between horizontal bars
  const float CHART_X_SPACING = 40;
  //not 7.5, because of the leading 9:30 we draw separately
  const float CHART_X_WIDTH = CHART_X_SPACING * 6.5;
  //top border of chart - distance from origin
  const int CHART_Y_ORIGIN = 3;
  //total height of chart
  const int CHART_Y_HEIGHT = 200;
  //how many pixels between vertical bars
  const int CHART_Y_SPACING = 32;
  //vertical position of chart labels - hour
  const int CHART_Y_TICKER_POS = 240;

  const char* TBILL_LABEL = "10 Year TBill";
  const char* OIL_LABEL = "WTI";
  const char* COIN_LABEL = "Bitcoin Price";
#endif

//SCL ->  SCK   ->  D5
//SDA ->  MOSI  ->  D7
//VCC     +3.3V
//LED-    GND
//LED+    VIN +5V
//GND     GND

#define ARDUINOJSON_USE_DOUBLE 0

const int MAX_WIFI_NETWORKS = 5;
const char *WIFI_FILE = "/wifi.txt";

//number of 500 ms increments to wait for a connection
const int MAX_WIFI_TIMEOUT = 20;
const int HTTP_PORT = 80;
const int HTTPS_PORT = 443;
const int OTA_PORT = 8080;
char hostName [20] = "esp";
const char* SOFT_AP_NAME = "ESPSoftAP";

//stock ticker api and SSL fingerprint
//const char* SSL_FINGERPRINT = "D1 34 42 D6 30 58 2F 09 A0 8C 48 B6 25 B4 6C 05 69 A4 2E 4E";

const char* TICKER_FILE = "/tickers.txt";
const char* CHART_FILE = "/chart.json";
const char* PRICING_FILE = "/prices.json";
const char* KEY_STATS_FILE = "/keystats.json";
const char* TBILL_HIST_FILE = "/tbill.csv";
const char* COIN_HIST_FILE = "/coinhist.json";
const char* OIL_HIST_FILE = "/oilhist.csv";

const char* FW_REMOTE_VERSION_FILE = "/version.remote";

const char* IEX_HOST = "cloud.iexapis.com";
const char* PRICING_CHART_URL = "GET /stable/stock/market/batch?token=pk_8990f87d67544a4b889cd03d8fe3b68c&filter=latestPrice,changePercent&types=quote&displayPercent=true&symbols=%s HTTP/1.0\r\nHost: cloud.iexapis.com\r\nUser-Agent: ESP8266\r\nConnection: close\r\n\r\n";
//interval is number of minutes between prices
const char* BASE_CHART_URL = "GET /stable/stock/%s/intraday-prices?token=pk_8990f87d67544a4b889cd03d8fe3b68c&filter=average&chartInterval=%d HTTP/1.0\r\nHost: cloud.iexapis.com\r\nUser-Agent: ESP8266\r\nConnection: close\r\n\r\n";
const char* KEY_STATS_URL = "GET /stable/stock/%s/stats?token=pk_8990f87d67544a4b889cd03d8fe3b68c&filter=week52low,week52high,day200MovingAvg HTTP/1.0\r\nHost: cloud.iexapis.com\r\nUser-Agent: ESP8266\r\nConnection: close\r\n\r\n";

const char* TIME_HOST = "worldclockapi.com";
const char* TIME_URL = "GET /api/json/cst/now HTTP/1.0\r\nHost: worldclockapi.com\r\nUser-Agent: ESP8266\r\nConnection: close\r\n\r\n";

//https://www.federalreserve.gov/datadownload/Output.aspx?rel=H15&series=bcb44e369321bfb3f&lastobs=180&from=&to=&filetype=csv&label=omit&layout=seriescolumn

const char* COIN_HOST = "api.coindesk.com";
const char* COIN_HIST_URL = "GET /v1/bpi/historical/close.json HTTP/1.0\r\nHost: api.coindesk.com\r\nUser-Agent: ESP8266\r\nConnection: close\r\n\r\n";
const char* COIN_CURR_URL = "GET /v1/bpi/currentprice/USD.json HTTP/1.0\r\nHost: api.coindesk.com\r\nUser-Agent: ESP8266\r\nConnection: close\r\n\r\n";

const char* FED_HOST = "fred.stlouisfed.org";
const char* OIL_URL = "GET /graph/fredgraph.csv?cosd=%s&mode=fred&id=DCOILWTICO&fq=Daily HTTP/1.0\r\nHost: fred.stlouisfed.org\r\nUser-Agent: ESP8266\r\nConnection: close\r\n\r\n";
const char* TBILL_URL = "GET /graph/fredgraph.csv?cosd=%s&mode=fred&id=DGS10&fq=Daily HTTP/1.0\r\nHost: fred.stlouisfed.org\r\nUser-Agent: ESP8266\r\nConnection: close\r\n\r\n";

//used to process stock selection form submission
//also used to create API GET request
const int GET_REQUEST_BUFFER_SIZE = 1000;
//https client connection timeout for API calls
const int CLIENT_TIMEOUT = 10000; //http read in ms

//where to check for firmware version
const char* FIRMWARE_HOST = "raw.githubusercontent.com";
const char* FW_VERSION_URL = "GET /brownjd/ESPStockTicker/master/data/version.txt";
const char* FW_GET_SUFFIX = " HTTP/1.0\r\nHost: raw.githubusercontent.com\r\nUser-Agent: ESP8266\r\nConnection: close\r\n\r\n";
//OTA update does not support HTTPS, so had to use amazon for firmware hosting

#ifdef ARDUINO_ESP8266_NODEMCU
  const char* FW_BIN_URL = "http://espstockticker.s3-website-us-east-1.amazonaws.com/ESPStockTicker.ino.nodemcu.bin";
#endif
#ifdef ARDUINO_ESP8266_ESP12
  const char* FW_BIN_URL = "http://espstockticker.s3-website-us-east-1.amazonaws.com/ESPStockTicker.ino.adafruit.bin";
#endif

const int MAX_STRINGS = 365;
const int MAX_STRING_LEN = 11;

//this gets reused to hold temp data for the page being rendered
//pay careful attention to the data lengths. 
//it needs to be cleared out before use.
char string_list[MAX_STRINGS][MAX_STRING_LEN];
float price_list[MAX_STRINGS][2]; //can hold to a set of two floats

const int MAX_TICKER_SIZE = 8;
const int TICKER_COUNT = 16;

const int MAX_TBILLS = 365;
const int MAX_DATE_LEN = 11;

const int MAX_OIL_PRICES = 365;

const int MAX_COINS = 32;

//how many minutes between data points to request
const int CHART_INTERVAL = 5;
//9:30 - 4 pm = 390 minutes / increment
const int MAX_CHART_POINTS = 390 / CHART_INTERVAL;

//200 day moving average
float movingAvg = 0.0f;
float yearlow = 0.0f;
float yearhigh = 0.0f;

//TODO: create a settings file
bool SHOW_TBILLS = true;
bool SHOW_OIL = true;
bool SHOW_BITCOIN = true;

//no need to keep these in a temp file since they're small
float coinprice = 0.0;
char coindate[MAX_DATE_LEN];

//re-used and cleared to make API calls for prices and chart info
char requestBuffer [GET_REQUEST_BUFFER_SIZE] = {""};
//for serving configuration pages
ESP8266WebServer httpServer(HTTP_PORT);

//how often to get new data in ms
const int MAX_STOCK_API_INTERVAL = 300000;
const int MAX_FED_API_INTERVAL = 3600000;
const int MAX_COIN_API_INTERVAL = 60000;

//how often to check for new firmware in ms
const int MAX_FW_INTERVAL = 600000;
//how often to switch display pages in ms
const int MAX_PAGE_INTERVAL = 9500;
//how long to remain in AP mode before rebooting
const int MAX_AP_INTERVAL = 120000;

//keep track of when to switch pages
elapsedMillis sincePrint;
//keep track of when to refresh data
elapsedMillis sinceStockAPIUpdate = MAX_STOCK_API_INTERVAL;
elapsedMillis sinceFedAPIUpdate = MAX_FED_API_INTERVAL;
elapsedMillis sinceCoinAPIUpdate = MAX_COIN_API_INTERVAL;

//keep track of when to check for updates
elapsedMillis sinceFWUpdate = MAX_FW_INTERVAL;

ESP8266WiFiMulti wifiMulti;

int page = 0;
bool soft_AP_started = false;
