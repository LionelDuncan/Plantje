/* Code for Assignment 2 of Interaction Technology
 * Made by:
 * Takis Hoogterp - 5931967
 * Lionel - ???????
 */

#include <Wire.h>
#include <Servo.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED display
#define OLED_RESET D4
Adafruit_SSD1306 display(OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2
#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 

// BME280 sensor
#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C

// Time variables
unsigned long startupTime;
unsigned long lastTimeWater;

class Sensors {
  public:
    // returns TEMPERATURE in celsius degree
    int temperature() {
      return bme.readTemperature();
      }
    
    // returns PRESSURE in hPa
    int pressure() {
      return bme.readPressure() / 100.0F;
      }
    
    // returns HUMIDITY in bar
    int humidity() {
      return bme.readHumidity();
      }
};

class Display {
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

    void screenBME() {
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.clearDisplay();
      display.println(sensors.temperature() + " *C");
      display.setCursor(0, 5);
      display.println(sensors.pressure() + " hPa");
      display.setCursor(0, 10);
      display.println(sensors.humidity() + " %");
      display.display();
    }
    
    void loop() {
      screenBME();

    }

};

class Machine {
  private:
    Servo waterServo;
    Display Screen;
  public:
    void waterPlant() {
      int pos;
      Screen.initiateWater();
      

      // Command servo
      
    }
    void setup() {
      // Machine state. 1 means automatic state, waters plant based on sensors. 0 means manual state, waters plant when command is given.
      int machineState = 1;
      waterServo.attach(D2);
    }
};

Machine Machine;
Sensors Sensors;
Display Screen;

void setup() {
  // put your setup code here, to run once:
  startupTime = millis();
  pinMode(A0, INPUT);
  Wire.begin(D4, D6);
  bme.begin();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.display();
  display.clearDisplay();
  
  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(D7, LOW); 
  Screen.screenBME();
}



/**
 * returns BRIGHTNESs in lumen?
 */
// int brightness() {
//   return analogRead(lightPin);
// }
