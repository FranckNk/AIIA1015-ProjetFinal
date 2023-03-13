/*

TITRE          : Test de la communication MQTT sur NodeRED
AUTEUR         : Franck Nkeubou Awougang
DATE           : 10/03/2023
DESCRIPTION    : Communication MQTT avec NodeRED à l'aide de Mosquitto installé sur le raspberry Pi
VERSION        : 0.0.1

*/



#include <Arduino.h>
#include "Timer.h"
#include <Wire.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <PubSubClient.h>

//const char* ssid = "FibreOP532";
const char* ssid = "MSI";
//const char* password = "9PXPE66PM6XM55M8";
const char* password = "12345678";
const char* mqtt_server = "192.168.2.75";
const char* mqtt_username = "ubuntu";
const char* mqtt_password = "ubuntu";

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);
Timer temps;
WiFiClient espClient;
PubSubClient client(espClient);
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void MQTTConnect();
uint8_t getFingerprintEnroll();
int getFingerprintIDez();
void Clearfingers();
int PinLED = 4;

void setup() {
  Serial.begin(9600);
  delay(100);
  pinMode(PinLED, OUTPUT);
  Serial.print("Connexion au réseau WiFi ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connecté");
  Serial.println("Adresse IP : ");
  Serial.println(WiFi.localIP());

  MQTTConnect();
  finger.begin(57600);

  if (finger.verifyPassword()) {
    //Serial.println("Found fingerprint sensor!");
  } else {
    client.publish("enroll","Did not find fingerprint sensor :(");
  }
  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else {
      Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  }
  temps.startTimer(100);
}

void loop() {
  if (temps.isTimerReady())
  {
    int id = getFingerprintIDez();
    if (id != -1)
    {
      digitalWrite(PinLED, HIGH);
      Serial.print("Found ID #"); Serial.print(finger.fingerID);
      Serial.print(" with confidence of "); Serial.println(finger.confidence);
      delay(100);
      digitalWrite(PinLED, LOW);
    }
    temps.startTimer(100);
  }
  
  client.loop();
}

void MQTTConnect(){
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Serial.println("Connexion au broker MQTT...");
  while (!client.connected()) {
    if (client.connect("ESP32Client", mqtt_username, mqtt_password )) {
      Serial.println("Connecté au broker MQTT");
    } else {
      Serial.print("Échec de connexion au broker MQTT, code erreur = ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  client.subscribe("test/state");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message reçu : ");
  String data = "";
  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    data += (char)payload[i];
  }
  if(data == "enroll") {
    // Serial.print("\n data reçu = ");Serial.println(data); 
    while (!getFingerprintEnroll());

  }
  
  if(data == "clearfinger") {
    //Serial.print("] \n data reçu = ");
    //Serial.println(data); 
    Clearfingers();
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
  Serial.println("Empreintes supprimées avec succès...");
}

uint8_t getFingerprintEnroll() {
  
  Timer pause;
  int time = 100;
  Serial.println("\n\nAdafruit Fingerprint sensor enrollment");
  // set the data rate for the sensor serial port
  
  //Serial.println(F("Reading sensor parameters"));
  //finger.getParameters();
  // initialise la graine aléatoire avec une valeur différente à chaque exécution
  randomSeed(analogRead(0)); 
  int id = random(100);
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); 
  Serial.println(id);
  pause.startTimer(100);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if(pause.isTimerReady()){
      switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(".");
        delay(100);
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
      }
      pause.startTimer(time);
    } 
  }
  // OK success!

  p = finger.image2Tz(1);

  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  pause.startTimer(100);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if(pause.isTimerReady()){
      switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(".");
        delay(100);
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
      }
      pause.startTimer(time);
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return true;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return true;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return true;
    default:
      Serial.println("Unknown error");
      return true;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    client.publish("enroll","Communication error");
    return true;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
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