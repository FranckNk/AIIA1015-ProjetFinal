#include <Arduino.h>

const int channel = 0;
const int freq = 100; // fréquence de la tonalité en Hz
const int resolution = 8; // résolution en bits du signal PWM
int motionPIN = 23;               // choose the input pin (for PIR sensor)
int pirState = HIGH;             // we start, assuming no motion's not detected
int inputSonette = 18; 
int pinBuzzer = 26;

void setup() {
  pinMode(pinBuzzer, OUTPUT);
  pinMode(inputSonette, INPUT);
  pinMode(motionPIN, INPUT);
  // Configurer le canal LEDC 0 avec la fréquence de base de 5 kHz et la résolution de 8 bits
  ledcSetup(channel, freq, resolution);
  // Associer le canal LEDC 0 à la broche GPIO 2
  ledcAttachPin(pinBuzzer, channel);
}

void loop() {
  // Générer une tonalité de 1 kHz pendant 1 seconde
  int val = digitalRead(inputSonette);
  int stateMotion = digitalRead(motionPIN);
  if(val == HIGH){
    ledcWriteTone(channel, freq); 
    delay(50);
  }
  else{
    ledcWrite(channel, 0);
  }
  if(stateMotion == HIGH && pirState == LOW){
    // Motion detected
      pirState = HIGH;
  }else if (stateMotion == LOW && pirState == HIGH){
    // motion finished
    pirState = HIGH;
  }

}