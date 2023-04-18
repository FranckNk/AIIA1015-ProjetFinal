/*

TITRE          : Classe Doorbell
AUTEUR         : Franck Nkeubou Awougang
DATE           : 19/03/2023
DESCRIPTION    : Classe permettant de gérer les opérations d'une sonnette sur une porte.
VERSION        : 0.0.1

*/

#ifndef DOORBELL_HPP
    #define DOORBELL_HPP // En même temps, on inclu Arduino.h
    #include <Arduino.h>

class Doorbell
{
private:
    bool DoorState; // to indicate the state of the door
    int pinDoor; // the door pin attached to the relay module
    unsigned long timerStart  = 0; 
    unsigned long timerTarget = 5000; // delay for let door opened
    bool songAlert = false; // state if we need to make noise with the ring

public:
    /**
     * @brief Construct a new Doorbell object
     * 
     * @param pin set pin to the relay module
     * @param State default state of the door
     */
    Doorbell(int pin, bool State);

    /**
     * @brief check the currently door state
     * 
     * @return true if opened
     * @return false if closed
     */
    bool isDoorOpen();

    /**
     * @brief function to open the door independly of the state
     * 
     */
    void openDoor();
    
    /**
     * @brief check if its time to close the door bcs we open the door just for few seconds specified on "timeTarget"
     * 
     * @return true 
     * @return false 
     */
    bool TimetoClose();

    /**
     * @brief Function to close the door independly of the state
     * 
     */
    void closeDoor();

};

#endif
