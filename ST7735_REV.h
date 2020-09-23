#include "Adafruit_ST7735.h"

#define INITR_YELLOWTAB  0x8 //for the goofy backwards displays
#define INITR_YELLOWTAB_NON_REVERSED  0x9 //for the goofy non-backwards displays

class ST7735_REV : public Adafruit_ST7735 {

 public:
 
 ST7735_REV(int8_t cs, int8_t dc, int8_t sid, int8_t sclk, int8_t rst) : 
  Adafruit_ST7735(cs, dc, sid, sclk, rst) {}
  
 ST7735_REV(int8_t cs, int8_t dc, int8_t rst) : 
  Adafruit_ST7735(cs, dc, rst) {}
  
  void  setRotation(uint8_t m);

  void  initR(uint8_t options = INITR_YELLOWTAB); // for ST7735R

 private:
  uint8_t  tabcolor;
};
