#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> //displaylibraries
#include <Button.h>        //https://github.com/JChristensen/Button -> thanks!

#include <ESP8266WiFi.h> //wifi
#include <PubSubClient.h> // mqtt
#include <Wire.h> // display+bme280
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h> //bme280
#include <Servo.h> //servo

#include <neotimer.h> //timer

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

const char* ssid = "";
const char* password =  "";

const char* mqttServer = "";
//const int mqttPort = ;
const char* mqttUser = "";
const char* mqttPassword = "";

//sensorvalues
float temp;
float pres;
float hum;
float alt;
int ldr;
int mois;

bool automode = 0; //manual
bool waterplant = false; // wordt de plant bijgevuld
unsigned long startwater = 0; // start water geven
const int waterinterval = 4000; // time water in automode
unsigned long lastWatering; // laatste keer dat water gegeven

unsigned long lastReset; // displayverandering

WiFiClient espClient; // wifi
PubSubClient client(espClient); // mqtt
Servo myservo;

Neotimer updatetimer = Neotimer(60000); // wanneer updaten in automodus
Neotimer ldrtimer = Neotimer(100); // op het juiste moment stroom naar select amuxboard

Button flash = Button(D3, true, true, 25); //flashbutton

//setupdelays
unsigned long wifidelay = 0;
unsigned long bmedelay = 0;
unsigned long clientdelay = 0;
unsigned long lastReconnectAttempt = 0;
unsigned long setupdelay = 0;

bool statuss; //bme

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Wire.begin(D7, D6); // nodig voor display en bme

  statuss = bme.begin();
  bmedelay = millis();

  if( millis() - bmedelay > 200){
    if(!statuss){
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      bmedelay = millis();
    }
  }
  
  pinMode(D2, OUTPUT); // select amuxboard
  pinMode(A0, INPUT); // aout amuxboard
  pinMode(D0, OUTPUT); // onboard led
  
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
      Serial.print(F("."));
      setupdelay = millis();
      }
    }
    
    else  {
    Serial.println(WiFi.localIP());    
    }
  
}

boolean reconnection() {
    if (client.connect("ESP", mqttUser, mqttPassword, "esp/lastwill", 1, 1, "offline")) {
      client.publish("esp/lastwill", "online", true);
      client.subscribe("esp/servo", 1);
      client.subscribe("esp/mode", 1);
      client.subscribe("esp/values", 1);
      client.publish("esp/mode", "Manual", true);
    }
    return client.connected();
}

void callback(char* topic, byte* payload, unsigned int length) {
 
  //Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  String top(topic);
  String message;
  
  Serial.print(F("Message:"));
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
    Serial.println((char)payload[i]);
  }

  String servo = F("esp/servo");
  String autostring = F("esp/mode");
  String valuestring = F("esp/values");

  // van manual naar auto
  if(top == autostring && message == "Auto"){
    automode = 1; // auto
    waterplant = 0;
  }

  //van auto naar manual
  else if (top == autostring && message == "Manual"){
    automode = 0; // manual
    waterplant = 0;
  }

  // manual modus, zet servo uit
  if(top == servo && message == "0" && automode == 0 ){
    waterplant = false;
    myservo.write(120);
    lastWatering = millis();
  }

  // manual modus, zet servo aan
  else if(top == servo && message == "1" && automode == 0){
    myservo.write(0);
    waterplant = true;
    startwater = millis();
  }

  // manual modus, haal direct waarden op
  if (top == valuestring && message == "1" && automode == 0){
    printValues();
  }
}

// scherm 1 waarden
void displayfirst(){
    display.setCursor(0,0);
    display.print("Temperature: " + String(temp) + " *C");
    display.setCursor(0,20);
    display.print("Pressure " + String(pres) + "hPA" );
    display.setCursor(0,40);
    display.print("Altitude = " + String(alt) + " m");
}

// scherm 2 waarden
void displaysecond(){
    display.setCursor(0,0);
    display.print("Humidity =  " + String(hum) + "%");
    display.setCursor(0,20);
    display.print("LDR: " + String(ldr));
    display.setCursor(0,40);
    display.print("Soil Moisture: " + String(mois));
}

void loop() {
  if(!client.connected()){ // reconnect
    Serial.println("not connected");
    if(millis() - lastReconnectAttempt > 5000){
      lastReconnectAttempt = millis();
    
      if(reconnection()){ // methode die verbinding maakt met MQTT
        lastReconnectAttempt = 0;  
      }
    }
  }
  else{
    client.loop();   
  }

  showDisplay(); // methode voor displayen

  // flashbutton doe modus switcht
  flash.read();
  if(flash.wasPressed() && automode == 0){
    automode = 1;
    client.publish("esp/mode", "Auto", true);
  }
  
  else if(flash.wasPressed() && automode == 1){
    automode = 0;
    client.publish("esp/mode", "Manual", true);
  }

  // juiste led bij juiste modus
  if(automode == 1) {
    digitalWrite(D0, HIGH);
  }

  else if(automode == 0){
    digitalWrite(D0, LOW);
  }

  if(!waterplant){
    if(automode == 1) {   //auto
      if(updatetimer.repeat()){
        printValues(); // elke minuut update
      }
  
      if(mois < 500){ // lager dan threshold in automodus
        myservo.write(0);
        waterplant = true;
        startwater = millis();
        client.publish("esp/servo", "1", true);
      }    
    }
  }

  // OTA updates
  //ArduinoOTA.handle();

  if(waterplant){
      if(automode == 1){
        if(millis() - startwater > waterinterval){          // 4 seconde voorbij        
          myservo.write(120);
          lastWatering = millis();
          printValues();
          waterplant = false;
          client.publish("esp/servo", "0", true);
        }
    }
  }
}

void printValues() {
    //temperatuur meting + publish
    temp = bme.readTemperature(); 
    char buftemp[10]; //float naar *char
    dtostrf(temp, 6, 2, buftemp);
    client.publish("esp/temperature", buftemp);
    Serial.println(temp);

    //pressure meting + publish
    pres = bme.readPressure() / 100.0F;
    char bufpres[10];
    dtostrf(pres, 6, 2, bufpres);
    client.publish("esp/pressure", bufpres);

    //hoogte meting + publish
    alt = bme.readAltitude(SEALEVELPRESSURE_HPA);
    char bufalt[10];
    dtostrf(alt, 6, 2, bufalt);
    client.publish("esp/altitude", bufalt);

    //humidity meting + publish
    hum = bme.readHumidity();
    char bufhum[10];
    dtostrf(hum, 6, 2, bufhum);
    client.publish("esp/humidity", bufhum);

    //ldr meting + publish
    digitalWrite(D2, LOW);
    ldr = analogRead(A0);
    char ldrbuf [4];
    itoa(ldr, ldrbuf, 10);
    client.publish("esp/ldr", ldrbuf);
    ldrtimer.start();

    //opladen moisture
    while(ldrtimer.waiting()){
      digitalWrite(D2, HIGH);
    }

    //moisturemeting + publish
    if(ldrtimer.done()){
      mois = analogRead(A0);
      char buf [4];
      itoa(mois, buf, 10);
      client.publish("esp/moisture", buf, true);
    }
}

void showDisplay(){
  // wanneer servo aan is.
  if(waterplant){
      display.clearDisplay();
      display.setCursor(0,0);
      display.print("watering plant");
      display.display();
  }

  //wanneer servo uit en verbinding met broker
  else if(!waterplant && client.connected()){
    //scherm 1
    if (millis() - lastReset < 5000) {
      display.clearDisplay();
      displayfirst();
      display.display();
    }

    //scherm 2
    else if (millis() - lastReset > 5000 && millis() - lastReset < 10000) {
      display.clearDisplay();
      displaysecond();
      display.display();
    }

    // scherm 3: laatste keer water
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

  //wanneer servo uit en geen verbinding
  else if(!waterplant && !client.connected()){
    display.clearDisplay();
    display.setCursor(0,0);
    display.print(F("Trying to bitconnect!"));
    display.display();
  }
}
