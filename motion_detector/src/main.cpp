#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

#define XSTR(x) #x
#define STR(x) XSTR(x)

const char* ssid = STR(WIFI_SSID);
const char* password = STR(WIFI_PASSWORD);
const char* slackUrl = STR(SLACK_WEBHOOK_URL);
byte ledPin = 2;
byte inputPin = 32;
volatile byte state = LOW;
volatile int interruptCounter = 0;
int numberOfInterrupts = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
HTTPClient http;

void send_message() {
  Serial.print("Sending message to Slack channel");
  http.begin(slackUrl);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST("{\"username\":\"Motion detector\",\"text\":\"A new motion has been detected\"}"); 
  if (httpCode > 0) { //Check for the returning code 
    String payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);      
  } else {
    Serial.println("Error on HTTP request");      
  } 
  http.end(); //Free the resources
}

/* interrupt function */
void IRAM_ATTR handleInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&mux);
}

void setup() {
  
  Serial.begin(115200);
  Serial.println("Configuration:");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.print("Slack URL: ");
  Serial.println(slackUrl);

  // GPIO init
  pinMode(ledPin, OUTPUT);
  pinMode(inputPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(inputPin), handleInterrupt, FALLING);

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
  
  // LED sequence
  digitalWrite(ledPin, HIGH);    
  delay(500);
  digitalWrite(ledPin, LOW);  
  delay(500);
  digitalWrite(ledPin, HIGH);    
  delay(500);
  digitalWrite(ledPin, LOW);       
}

void loop() {  
  if(interruptCounter>0) {
    portENTER_CRITICAL(&mux);
    interruptCounter--;
    portEXIT_CRITICAL(&mux);
    
    send_message();
    
    // LED sequence
    state = !state;
    digitalWrite(ledPin, state);    
    delay(100);
    state = !state;
    digitalWrite(ledPin, state);    
  }  
}