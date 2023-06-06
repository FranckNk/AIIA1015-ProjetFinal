/*

TITRE          : Smart doorbell code using mqtt communication to interact with NodeRED server.
AUTEUR         : Franck Nkeubou Awougang
DATE           : 10/03/2023
DESCRIPTION    : Communication MQTT avec NodeRED à l'aide de Mosquitto installé sur le raspberry Pi
VERSION        : 0.0.1

*/

#include "Doorbell.hpp"
#include "Timer.h"
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "ledRGB.hpp"
#include  "OLED.hpp"

//Wifi credentials
//const char* ssid = "FibreOP532";
//const char* ssid = "MSI";
//const char* password = "9PXPE66PM6XM55M8";
//const char* password = "12345678";
//const char* ssid = "UNIFI_IDO1";
//const char* password = "42Bidules!";
const char* ssid = "TELUSBDC8B4_2.4G";
const char* password = "6QBR55Z682";

// MQTT credentials
//const char* mqtt_server = "192.168.137.100";
//const char* mqtt_server = "192.168.2.75";
const char* mqtt_server = "192.168.0.200";
const char* mqtt_username = "openhabian";
const char* mqtt_password = "openhabian";

// Topics crédentials
const char* topic1 = "state/finger";
//const char* topic2 = "state/motion";
const char* topic3 = "state/ring";
const char* topic4 = "state/door";
const char* topic5 = "value/motion";
const char* topic6 = "update/features";
const char* topic7 = "fingers/number";

//IPAddress localIP fixed;
IPAddress localIP(192, 168, 1, 210); // IP Address déclarées de manière statique
// Set your Gateway IP address
IPAddress localGateway(192, 168, 137, 1);
//IPAddress localGateway(192, 168, 1, 1); //IP Address déclarées de manière statique
IPAddress subnet(255, 255, 255, 0);

// Déclaration des variables
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);
Timer temps;
Timer tempsMotion;
Timer tempsRing;
LedRGB led(32, 33, 25);
Doorbell bell = Doorbell(2, false);
WiFiClient espClient;
PubSubClient client(espClient);
OLED maOLED = OLED();


// Functions declarations
void callback(char* topic, byte* payload, unsigned int length);
void MQTTConnect();
uint8_t getFingerprintEnroll();
int getFingerprintIDez();
void Clearfingers();
void MakeAction(String data);
void WifiConnect(bool dhcpaddress);

const int channel = 0; // variable pour utilisation du buzzer
const int freq = 100; // fréquence de la tonalité en Hz
const int resolution = 8; // résolution en bits du signal PWM
int motionPIN = 23;               // choose the input pin (for PIR sensor)
int pirState = HIGH;             // we start, assuming no motion's not detected
int inputSonette = 19;       // pin that get if someone ring the bell. Used for making noise buzzer
int pinBuzzer = 26;
bool makeBuzzerNoise = true;  // Start with noise buzzer
short int delayEvent = 2000; // Délai entre les évènements
//int pinLED = 4;

// function for oppening the door and update database via mqtt on nodeRed
// void openDoor(); activate relay

void setup() {

  //Serial.begin(9600); // Start serial monitor
  delay(100);

  
  //Configuration des broches nécessaires
  pinMode(pinBuzzer, OUTPUT);
  pinMode(inputSonette, INPUT);
  pinMode(motionPIN, INPUT);

  // Configurer le canal LEDC 0 avec la fréquence de base de 5 kHz et la résolution de 8 bits
  ledcSetup(channel, freq, resolution);

  // Associer le canal LEDC 0 à la broche GPIO 2
  ledcAttachPin(pinBuzzer, channel);

  led.setColor(241, 238, 0); // set color yellow to indicate that we are trying to connect to wifi and mqtt

	maOLED.Init();
	maOLED.PrintMessage("Welcome !", 3000); 

  WifiConnect(false);
  MQTTConnect();

  // Initialise finger print sensor
  finger.begin(57600);
  if (finger.verifyPassword()) {
    //Serial.println("Found fingerprint sensor!");
  } else {
    client.publish("enroll","Did not find fingerprint sensor :(");
    maOLED.PrintMessage("Did not find fingerprint sensor :(", 2000);
  }
  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    // Mise à jour du nombre d'empreintes sur le dashboard
    client.publish(topic7,String(finger.templateCount).c_str());
    client.publish("enroll","Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else {
    // Mise à jour du nombre d'empreintes sur le dashboard
    client.publish(topic7,String(finger.templateCount).c_str());
  }

  temps.startTimer(100); // delay for making any action in the loop. 
  tempsMotion.startTimer(delayEvent);
  tempsRing.startTimer(delayEvent);
  led.setGreen(); // Say evreting is okay at this state
}


void loop() {
	maOLED.TimetoClear();
  // Vérification s'il est temps d'ouvrir la porte. 
  if (bell.TimetoClose() && !bell.isDoorOpen())
  {
    client.publish(topic6,"door_off");
    client.publish("enroll","Fermeture de la porte");
    led.setRed();
  }
  
  if (temps.isTimerReady()) // if we can make action
  {
    int id = getFingerprintIDez();
    if (id != -1)
    {
      bell.openDoor();
      led.setGreen();
      client.publish(topic6,"door_on");
      client.publish("enroll","Ouverture de la porte");
      maOLED.PrintMessage("Door opened", 5000);
    }
    temps.startTimer(100);
  }

  // Générer une tonalité de 1 kHz pendant 1 seconde
  int val = digitalRead(inputSonette);
  int stateMotion = digitalRead(motionPIN);

  // if we can make noise buzzer
  if(val == HIGH && makeBuzzerNoise){
    if(tempsRing.isTimerReady()){
      client.publish(topic5,"ring");
      tempsRing.startTimer(delayEvent);
      // send state to mqtt
    }
    ledcWriteTone(channel, freq); 
    delay(50);
  }
  else{
    ledcWrite(channel, 0);
  }
  // If motion is detected
  if(stateMotion == HIGH){
    // Motion detected
    //digitalWrite(pinLED, HIGH);
    if (pirState == LOW) 
	  {
      client.publish(topic5,"motion");
      led.setBlue();
      pirState = HIGH;
    }
    /*
    if(tempsMotion.isTimerReady()){
      tempsMotion.startTimer(delayEvent);
      // send state to mqtt
    }
    */
  }else if (stateMotion == LOW){
    // motion finished
    if (pirState == HIGH)
	  {
      led.setRed();
      pirState = LOW;
    }
  }
  client.loop();
}

/**
 * @brief Fonction pour se connecter au wifi local en utilisant les crédentials ssid, et password
 * 
 * @param dhcpaddress boolean to set (true) if we use static ip or (false) for dynamic ip address
 */
void WifiConnect(bool dhcpaddress){
  maOLED.PrintMessage("Connexion au réseau WiFi ",2000);

  if(dhcpaddress){
    //maOLED.PrintMessage(ssid, 2000);
    //configure ESP to get static IP
    if (!WiFi.config(localIP, localGateway, subnet)){
      maOLED.PrintMessage("STA Failed to configure", 2000);
    }
  }
  // connect to wifi using parameters needed
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
  }
  maOLED.PrintMessage("WiFi connecté..", 2000);

}

/**
 * @brief Function to connect to MQTT Broker using mqtt credentials
 * 
 */
void MQTTConnect(){
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  maOLED.PrintMessage("broker MQTT...", 2000);
  while (!client.connected()) {
    if (client.connect("ESP32Client", mqtt_username, mqtt_password )) {
      maOLED.PrintMessage("Connecté au broker MQTT", 5000);
    } else {
      maOLED.PrintMessage("Failed broker MQTT..", 2000);
      //Serial.print(client.state());
      delay(2000);
    }
  }
  // Reinitialise all features bcs the device restarted
  client.publish(topic6,"start");
  client.publish(topic6,"door_off");
  client.publish(topic6,"motion_off");
  client.publish(topic6,"ring_on"); // Start with noise buzzer song on the device
  client.publish(topic6,"email_off");
  client.subscribe(topic1);    
  client.subscribe(topic3);    
  client.subscribe(topic4);
}

/**
 * @brief Function who called when we received a mqtt message from broker after subscribed.
 * 
 * @param topic topic used to send message to us (this device)
 * @param payload message content. it contents data sent from broker
 * @param length size of message received from broker
 */
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message reçu : ");
  String data = "";
  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    data += (char)payload[i];
  }
  Serial.println(data);
  // lets make a action depends of message received
  MakeAction(data);
}


/**
 * @brief Fonction utilisée pour traduire le message reçu du broker mqtt.
 * 
 * @param data messange reçu
 */
void MakeAction(String data){
  if(data == "enroll") {
    while (!getFingerprintEnroll());
    finger.getTemplateCount();
    client.publish(topic7,String(finger.templateCount).c_str());
  }
  if(data == "clearfinger") {
    finger.getTemplateCount();
    client.publish(topic7,String(finger.templateCount).c_str());
    Clearfingers();
  }
  if(data == "door_on"){
    bell.openDoor();
    led.setGreen();
  }
  if(data == "door_off"){
    bell.closeDoor();
    led.setRed();
  }
  if(data == "ring_on"){
    makeBuzzerNoise = true;
  }
  if(data == "ring_off"){
    makeBuzzerNoise = false;
  }
}

/**
 * @brief function used for delete all fingerprints saved on the sensor
 * 
 */
void Clearfingers(){
  
  Timer pause;
  int time = 100;
  finger.emptyDatabase();
  client.publish("enroll","Empreintes supprimées avec succès...");
  //Serial.println("Empreintes supprimées avec succès...");
}

/**
 * @brief Get the Fingerprint Enroll object
 * 
 * @return uint8_t 
 */
uint8_t getFingerprintEnroll() {
  
  Timer pause;
  int time = 100;
  maOLED.PrintMessage("\nEnrollment...", 2000);
  // set the data rate for the sensor serial port
  
  //Serial.println(F("Reading sensor parameters"));
  //finger.getParameters();
  // initialise la graine aléatoire avec une valeur différente à chaque exécution 
  // bcs we need to set a id on a finger in the sensor memory
  randomSeed(analogRead(0)); 
  int id = random(100);
  int p = -1;
  maOLED.PrintMessage("\nEnrollment...\nPut your finger", 2000);
  //Serial.print("Waiting for valid finger to enroll as #"); 
  Serial.println(id);
  pause.startTimer(100);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if(pause.isTimerReady()){
      switch (p) {
      case FINGERPRINT_OK:
        //Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        //Serial.print(".");
        delay(100);
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        //Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        //Serial.println("Imaging error");
        break;
      default:
        //Serial.println("Unknown error");
        break;
      }
      pause.startTimer(time);
    } 
  }
  // OK success!

  p = finger.image2Tz(1);

  switch (p) {
    case FINGERPRINT_OK:
     // Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      //Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
     // Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      //Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
     // Serial.println("Could not find fingerprint features");
      return p;
    default:
      //Serial.println("Unknown error");
      return p;
  }

  //Serial.println("Remove finger");
  maOLED.PrintMessage("Remove Finger and replace same finger...", 2000);
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  //Serial.print("ID "); Serial.println(id);
  p = -1;
  //Serial.println("Place same finger again");
  pause.startTimer(100);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if(pause.isTimerReady()){
      switch (p) {
      case FINGERPRINT_OK:
        //Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        //Serial.print(".");
        delay(100);
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        //Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        //Serial.println("Imaging error");
        break;
      default:
        //Serial.println("Unknown error");
        break;
      }
      pause.startTimer(time);
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      //Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      //Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("Communication error");
      return true;
    case FINGERPRINT_FEATUREFAIL:
      //Serial.println("Could not find fingerprint features");
      return true;
    case FINGERPRINT_INVALIDIMAGE:
      //Serial.println("Could not find fingerprint features");
      return true;
    default:
      //Serial.println("Unknown error");
      return true;
  }

  // OK converted!
  //Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    //Serial.println("Prints matched!");
    maOLED.PrintMessage("Prints matched!..", 2000);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    //Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    maOLED.PrintMessage("Prints didnt matched!..", 2000);
    //Serial.println("Fingerprints did not match");
    return p;
  } else {
    //Serial.println("Unknown error");
    return p;
  }

  //Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
   // Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    client.publish("enroll","Communication error");
    return true;
  } else if (p == FINGERPRINT_BADLOCATION) {
    //Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    //Serial.println("Error writing to flash");
    return p;
  } else {
    //Serial.println("Unknown error");
    return p;
  }
  client.publish("enroll", "Enrollement terminé");
  return true;

}

/**
 * @brief Get the Fingerprint I Dez object
 * 
 * @return int 
 */
int getFingerprintIDez() {
  //Serial.println("\n\nAdafruit Fingerprint sensor enrollment");
  // set the data rate for the sensor serial port
  
  //Serial.println("Waiting for valid finger...");
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  return finger.fingerID;
}