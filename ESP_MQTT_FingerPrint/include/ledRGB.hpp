#ifndef LedRGB_hpp
#define LedRGB_hpp

#include "Arduino.h"

class LedRGB {
  private:
    int redPin;
    int greenPin;
    int bluePin;

  public:
    LedRGB(int r, int g, int b);
    void setColor(int redValue, int greenValue, int blueValue);
    void setRed();
    void setGreen();
    void setBlue();
    void setWhite();
    void setPurple();
};

#endif /* LedRGB_hpp */