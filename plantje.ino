#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "eduroam";
const char* password =  "YourNetworkPassword";

const char* mqttServer = "m23.cloudmqtt.com";
const int mqttPort = 16647;
const char* mqttUser = "tyihhfet";
const char* mqttPassword = "iJyCeYCrqDzI";

WiFiClient espClient;
PubSubClient client(espClient);

// https://techtutorialsx.com/2017/04/09/esp8266-connecting-to-mqtt-broker/
// gitkraken test

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print("Connecting to WiFi..");
  }
 
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
 
  client.publish("esp/test", "Hello from ESP8266");
  client.subscribe("esp/test"); 
}

void callback(char* topic, byte* payload, unsigned int length) {
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
 
  Serial.println();
  Serial.println("-----------------------");
 
}

void loop() {
  // put your main code here, to run repeatedly:
  client.loop();
}
