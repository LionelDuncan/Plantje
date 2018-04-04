#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

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

#define OLED_RESET D8
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

unsigned long delayTime;

const char* ssid = "test";
const char* password =  "testtest123";

const char* mqttServer = "m23.cloudmqtt.com";
const int mqttPort = 16647;
const char* mqttUser = "esp";
const char* mqttPassword = "lionel123";

bool automode = 0; //manual
//Sensors Sensors;

WiFiClient espClient;
PubSubClient client(espClient);
Servo myservo;

Neotimer servotimer = Neotimer(15);
Neotimer updatetimer = Neotimer(5000);
Neotimer wifitimer = Neotimer(500);
Neotimer mqtttimer = Neotimer(2000);
Neotimer displaytimer = Neotimer(100);
Neotimer ldrtimer = Neotimer(100);
Neotimer debounce = Neotimer(100);

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

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  pinMode(D2, OUTPUT);
  pinMode(A0, INPUT);
  pinMode(D3, INPUT);
  pinMode(D0, OUTPUT);

  Wire.begin(D1, D5);
  
  // oled display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(2000);

  // Clear the buffer.
  display.clearDisplay();

  // draw a single pixel
  display.drawPixel(10, 10, WHITE);
  // Show the display buffer on the hardware.
  // NOTE: You _must_ call display after making any drawing commands
  // to make them visible on the display hardware!
  display.display();
  delay(2000);
  display.clearDisplay();

  // draw many lines
  testdrawline();
  display.display();
  delay(2000);
  display.clearDisplay();
  
  Serial.println("Connected to the WiFi network");

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {
 
      Serial.println("connected");  
 
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  Wire.begin(D7, D6);

  bool status;
  status = bme.begin();
    
   if (!status) {
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      while (1);
  }

  myservo.attach(D2);
    
  client.publish("esp/test", "Hello from ESP8266");
  client.subscribe("esp/servo");
  client.subscribe("esp/mode");
  client.subscribe("esp/moisture");

  delayTime = 1000;

  //arduino ota configs
  ArduinoOTA.onStart([]() {
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
  Serial.println(WiFi.localIP());
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
  String valuestring = "esp/value";
  char charBuf[50];

  if(top == autostring && message == "Auto"){
    automode = 1; // auto
  }
  
  else if (top == autostring && message == "Manual"){
    //automode = 0; // manual
  }
   
  if(top == servo && message == "1" && automode == 0){
    myservo.write(180);
    return;
    //if(servotimer.repeat()){
    //}
  }

  else if(top == servo && message == "0" && automode == 0){
    myservo.write(0);
    return;
    //if(servotimer.repeat()){
    //}
  }

  if (top == valuestring && message == "1" && automode == 0){
    printValues();
  }
 
  //Serial.println();
  //Serial.println("-----------------------");
}

void autoupdate(){
  if(automode == 1){
    
  }
}

void loop() {
  //display.clearDisplay();
  //delay(1000);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Hello, world!");
  display.display();
  delay(2000);
  display.clearDisplay();
  
  ArduinoOTA.handle();
  
  if(automode == 1){
    digitalWrite(D0, HIGH);
  }
  
  else if(automode == 0){
    digitalWrite(D0, LOW);
  }
  
  int flash = digitalRead(D3);
  //int led = digitalRead(D0);
  Serial.println(flash);
  
  if(flash == HIGH){
    //debounce.start();
    Serial.println("iets");
    //while(debounce.waiting()){
    //  Serial.println("waiting");
    //}
    //i/f(debounce.done()){
    // Serial.println("done");*/
      automode = 1;
      //debounce.reset();
    //} 
  }

  /*else if(automode == 1 && flash == LOW){
    //debounce.start();

    //if(debounce.done()){
      automode == 0;
      //debounce.reset();
    //} 
  }*/
  
  // put your main code here, to run repeatedly:
  client.loop();

  if(updatetimer.repeat()){
      printValues();
  }
}

void printValues() {
    float temp = bme.readTemperature();
    char buftemp[10];
    dtostrf(temp, 6, 2, buftemp);
    client.publish("esp/temperature", buftemp);
    //Serial.println(temp);
    
    float pres = bme.readPressure() / 100.0F;
    char bufpres[10];
    dtostrf(pres, 6, 2, bufpres);
    client.publish("esp/pressure", bufpres);
    //Serial.println(pres);

    float alt = bme.readAltitude(SEALEVELPRESSURE_HPA);
    char bufalt[10];
    dtostrf(alt, 6, 2, bufalt);
    client.publish("esp/altitude", bufalt);
    //Serial.println(alt);

    float hum = bme.readHumidity();
    char bufhum[10];
    dtostrf(hum, 6, 2, bufhum);
    client.publish("esp/humidity", bufhum);
    //Serial.println(hum);

    digitalWrite(D2, LOW);
    int ldr = analogRead(A0);
    char ldrbuf [4];
    itoa(ldr, ldrbuf, 10);
    client.publish("esp/ldr", ldrbuf);
    ldrtimer.start();

    while(ldrtimer.waiting()){
      digitalWrite(D2, HIGH);
    }
    
    if(ldrtimer.done()){
      int reading = analogRead(A0);
      char buf [4];
      itoa(reading, buf, 10);
      client.publish("esp/moisture", buf);
    }

    //display.clear();
    //display.drawString(0, 0, "Temperature: " + String(temp));
    //display.display();
}

void testdrawline() {  
  for (int16_t i=0; i<display.width(); i+=4) {
    display.drawLine(0, 0, i, display.height()-1, WHITE);
    display.display();
    delay(1);
  }
  for (int16_t i=0; i<display.height(); i+=4) {
    display.drawLine(0, 0, display.width()-1, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);
  
  display.clearDisplay();
  for (int16_t i=0; i<display.width(); i+=4) {
    display.drawLine(0, display.height()-1, i, 0, WHITE);
    display.display();
    delay(1);
  }
  for (int16_t i=display.height()-1; i>=0; i-=4) {
    display.drawLine(0, display.height()-1, display.width()-1, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);
  
  display.clearDisplay();
  for (int16_t i=display.width()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, i, 0, WHITE);
    display.display();
    delay(1);
  }
  for (int16_t i=display.height()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, 0, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();
  for (int16_t i=0; i<display.height(); i+=4) {
    display.drawLine(display.width()-1, 0, 0, i, WHITE);
    display.display();
    delay(1);
  }
  for (int16_t i=0; i<display.width(); i+=4) {
    display.drawLine(display.width()-1, 0, i, display.height()-1, WHITE); 
    display.display();
    delay(1);
  }
  delay(250);
}

