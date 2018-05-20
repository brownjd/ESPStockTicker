#include "ST7735_REV.h"

void ST7735_REV::initR(uint8_t options) {
  Adafruit_ST7735::initR(options);
  if (options == INITR_YELLOWTAB) {
    //tabcolor is private in parent class
    tabcolor = INITR_YELLOWTAB;
    writecommand(ST77XX_MADCTL);
    writedata(0xC0);
  }
}

void ST7735_REV::setRotation(uint8_t m) {
  Adafruit_ST7735::setRotation(m);
  if (tabcolor == INITR_YELLOWTAB) {
    switch (rotation) {
      case 0:
        writedata(ST77XX_MADCTL_MX | ST77XX_MADCTL_RGB);
        break;
      case 1:
        writedata(ST77XX_MADCTL_MX | ST77XX_MADCTL_MV | ST77XX_MADCTL_MY | ST77XX_MADCTL_RGB);
        break;

      case 2:
        writedata(ST77XX_MADCTL_MY | ST77XX_MADCTL_RGB);
        break;

      case 3:
        writedata(ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB);
        break;
    }
  }
}

