#include <Arduino.h>

/*
  # Description:
  # When you push the digital button, the Led 13 on the board will turn off. Otherwise,the led turns on.
*/
          // choose the pin for the LED
int inputPin = 33;               // Connect sensor to input pin 3


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);      // declare LED as output
  pinMode(inputPin, INPUT);     // declare pushbutton as input
}

void loop(){
  int val = digitalRead(inputPin);  // read input value
  if (val == HIGH) {            // check if the input is HIGH
    digitalWrite(LED_BUILTIN, LOW);  // turn LED OFF
  } else {
    digitalWrite(LED_BUILTIN, HIGH); // turn LED ON
  }
}