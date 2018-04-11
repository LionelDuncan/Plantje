#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Button.h>        //https://github.com/JChristensen/Button

#include <ESP8266WiFi.h>
//#include <ESP8266mDNS.h>
//#include <WiFiUdp.h>
//#include <ArduinoOTA.h>

#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Servo.h>
//#include "SSD1306.h"
#include <neotimer.h>

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C

#define OLED_RESET D5
Adafruit_SSD1306 display(OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2
#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

const int interval = 4000; // time water in automode

const char* ssid = "test";
const char* password =  "testtest123";

const char* mqttServer = "m23.cloudmqtt.com";
const int mqttPort = 16647;
const char* mqttUser = "esp";
const char* mqttPassword = "lionel123";

//sensorvalues
float temp;
float pres;
float hum;
float alt;
int ldr;
int mois;

bool automode = 0; //manual
bool waterplant = false;
//Sensors Sensors;

WiFiClient espClient;
PubSubClient client(espClient);
Servo myservo;

Neotimer servotimer = Neotimer(4000);
Neotimer updatetimer = Neotimer(5000);
Neotimer wifitimer = Neotimer(500);
Neotimer mqtttimer = Neotimer(2000);
Neotimer displaytimer = Neotimer(100);
Neotimer ldrtimer = Neotimer(100);
Neotimer startuptimer = Neotimer(2000);
Neotimer watertimer = Neotimer(4000);

Button flash = Button(D3, true, true, 25);

int counter = 1;
int lastReset;
unsigned long lastWatering;

/*class Display {
  private:
    Sensors sensors;
  public:
    void setup() {
      display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize the I2C addr 0x3C (for the 128x64)
      display.display();
    }

    void initiateWater() {
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.println(F("Watering the plant"));
    }
    
    void loop() {
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.println(F(sensors.temperature() + " *C"));
      display.setCursor(0, 5);
      display.println(F(sensors.pressure() + " hPa"));
      display.setCursor(0, 10);
      display.println(F(sensors.humidity() + " %"));
      display.display();
    }

};*/

/*#define DEMO_DURATION 3000
typedef void (*Demo)(void);

int demoMode = 0;
int counter = 1;
*/
// https://techtutorialsx.com/2017/04/09/esp8266-connecting-to-mqtt-broker/
// gitkraken test

unsigned long wifidelay = 0;
unsigned long bmedelay = 0;
unsigned long clientdelay = 0;
unsigned long lastReconnectAttempt = 0;
unsigned long setupdelay = 0;

bool statuss;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Wire.begin(D7, D6);

  statuss = bme.begin();
  bmedelay = millis();

  if( millis() - bmedelay > 200){
    if(!statuss){
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      bmedelay = millis();
    }
  }
  
  pinMode(D2, OUTPUT);
  pinMode(A0, INPUT);
  pinMode(D0, OUTPUT);
  
  // oled display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  
  //Serial.println("Connected to the WiFi network");
  
  display.setTextSize(1);
  display.setTextColor(WHITE);

  myservo.attach(D8);

  setup_wifi();

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  //arduino ota configs
  /*ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());*/
}

void setup_wifi() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  wifidelay = millis();
  if(millis() - wifidelay > 500){
      while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      setupdelay = millis();
      //wifidelay = millis();
      }
    }
      else  {
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      //delay(500);     
      }
  
}

boolean reconnection() {
    if (client.connect("ESP", mqttUser, mqttPassword, "esp/lastwill", 1, 1, "offline")) {
      client.publish("esp/lastwill", "online");
      Serial.println("connected");
      client.subscribe("esp/servo");
      client.subscribe("esp/mode");
      //client.subscribe("esp/moisture");
      client.subscribe("esp/values");
      client.publish("esp/mode", "Manual");
    }
    return client.connected();
}

void callback(char* topic, byte* payload, unsigned int length) {
 
  //Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  String top(topic);
  String message;
  
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
    Serial.println((char)payload[i]);
  }

  String servo = "esp/servo";
  String autostring = "esp/mode";
  String valuestring = "esp/values";

  if(top == autostring && message == "Auto"){
    automode = 1; // auto
    waterplant = 0;
    //display.clearDisplay();
  }
  
  else if (top == autostring && message == "Manual"){
    automode = 0; // manual
    waterplant = 0;
    //display.clearDisplay();
  }
   
  if(top == servo && message == "0" && automode == 0 ){
    waterplant = false;
    myservo.write(120);
    lastWatering = millis();
  }

  else if(top == servo && message == "1" && automode == 0){
    myservo.write(0);
    waterplant = true;
  }

  if (top == valuestring && message == "1" && automode == 0){
    printValues();
  }
}

void displayfirst(){
    display.setCursor(0,0);
    display.print("Temperature: " + String(temp) + " *C");
    display.setCursor(0,20);
    display.print("Pressure " + String(pres) + "hPA" );
    display.setCursor(0,40);
    display.print("Altitude = " + String(alt) + " m");
}

void displaysecond(){
    display.setCursor(0,0);
    display.print("Humidity =  " + String(hum) + "%");
    display.setCursor(0,20);
    display.print("LDR: " + String(ldr));
    display.setCursor(0,40);
    display.print("Soil Moisture: " + String(mois));
}

unsigned long last;

void loop() {
  /*if(!statuss){
    bmedelay = millis();
    if (millis()- bmedelay > 500){
      statuss = bme.begin();
    }
  }*/
  
  if(!client.connected()){
    Serial.println("not connected");
    if(millis() - lastReconnectAttempt > 5000){
      lastReconnectAttempt = millis();
    
      if(reconnection()){
        lastReconnectAttempt = 0;  
      }
    }
  }
  else{
    client.loop();   
  }

  showDisplay();

  flash.read();
  if(flash.wasPressed() && automode == 0){
    automode = 1;
    client.publish("esp/mode", "Auto");
  }
  
  else if(flash.wasPressed() && automode == 1){
    automode = 0;
    client.publish("esp/mode", "Manual");
  }

  if(automode == 1) {   //auto
    digitalWrite(D0, HIGH);
  }

  else if(automode == 0){
    digitalWrite(D0, LOW);
  }

  if(!waterplant){
    if(automode == 1) {   //auto
      if(updatetimer.repeat()){
        printValues();
      }
  
      if(mois < 600){
        myservo.write(0);
        waterplant = true;
        last = millis();
      }    
    }
  }
  
  //ArduinoOTA.handle();

  if(waterplant){
      if(automode == 1){
        if(millis() - last > interval){        
          myservo.write(120);
          lastWatering = millis();
          printValues();
          waterplant = false;
        }
    }
  }
}

void printValues() {
    temp = bme.readTemperature();
    char buftemp[10];
    dtostrf(temp, 6, 2, buftemp);
    client.publish("esp/temperature", buftemp);
    Serial.println(temp);
    
    pres = bme.readPressure() / 100.0F;
    char bufpres[10];
    dtostrf(pres, 6, 2, bufpres);
    client.publish("esp/pressure", bufpres);
    
    alt = bme.readAltitude(SEALEVELPRESSURE_HPA);
    char bufalt[10];
    dtostrf(alt, 6, 2, bufalt);
    client.publish("esp/altitude", bufalt);

    hum = bme.readHumidity();
    char bufhum[10];
    dtostrf(hum, 6, 2, bufhum);
    client.publish("esp/humidity", bufhum);

    digitalWrite(D2, LOW);
    ldr = analogRead(A0);
    char ldrbuf [4];
    itoa(ldr, ldrbuf, 10);
    client.publish("esp/ldr", ldrbuf);
    ldrtimer.start();

    while(ldrtimer.waiting()){
      digitalWrite(D2, HIGH);
    }
    
    if(ldrtimer.done()){
      mois = analogRead(A0);
      char buf [4];
      itoa(mois, buf, 10);
      client.publish("esp/moisture", buf);
    }
}

void showDisplay(){
  if(waterplant){
      display.clearDisplay();
      display.setCursor(0,0);
      display.print("watering plant");
      display.display();
  }

  else if(!waterplant && client.connected()){
    if (millis() - lastReset < 5000) {
      display.clearDisplay();
      displayfirst();
      display.display();
    }
    
    else if (millis() - lastReset > 5000 && millis() - lastReset < 10000) {
      display.clearDisplay();
      displaysecond();
      display.display();
    }
    
    else if (millis() - lastReset > 10000 && millis() - lastReset < 15000) {
      display.clearDisplay();
      display.setCursor(0,0);
      unsigned long temp = (millis() - lastWatering) / 1000;
      display.print(String(temp));
      display.setCursor(0,20);
      display.print("Seconds"); 
      display.display();
    }
    
    else if (millis() - lastReset > 15000) {
      lastReset = millis();
      display.clearDisplay();
    }
  }

  else if(!waterplant && !client.connected()){
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Trying to bitconnect!");
    display.display();
  }
}

