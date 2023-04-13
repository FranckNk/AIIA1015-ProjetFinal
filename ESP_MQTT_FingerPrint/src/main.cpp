/*

TITRE          : Test de la communication MQTT sur NodeRED
AUTEUR         : Franck Nkeubou Awougang
DATE           : 10/03/2023
DESCRIPTION    : Communication MQTT avec NodeRED à l'aide de Mosquitto installé sur le raspberry Pi
VERSION        : 0.0.1

*/



#include "Doorbell.hpp"
#include "Timer.h"
#include <Wire.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "ledRGB.hpp"
#include  "OLED.hpp"

//Wifi credentials
//const char* ssid = "FibreOP532";
const char* ssid = "MSI";
//const char* password = "9PXPE66PM6XM55M8";
const char* password = "12345678";
// MQTT credentials
//const char* ssid = "UNIFI_IDO1";
//const char* password = "42Bidules!";
const char* mqtt_server = "192.168.137.100";
//const char* mqtt_server = "192.168.2.75";
//const char* mqtt_server = "192.168.2.23";
const char* mqtt_username = "openhabian";
const char* mqtt_password = "openhabian";

const char* topic1 = "state/finger";
//const char* topic2 = "state/motion";
const char* topic3 = "state/ring";
const char* topic4 = "state/door";
const char* topic5 = "value/motion";
const char* topic6 = "update/features";
const char* topic7 = "fingers/number";

//IPAddress localIP;
IPAddress localIP(192, 168, 137, 200); // hardcoded
// Set your Gateway IP address
IPAddress localGateway(192, 168, 137, 1);
//IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 255, 0);


Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);
Timer temps;
Timer tempsMotion;
Timer tempsRing;
LedRGB led(32, 25, 33);
Doorbell bell = Doorbell(2, false);
WiFiClient espClient;
PubSubClient client(espClient);
OLED maOLED = OLED();


// functions declarations
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void MQTTConnect();
uint8_t getFingerprintEnroll();
int getFingerprintIDez();
void Clearfingers();
void MakeAction(String data);
void WifiConnect();

const int channel = 0;
const int freq = 100; // fréquence de la tonalité en Hz
const int resolution = 8; // résolution en bits du signal PWM
int motionPIN = 23;               // choose the input pin (for PIR sensor)
int pirState = HIGH;             // we start, assuming no motion's not detected
int inputSonette = 19;       // pin that get if someone ring the bell. Used for making noise buzzer
int pinBuzzer = 26;
bool makeBuzzerNoise = true;  // Start with noise buzzer
short int delayEvent = 2000;
//int pinLED = 4;

// function for oppening the door and update database via mqtt on nodeRed
// void openDoor(); activate relay

void setup() {
  Serial.begin(9600);
  delay(100);
  pinMode(pinBuzzer, OUTPUT);
  pinMode(inputSonette, INPUT);
  //pinMode(pinLED, OUTPUT);
  pinMode(motionPIN, INPUT);
  // Configurer le canal LEDC 0 avec la fréquence de base de 5 kHz et la résolution de 8 bits
  ledcSetup(channel, freq, resolution);
  // Associer le canal LEDC 0 à la broche GPIO 2
  ledcAttachPin(pinBuzzer, channel);
	maOLED.Init();
	maOLED.PrintMessage("Bonjour Tout le monde", 3000); 

  WifiConnect();
  MQTTConnect();
  finger.begin(57600);
  if (finger.verifyPassword()) {
    //Serial.println("Found fingerprint sensor!");
  } else {
    client.publish("enroll","Did not find fingerprint sensor :(");
  }
  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    client.publish("enroll","Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else {
    //Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
    client.publish(topic7,String(finger.templateCount).c_str());
  }
  // Mise à jour du nombre d'empreintes sur le dashboard

  temps.startTimer(100);
  tempsMotion.startTimer(delayEvent);
  tempsRing.startTimer(delayEvent);
  led.setGreen();
}


void loop() {
	maOLED.TimetoClear();
  // Vérification s'il est temps d'ouvrir la porte. 
  if (bell.TimetoClose() && !bell.isDoorOpen())
  {
    client.publish(topic6,"door_off");
    led.setRed();
  }
  
  if (temps.isTimerReady())
  {
    int id = getFingerprintIDez();
    if (id != -1)
    {
      bell.openDoor();
      led.setGreen();
      client.publish(topic6,"door_on");
      maOLED.PrintMessage("Door opened", 2000);
      //Serial.print("Found ID #"); Serial.print(finger.fingerID);
      //Serial.print(" with confidence of "); Serial.println(finger.confidence);
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
  
  if(stateMotion == HIGH){
    // Motion detected
    //digitalWrite(pinLED, HIGH);
    if (pirState == LOW) 
	  {
      //Serial.println("Motion detected!");	// print on output change
      client.publish(topic5,"motion");
      led.setBlue();
      pirState = HIGH;
    }
    if(tempsMotion.isTimerReady()){
      tempsMotion.startTimer(delayEvent);
      // send state to mqtt
    }
  }else if (stateMotion == LOW){
    // motion finished
    //digitalWrite(pinLED, LOW);
    if (pirState == HIGH)
	  {
      //Serial.println("Motion ended!");	// print on output change
      led.setRed();
      pirState = LOW;
    }
  }
  client.loop();
}

void WifiConnect(){
  maOLED.PrintMessage("Connexion au réseau WiFi ",2000);
  //maOLED.PrintMessage(ssid, 2000);
  //configure ESP to get static IP
  if (!WiFi.config(localIP, localGateway, subnet)){
    maOLED.PrintMessage("STA Failed to configure", 2000);
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    //maOLED.PrintMessage(".", 2000);
  }
  //maOLED.PrintMessage("", 2000);
  maOLED.PrintMessage("WiFi connecté..", 2000);
  //maOLED.PrintMessage("Adresse IP : ");
  //maOLED.PrintMessage(WiFi.localIP());
  //maOLED.PrintMessage("Gateway IP : ");
  //maOLED.PrintMessage(WiFi.gatewayIP());

}

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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message reçu : ");
  String data = "";
  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    data += (char)payload[i];
  }
  Serial.println(data);
  MakeAction(data);
}



void MakeAction(String data){
  if(data == "enroll") {
    // Serial.print("\n data reçu = ");Serial.println(data); 
    while (!getFingerprintEnroll());
    finger.getTemplateCount();
    client.publish(topic7,String(finger.templateCount).c_str());
  }
  if(data == "clearfinger") {
    //Serial.print("] \n data reçu = ");
    //Serial.println(data); 
    finger.getTemplateCount();
    client.publish(topic7,String(finger.templateCount).c_str());
    //PrintMessage("Empreintes effacées...");
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

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentative de reconnexion au broker MQTT...");
    if (client.connect("ESP32Client", mqtt_username, mqtt_password )) {
      Serial.println("Connecté");
      client.subscribe("test");
      //client.publish("test", "Hello from ESP32, reconnect");
    } else {
      Serial.print("Échec de connexion au broker MQTT, code erreur = ");
      Serial.print(client.state());
      
    }
  }
}

void Clearfingers(){
  
  Timer pause;
  int time = 100;
  finger.emptyDatabase();
  client.publish("enroll","Empreintes supprimées avec succès...");
  //Serial.println("Empreintes supprimées avec succès...");
}

uint8_t getFingerprintEnroll() {
  
  Timer pause;
  int time = 100;
  maOLED.PrintMessage("\nEnrollment...", 2000);
  // set the data rate for the sensor serial port
  
  //Serial.println(F("Reading sensor parameters"));
  //finger.getParameters();
  // initialise la graine aléatoire avec une valeur différente à chaque exécution
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