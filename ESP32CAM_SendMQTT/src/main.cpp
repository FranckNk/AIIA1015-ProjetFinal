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


#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "soc/soc.h" //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout problems
#include "esp_http_server.h"
#include "Timer.h"


//const char* ssid = "SMG";
//const char* password = "12345678";
const char* ssid = "FibreOP532";
const char* password = "9PXPE66PM6XM55M8";
const char* mqtt_server = "192.168.2.75";
const char* mqtt_username = "ubuntu";
const char* mqtt_password = "ubuntu";

typedef struct {
        httpd_req_t *req;
        size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"

// This project was tested with the AI Thinker Model

#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_AI_THINKER)
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
#else
  #error "Camera model not selected"
#endif

static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

static size_t jpg_encode_stream(void * arg, size_t index, const void* data, size_t len);
static esp_err_t capture_handler(httpd_req_t *req);
static esp_err_t stream_handler(httpd_req_t *req);
void startCameraServer();
void setup_Cam_parameters();

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

  setup_Cam_parameters();
  //pinMode(PIN_IN, OUTPUT);
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

  startCameraServer();

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

      //digitalWrite(PIN_IN, HIGH);
      client.publish("state/finger","Found ID #"); //client.publish("state/finger", finger.fingerID);
      client.publish("state/finger"," with confidence of "); //client.publish("state/finger",finger.confidence);
      delay(100);
      //digitalWrite(PIN_IN, LOW);
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

static size_t jpg_encode_stream(void * arg, size_t index, const void* data, size_t len){
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if(!index){
        j->len = 0;
    }
    if(httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK){
        return 0;
    }
    j->len += len;
    return len;
}

static esp_err_t capture_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    int64_t fr_start = esp_timer_get_time();

    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    size_t out_len, out_width, out_height;
    uint8_t * out_buf;
    bool s;
    bool detected = false;
    int face_id = 0;
    if(true){
        size_t fb_len = 0;
        if(fb->format == PIXFORMAT_JPEG){
            fb_len = fb->len;
            res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
        } else {
            jpg_chunking_t jchunk = {req, 0};
            res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk)?ESP_OK:ESP_FAIL;
            httpd_resp_send_chunk(req, NULL, 0);
            fb_len = jchunk.len;
        }
        esp_camera_fb_return(fb);
        int64_t fr_end = esp_timer_get_time();
        Serial.printf("JPG: %uB %ums\n", (uint32_t)(fb_len), (uint32_t)((fr_end - fr_start)/1000));
        return res;
    }
}

static esp_err_t stream_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK){
    return res;
  }

  while(true){
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      if(fb->width > 400){
        if(fb->format != PIXFORMAT_JPEG){
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if(!jpeg_converted){
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if(res == ESP_OK){
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if(fb){
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if(_jpg_buf){
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if(res != ESP_OK){
      break;
    }
    //Serial.printf("MJPG: %uB\n",(uint32_t)(_jpg_buf_len));
  }
  return res;
}

void startCameraServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };
  
  httpd_uri_t capture_uri = {
        .uri       = "/capture",
        .method    = HTTP_GET,
        .handler   = capture_handler,
        .user_ctx  = NULL
  };
    
   
    client.publish("state/finger","Starting stream server on port: '%d'\n", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &index_uri);
    }
    
    config.server_port += 1;
    config.ctrl_port += 1;
    client.publish("state/finger", "Starting snapshot server on port: '%d'\n", config.server_port);
    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(camera_httpd, &capture_uri);
    }
}

void setup_Cam_parameters(){
  
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  Serial.setDebugOutput(false);
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;   
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_SVGA; //UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    client.publish("state/finger", "Camera init failed with error 0x");
    return;
  }
}