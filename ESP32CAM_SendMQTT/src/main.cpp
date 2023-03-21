/*

TITRE          : Test de la communication MQTT sur NodeRED
AUTEUR         : Franck Nkeubou Awougang
DATE           : 10/03/2023
DESCRIPTION    : Communication MQTT avec NodeRED à l'aide de Mosquitto installé sur le raspberry Pi
VERSION        : 0.0.1

*/



#include <Arduino.h>
#include "Timer.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>

const char* ssid = "MSI";
const char* password = "12345678";
// const char* ssid = "FibreOP532";
// const char* password = "9PXPE66PM6XM55M8";
const char* mqtt_server = "192.168.2.75";
const char* mqtt_username = "ubuntu";
const char* mqtt_password = "ubuntu";

Timer temps;
WiFiClient espClient;
PubSubClient client(espClient);
HardwareSerial mySerial(0);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

const char* topic1 = "state/motion";
int PIN_IN = 33;
int tempspause = 1000;
bool state = false;

uint8_t getFingerprintEnroll();
//Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
int getFingerprintIDez();
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void MakeAction(String data);
void MQTTConnect();

void setup() {
  Serial.begin(9600);
  delay(100);
  pinMode(PIN_IN, OUTPUT);
  Serial.println();
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
  client.publish("state/finger","\n\nAdafruit Fingerprint sensor enrollment in Setup");

  // set the data rate for the sensor serial port
  finger.begin(57600);

  if (finger.verifyPassword()) {
    client.publish("state/finger","Found fingerprint sensor!");
  } else {
    client.publish("state/finger","Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }
  client.publish("state/finger", "Reading sensor parameters");
  finger.getParameters();

  temps.startTimer(tempspause);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }

  if (state)
  {
    //digitalWrite(PIN_IN, state);
    state = !state;
    /*
    Serial.println("loop");
    client.publish("test", "Hello from ESP32, loop");
    Serial.println("Hello from ESP32, loop");
    client.subscribe("test/state");
    */
    while (!getFingerprintEnroll());
    temps.startTimer(tempspause);
  }

  int id = getFingerprintIDez();
  if (id != -1)
    {
        //while (!getFingerprintEnroll());

      digitalWrite(PIN_IN, HIGH);
      client.publish("state/finger","Found ID #"); //client.publish("state/finger", finger.fingerID);
      client.publish("state/finger"," with confidence of "); //client.publish("state/finger",finger.confidence);
      delay(100);
      digitalWrite(PIN_IN, LOW);
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
  //client.publish("update/features","door_on");
  //client.publish("update/features","motion_off");
  //client.publish("update/features","ring_off");
  client.subscribe(topic1);    

}

void callback(char* topic, byte* payload, unsigned int length) {
  //client.publish("state/finger","Message reçu : ");
   String data = "";
  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    data += (char)payload[i];
  }
  Serial.println(data);
  MakeAction(data);
  //Serial.println(data);
}
void MakeAction(String data){
  if(data == "enroll") {
    // Serial.print("\n data reçu = ");Serial.println(data); 
    state = true;
  }

}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentative de reconnexion au broker MQTT...");
    if (client.connect("ESP32Client", mqtt_username, mqtt_password )) {
      Serial.println("Connecté");
      client.subscribe("test");
      client.publish("test", "Hello from ESP32, reconnect");
    } else {
      Serial.print("Échec de connexion au broker MQTT, code erreur = ");
      Serial.print(client.state());
      
    }
  }
}


uint8_t getFingerprintEnroll() {
  
  Timer pause;
  int time = 100;
  client.publish("state/finger", "\n\nGet fingerprint Enroll");
  // set the data rate for the sensor serial port
  
  //Serial.println(F("Reading sensor parameters"));
  //finger.getParameters();
  // initialise la graine aléatoire avec une valeur différente à chaque exécution
  //randomSeed(analogRead(0)); 
  int id = 5;
  int p = -1;
  client.publish("state/finger","Waiting for valid finger to enroll as #"); 
  //Serial.println(id);
  pause.startTimer(100);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if(pause.isTimerReady()){
      switch (p) {
      case FINGERPRINT_OK:
        client.publish("state/finger", "Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        client.publish("state/finger",".");
        delay(100);
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        client.publish("state/finger", "Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        client.publish("state/finger", "Imaging error");
        break;
      default:
        client.publish("state/finger", "Unknown error");
        break;
      }
      pause.startTimer(time);
    } 
  }
  // OK success!

  p = finger.image2Tz(1);

  switch (p) {
    case FINGERPRINT_OK:
      client.publish("state/finger", "Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      client.publish("state/finger", "Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      client.publish("state/finger", "Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      client.publish("state/finger", "Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      client.publish("state/finger", "Could not find fingerprint features");
      return p;
    default:
      client.publish("state/finger", "Unknown error");
      return p;
  }

  client.publish("state/finger", "Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  client.publish("state/finger","ID "); //Serial.println(id);
  p = -1;
  client.publish("state/finger", "Place same finger again");
  pause.startTimer(100);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if(pause.isTimerReady()){
      switch (p) {
      case FINGERPRINT_OK:
        client.publish("state/finger", "Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        client.publish("state/finger",".");
        delay(100);
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        client.publish("state/finger", "Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        client.publish("state/finger", "Imaging error");
        break;
      default:
        client.publish("state/finger", "Unknown error");
        break;
      }
      pause.startTimer(time);
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      client.publish("state/finger", "Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      client.publish("state/finger", "Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      client.publish("state/finger", "Communication error");
      return true;
    case FINGERPRINT_FEATUREFAIL:
      client.publish("state/finger", "Could not find fingerprint features");
      return true;
    case FINGERPRINT_INVALIDIMAGE:
      client.publish("state/finger", "Could not find fingerprint features");
      return true;
    default:
      client.publish("state/finger", "Unknown error");
      return true;
  }

  // OK converted!
  client.publish("state/finger","Creating model for #");//  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    client.publish("state/finger", "Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    client.publish("state/finger", "Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    client.publish("state/finger", "Fingerprints did not match");
    return p;
  } else {
    client.publish("state/finger", "Unknown error");
    return p;
  }

  client.publish("state/finger","ID "); //Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    client.publish("state/finger", "Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    client.publish("enroll","Communication error");
    return true;
  } else if (p == FINGERPRINT_BADLOCATION) {
    client.publish("state/finger", "Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    client.publish("state/finger", "Error writing to flash");
    return p;
  } else {
    client.publish("state/finger", "Unknown error");
    return p;
  }
  client.publish("state/finger", "Enrollement terminé");
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