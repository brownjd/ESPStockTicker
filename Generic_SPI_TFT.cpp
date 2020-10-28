#include "Generic_SPI_TFT.h"
#include <SPI.h>

Generic_SPI_TFT::Generic_SPI_TFT(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk, int8_t rst, int8_t miso)
    : Adafruit_SPITFT(0, 0, cs, dc, mosi, sclk, rst, miso) {}

Generic_SPI_TFT::Generic_SPI_TFT(int8_t cs, int8_t dc, int8_t rst)
    : Adafruit_SPITFT(0, 0, cs, dc, rst) {}

void Generic_SPI_TFT::begin(uint32_t freq) {
  initSPI(16000000);
}

void Generic_SPI_TFT::setRotation(unsigned char rot) {
  //do nothing
}

void Generic_SPI_TFT::invertDisplay(bool invert) {
  //do nothing
}

void Generic_SPI_TFT::setAddrWindow(unsigned short x1, unsigned short y1, unsigned short w, unsigned short h) {
  //do nothing
}
