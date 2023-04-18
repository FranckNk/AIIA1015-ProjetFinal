
/*

TITRE          : Classe OLED - partie code source
AUTEUR         : Franck Nkeubou Awougang
DATE           : 13/04/2023
DESCRIPTION    : Classe pour gérer l'affichage d'un message sur l'écran OLED en utilisant un temps d'affichage
VERSION        : 0.0.1

*/


 #include "OLED.hpp"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void OLED::Init(){
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
		Serial.println(F("SSD1306 allocation failed"));
		for(;;);
	}
	delay(100);
}

void OLED::PrintMessage(String chaine, int time){
    timerStart = millis();
    timerTarget = time;
	display.clearDisplay();
	display.setTextSize(2);
	display.setTextColor(INVERSE);
	display.setCursor(0, 0);
	// Display static text
	display.println("DOORBELL");
	display.setTextSize(1);
	display.setCursor(0, 18);
	display.println(chaine);
	display.display(); 
}


void OLED::TimetoClear()
{
    if ((millis() - timerStart) >= timerTarget){
        // Clear ecran
        timerStart = millis();
        timerTarget = 5000;
        ClearOLED();
    } 
}

void OLED::ClearOLED()
{
    display.clearDisplay();
	display.setTextSize(2);
	display.setTextColor(INVERSE);
	display.setCursor(0, 0);
	// Display static text
	display.println("DOORBELL");
	display.setTextSize(2);
	display.setCursor(0, 19);
	display.println("En Marche...");
	display.display(); 
}