#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>

const char* ssid = "Tp-Link";
const char* password = "16071607";
byte ledPin = 2;
byte interruptPin = 32;
volatile byte state = LOW;
volatile int interruptCounter = 0;
int numberOfInterrupts = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

WebServer server(80);

void IRAM_ATTR handleInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&mux);
}

void setup() {
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
  
  pinMode(ledPin, OUTPUT);
  pinMode(interruptPin, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, RISING);

  digitalWrite(ledPin, HIGH);    
  delay(2000);
  digitalWrite(ledPin, LOW);  

  server.on("/", []() {
    server.send(200, "text/plain", String(numberOfInterrupts));
    numberOfInterrupts = 0;
  });

  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });
}

void loop() {
  
  server.handleClient();
  
  if(interruptCounter>0) {
    portENTER_CRITICAL(&mux);
    interruptCounter--;
    portEXIT_CRITICAL(&mux);

    numberOfInterrupts++;
    Serial.print("An interrupt has occurred. Total: ");
    Serial.println(numberOfInterrupts);
    state = !state;
    digitalWrite(ledPin, state);    
    delay(100);
    state = !state;
    digitalWrite(ledPin, state);    
  }
}