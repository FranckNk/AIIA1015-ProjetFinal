#include "ledRGB.hpp"

LedRGB::LedRGB(int r, int g, int b) {
  redPin = r;
  greenPin = g;
  bluePin = b;
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
}

void LedRGB::setColor(int redValue, int greenValue, int blueValue) {
  analogWrite(redPin, redValue);
  analogWrite(greenPin, greenValue);
  analogWrite(bluePin, blueValue);
}

void LedRGB::setRed() {
  setColor(255, 0, 0);
}

void LedRGB::setGreen() {
  setColor(0, 255, 0);
}

void LedRGB::setBlue() {
  setColor(0, 0, 255);
}

void LedRGB::setWhite() {
  setColor(255, 255, 255);
}

void LedRGB::setPurple() {
  setColor(170, 0, 255);
}