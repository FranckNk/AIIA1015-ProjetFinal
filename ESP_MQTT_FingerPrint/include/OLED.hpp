/*

TITRE          : Classe OLED
AUTEUR         : Franck Nkeubou Awougang
DATE           : 13/04/2023
DESCRIPTION    : Classe pour gérer l'affichage d'un message sur l'écran OLED en utilisant un temps d'affichage
VERSION        : 0.0.1

*/

#ifndef OLED_HPP
    #define OLED_HPP // En même temps, on inclu Arduino.h
    #include <Arduino.h>
    #include <Wire.h>
    #include <Adafruit_GFX.h>
    #include <Adafruit_SSD1306.h>
    #define SCREEN_WIDTH 128 // OLED display width, in pixels
    #define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
extern Adafruit_SSD1306 display;

class OLED
{
private:
    unsigned long timerStart  = 0;
    unsigned long timerTarget = 2000;

public:
    OLED(){};
    void TimetoClear();
    void PrintMessage(String chaine, int time);
    void ClearOLED();
    void Init();
};

#endif
