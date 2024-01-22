#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

#define XSTR(x) #x
#define STR(x) XSTR(x)
#define EEPROM_SIZE 4
#define ENABLED_ADDRESS 1
#define DURATION_ADDRESS 0

const char* ssid = STR(WIFI_SSID);
const char* password = STR(WIFI_PASSWORD);
const char* voxloudApiKey = STR(VOXLOUD_API_KEY);
const char* phone_number = STR(PHONE_NUMBER);
const char* voxloudUrl = "https://developer.voxloud.com/api/v1/click-to-call/50e32e9b1fe0f44b384db360e29c2ced/connect";

byte ledPin = 2;
byte inputPin = 32;
int val = 0; 
int count = 0;
unsigned long currentTime = 0;
unsigned long lastSound = 0;
HTTPClient http;
WebServer server(80);
int duration;
int enabled;
int lastCount = 0;
StaticJsonDocument<250> jsonDocument;

void set_duration_handler() {
  if (server.hasArg("plain") == false) {
    server.send(400, "application/json", "{\"error\":\"Bad request\"}");
  }
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);
  
  // Persist duration
  Serial.printf("Updating duration to %d\n", duration);
  duration = jsonDocument["duration"];
  EEPROM.write(DURATION_ADDRESS, duration);
  EEPROM.commit();
  server.send(200, "application/json", "{\"result\":\"Ok\"}");
}

void set_enabled_handler() {
  if (server.hasArg("plain") == false) {
    server.send(400, "application/json", "{\"error\":\"Bad request\"}");
  }
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);
  
  // Persist enabled  
  int enabledValue = jsonDocument["enabled"];
  if(enabledValue == 0 || enabledValue == 1) {
    enabled = enabledValue;
    Serial.printf("Updating enabled to %d\n", enabled);
    EEPROM.write(ENABLED_ADDRESS, enabled);
    EEPROM.commit();
    server.send(200, "application/json", "{\"result\":\"Ok\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Bad request\"}");    
  }
}

void get_count_handler() {
  
  char data[20];
  sprintf(data, "{\"count\":%d}", lastCount);
  server.send(200, "application/json", data);
}

void get_duration_handler() {
  
  char data[20];
  sprintf(data, "{\"duration\":%d}", duration);
  server.send(200, "application/json", data);
}

void get_enabled_handler() {
  
  char data[20];
  sprintf(data, "{\"enabled\":%d}", enabled);
  server.send(200, "application/json", data);
}

void setup_server() {
  server.on("/duration", HTTP_PUT, set_duration_handler);
  server.on("/duration", HTTP_GET, get_duration_handler);
  server.on("/enabled", HTTP_PUT, set_enabled_handler);
  server.on("/enabled", HTTP_GET, get_enabled_handler);
  server.on("/count", HTTP_GET, get_count_handler);
  server.begin();
  Serial.println("HTTP server started");
}

void blink_led() {  
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
  delay(500);
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
}

void call_number() {
  if(enabled == 1) {
    Serial.printf("Calling %s\n", phone_number);
    char authHeader[200];
    char payload[300];  
    http.begin(voxloudUrl);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Voverc-Auth", voxloudApiKey);
    sprintf(payload, "{\"contact\":\"%s\"}", phone_number);
    int httpCode = http.POST(payload); 
    if (httpCode > 0) { //Check for the returning code 
      String payload = http.getString();
      Serial.println(httpCode);
      Serial.println(payload);
    } else {
      Serial.println("Error on HTTP request");
    } 
    http.end(); //Free the resources
  } else {
    Serial.println("Not enabled, skipping");
  }
}

void setup() {
    
  EEPROM.begin(EEPROM_SIZE);
  pinMode (ledPin, OUTPUT);
  pinMode (inputPin, INPUT);
  Serial.begin(115200);
  Serial.println("Configuration:");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.printf("Voxloud URL: %s\n", voxloudUrl);

  // Init EEPROM
  duration = EEPROM.read(DURATION_ADDRESS);
  enabled = EEPROM.read(ENABLED_ADDRESS);
  Serial.printf("Duration value is %d\n", duration);
  Serial.printf("Enabled value is %d\n", enabled);

  // WIFI initialization
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);  

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // LED sequence to signal correct WIFI connection
  blink_led();
  setup_server();
}

void loop() {
  server.handleClient();
  val = digitalRead(inputPin);
  currentTime = millis();
  if (val == LOW)
  {
    digitalWrite (ledPin, HIGH);
    count ++;
    lastSound = currentTime;
    Serial.print("Noise ");
    Serial.println(count);
    delay(100);
  } else {
    digitalWrite (ledPin, LOW);
    if(currentTime - lastSound > 500) {
      if(count >= duration) {
        Serial.printf("Noise duration exceeded configured duration of %d\n", duration);
        call_number();
      }
      if(count != 0) {
        lastCount = count;
      }
      count = 0;
    }
  }
}