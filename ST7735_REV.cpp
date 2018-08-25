#include "ST7735_REV.h"

void ST7735_REV::initR(uint8_t options) {
  Adafruit_ST7735::initR(options);
  //tabcolor is private in parent class
  tabcolor = options;
  if (options == INITR_YELLOWTAB) {
    //Serial.println("ST7735_REV.initR(): YELLOWTAB on");
    startWrite();
    writeCommand(ST77XX_MADCTL);
    spiWrite(0xC0);
    endWrite();
  }
}

void ST7735_REV::setRotation(uint8_t m) {
  Adafruit_ST7735::setRotation(m);
  //Serial.print("ST7735_REV.setRotation(): ");
  //Serial.println(m);
  uint8_t madctl = 0;
  if (tabcolor == INITR_YELLOWTAB) {
    //Serial.println("ST7735_REV.setRotation(): YELLOWTAB on");
    switch (rotation) {
      case 0:
        madctl = ST77XX_MADCTL_MX | ST77XX_MADCTL_RGB;
        break;
      case 1:
        madctl = ST77XX_MADCTL_MX | ST77XX_MADCTL_MV | ST77XX_MADCTL_MY | ST77XX_MADCTL_RGB;
        break;

      case 2:
        madctl = ST77XX_MADCTL_MY | ST77XX_MADCTL_RGB;
        break;

      case 3:
        madctl = ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB;
        break;
    }
    startWrite();
    writeCommand(ST77XX_MADCTL);
    spiWrite(madctl);
    endWrite();
  }
}

