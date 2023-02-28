#include <Arduino.h>
int analogValue = 0;
int analogVolts = 10;

void setup() {
  // initialize serial communication at 115200 bits per second:
  Serial.begin(9600);
  
  //set the resolution to 12 bits (0-4096)
  analogReadResolution(12);
}

void loop() {
  // read the analog / millivolts value for pin 2:
  analogValue = analogValue + 5;
   analogVolts = analogVolts + 10;
  
  // print out the values you read:
  Serial.printf("ADC analog value = %d\n",analogValue);
  Serial.printf("ADC millivolts value = %d\n",analogVolts);
  
  delay(1000);  // delay in between reads for clear read from serial
}
