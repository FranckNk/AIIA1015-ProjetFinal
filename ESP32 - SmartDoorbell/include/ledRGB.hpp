#ifndef LedRGB_hpp
#define LedRGB_hpp

#include "Arduino.h"

class LedRGB {
  private:
    int redPin; // pin used for define Red pin on the LED RGB
    int greenPin; // pin used for define green pin on the LED RGB
    int bluePin; // pin used for define blue pin on the LED RGB

  public:
    /**
     * @brief Construct a new Led R G B object
     * 
     * @param r set red pin
     * @param g set green pin
     * @param b set blue pin
     */
    LedRGB(int r, int g, int b);

    /**
     * @brief Set the specific Color on the led
     * 
     * @param redValue set the value for led color
     * @param greenValue set the value for green color
     * @param blueValue set the value fro blue color
     */
    void setColor(int redValue, int greenValue, int blueValue);

    /**
     * @brief Set the Red color on the LED RGB
     * 
     */
    void setRed();

    /**
     * @brief Set the Green color on the led
     * 
     */
    void setGreen();

    /**
     * @brief Set the Blue color on the led
     * 
     */
    void setBlue();

    /**
     * @brief Set the White color
     * 
     */
    void setWhite();
    
    /**
     * @brief Set the Purple color
     * 
     */
    void setPurple();
};

#endif /* LedRGB_hpp */