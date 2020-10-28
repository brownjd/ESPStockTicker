/**
 * This class exists solely to query the hardware to see if we're connected to 
 * an ILI9341 or an HX8357 screen. One is 2.4 inches and the other is 3.5.
 * It does not propertly initialize the screen, since we don't know it at runtime.
 */
 #include <Adafruit_SPITFT.h>

class Generic_SPI_TFT : public Adafruit_SPITFT {
public:

  Generic_SPI_TFT(int8_t _CS, int8_t _DC, int8_t _MOSI, int8_t _SCLK, int8_t _RST, int8_t _MISO);
  
  Generic_SPI_TFT(int8_t _CS, int8_t _DC, int8_t _RST = -1);

  void begin(uint32_t freq = 0), setRotation(uint8_t r),
      invertDisplay(boolean i),
      setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
};
