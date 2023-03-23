/*

TITRE          : Classe Doorbell
AUTEUR         : Franck Nkeubou Awougang
DATE           : 19/03/2023
DESCRIPTION    : Classe permettant de gérer les opérations d'une sonnette sur une porte.
VERSION        : 0.0.1

*/

#ifndef DOORBELL_HPP
    #define DOORBELL_HPP // En même temps, on inclu Arduino.h

class Doorbell
{
private:
    bool DoorState;
    int pinDoor;
    unsigned long timerStart  = 0;
    unsigned long timerTarget = 5000;
    bool songAlert = false;

public:
    Doorbell(int pin, bool State);
    bool isDoorOpen();
    void openDoor();
    bool TimetoClose();
    void closeDoor();

};

#endif
