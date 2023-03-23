/*

TITRE          : Classe Doorbell
AUTEUR         : Franck Nkeubou Awougang
DATE           : 19/03/2023
DESCRIPTION    : Classe permettant de gérer les opérations d'une sonnette sur une porte.
VERSION        : 0.0.1

*/
 #include "Doorbell.hpp"
#include <Arduino.h>

Doorbell::Doorbell(int pin, bool State)
{
    pinDoor = pin;
    DoorState = State;
    pinMode(pin, OUTPUT);
}

bool Doorbell::isDoorOpen()
{
    return DoorState;
}
void Doorbell::openDoor()
{
    timerStart = millis();
    DoorState = true;
    digitalWrite(pinDoor, LOW);
}
void Doorbell::closeDoor()
{
    DoorState = false;
    digitalWrite(pinDoor, HIGH);
}
bool Doorbell::TimetoClose()
{
    if ((millis() - timerStart) > timerTarget && DoorState){
        digitalWrite(pinDoor, HIGH); 
        DoorState = false;
        return true;
    } 
    return false;
}



