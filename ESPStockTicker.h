#define VERSION 2.31

//This supports an ST7735 with a custom
//PCB or an ILI9341 screen using the
//Adafruit Featherwing and Huzzah boards
//NodeMCU ARDUINO_ESP8266_NODEMCU -- this assumes an ST7735 display
//Huzzah ARDUINO_ESP8266_ESP12 -- this assumes an ILI9341 on a Featherwing
//these get set by the IDE at build time when you pick the board

#ifdef ARDUINO_ESP8266_NODEMCU
  #include "ST7735_REV.h"
  #include "Monospaced_plain_11.h"
  #define MONOSPACED_FONT Monospaced_plain_11

  #define HISTORICAL_TITLE_FONT
  #define HISTORICAL_TITLE_POS_X 0
  #define HISTORICAL_TITLE_POS_Y 5
  #define FONT_SIZE 1
  #define HISTORICAL_CHART_FONT_SIZE 1

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

  //list of mac addresses for ESPs soldered to screwed up Ebay screen that print backwards.
  //i call them YELLOWTABS because of they had yellow tabs on the screen protectors
  const int YELLOW_TAB_SIZE = 2;
  const char YELLOW_TABS[YELLOW_TAB_SIZE][18] = { "60:01:94:74:4A:42", "68:C6:3A:9F:B6:61" };

  const int ROTATION_SIZE = 1;
  const char ROTATION_OVERRIDES[ROTATION_SIZE][18] = { "DC:4F:22:1C:2A:98" };

  //Screen Settings
  const int SCREEN_WIDTH = 160;
  const int SCREEN_HEIGHT = 128;
  const int ROW_HEIGHT = 15;
  const int STATUS_MSG_HEIGHT = 8;
  //calculate the number of rows we can fit in usable area
  const int DISPLAY_ROWS = (SCREEN_HEIGHT - STATUS_MSG_HEIGHT) / ROW_HEIGHT;
  
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

  const int HISTORICAL_CHART_VERTICAL_OFFSET = 18;
  
#endif

#ifdef ARDUINO_ESP8266_ESP12
  #include "Adafruit_ILI9341.h"
  #include "Fonts/FreeSans12pt7b.h"
  #define MONOSPACED_FONT FreeSans12pt7b

  #define HISTORICAL_TITLE_FONT &MONOSPACED_FONT
  #define HISTORICAL_TITLE_POS_X 5
  #define HISTORICAL_TITLE_POS_Y 20
  #define FONT_SIZE 1
  #define HISTORICAL_CHART_FONT_SIZE 1
  
  #define TFT_CS   0
  #define TFT_DC   15
  #define TFT_BLACK ILI9341_BLACK
  #define TFT_WHITE ILI9341_WHITE
  #define TFT_GREEN ILI9341_GREEN
  #define TFT_RED ILI9341_RED
  #define TFT_YELLOW ILI9341_YELLOW
  #define TFT_GREY ILI9341_LIGHTGREY

  
  
  //Pricing screen Settings
  #define SCREEN_WIDTH ILI9341_TFTHEIGHT
  #define SCREEN_HEIGHT ILI9341_TFTWIDTH
  const int ROW_HEIGHT = 26;
  const int STATUS_MSG_HEIGHT = 16;
  //calculate the number of rows we can fit in usable area
  const int DISPLAY_ROWS = (SCREEN_HEIGHT - STATUS_MSG_HEIGHT) / ROW_HEIGHT;
  
  //screen is 320 x 256
  //column value is left hand side or start of column
  const int COLUMN_0 = 0;                     // SYMBOL - FIXED
  const int COLUMN_1 = 100;                   // PRICE - FIXED
  const int COLUMN_2 = SCREEN_WIDTH - 120;    // ARROW - FXED
  const int COLUMN_3 = SCREEN_WIDTH - 80;     // % CHANGE - FIXED
  const int COLUMN_4 = SCREEN_WIDTH - 30;     // SYMBOL - SLIDES

  //Chart Settings
  //left side of chart in pixels
  const int CHART_X_ORIGIN = 35;
  //how many pixels between horizontal bars
  const float CHART_X_SPACING = 43;//18.25;
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

  const int HISTORICAL_CHART_VERTICAL_OFFSET = 30;
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
const char* KEY_STATS_FILE = "keystats.json";
const char* TBILL_HIST_FILE = "tbill.csv";
const char* COIN_HIST_FILE = "coinhist.json";
const char* OIL_HIST_FILE = "oilhist.csv";

const float MSG_LINE_SPACING = 1.75;

const char* FW_REMOTE_VERSION_FILE = "/version.remote";

const char* IEX_HOST = "api.iextrading.com";
const char* PRICING_CHART_URL = "GET /1.0/stock/market/batch?filter=latestPrice,changePercent&types=quote&displayPercent=true&symbols=%s HTTP/1.0\r\nHost: api.iextrading.com\r\nUser-Agent: ESP8266\r\nConnection: close\r\n\r\n";
//interval is number of minutes between prices
const char* BASE_CHART_URL = "GET /1.0/stock/%s/chart/1d?chartInterval=%d&filter=average HTTP/1.0\r\nHost: api.iextrading.com\r\nUser-Agent: ESP8266\r\nConnection: close\r\n\r\n";
const char* KEY_STATS_URL = "GET /1.0/stock/%s/stats?filter=week52low,week52high,day200MovingAvg HTTP/1.0\r\nHost: api.iextrading.com\r\nUser-Agent: ESP8266\r\nConnection: close\r\n\r\n";

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
const int CLIENT_TIMEOUT = 3000; //http read in ms

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

//how often to get new data in ms
const int MAX_API_INTERVAL = 60000;
//how often to check for new firmware in ms
const int MAX_FW_INTERVAL = 600000;
//how often to switch display pages in ms
const int MAX_PAGE_INTERVAL = 9500;
//how long to remain in AP mode before rebooting
const int MAX_AP_INTERVAL = 120000;

const int MAX_STRINGS = 365;
const int MAX_STRING_LEN = 11;

//this gets reused to hold temp data for the page being rendered
//pay careful attention to the data lengths. 
//it needs to be cleared out before use.
char string_list[MAX_STRINGS][MAX_STRING_LEN];
float price_list[MAX_STRINGS][2]; //can hold to a set of two floats

const int MAX_TICKER_SIZE = 8;
const int TICKER_COUNT = 16;

const char* TBILL_LABEL = "10 Year TBill";
const int MAX_TBILLS = 365;
const int MAX_DATE_LEN = 11;

const char* OIL_LABEL = "Oil Price";
const int MAX_OIL_PRICES = 365;

const char* COIN_LABEL = "Bitcoin Price";
const int MAX_COINS = 32;

//how many minutes between data points to request
const int CHART_INTERVAL = 2;
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
//keep track of when to switch pages
elapsedMillis sincePrint;
//keep track of when to refresh data
elapsedMillis sinceAPIUpdate = MAX_API_INTERVAL;
//keep track of when to check for updates
elapsedMillis sinceFWUpdate = MAX_FW_INTERVAL;

ESP8266WiFiMulti wifiMulti;

int page = 0;
bool soft_AP_started = false;
