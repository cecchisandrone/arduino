#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include "DHT.h"
#include <HTTPClient.h>

#define DHTTYPE DHT11
#define HEALTH_CHECK_INTERVAL 3600000

/**
 * This constant is calculated with the following procedure:
 * - Area of pluviometer is 55 cm^2
 * - Volume of test water is 100 cm^2
 * Resulting height of water is 1.82 cm. Using this water quantity, generates 40 ticks of the pluviometer. So every tick is 0.455 mm of rain.
 */
const double millimetersPerTick = 0.455;
const char* ssid = "Tp-Link";
const char* password = "16071607";

WebServer server(80);
HTTPClient http;

/* LED pin */
byte ledPin = 2;
/* pin that is attached to interrupt */
byte interruptPin = 33;
// DHT Sensor
const int DHTPin = 32;
/* hold the state of LED when toggling */
volatile byte state = LOW;
volatile int interruptCounter = 0;
int numberOfInterrupts = 0;
long debouncing_time = 250; //Debouncing Time in Milliseconds
volatile unsigned long last_micros;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
int lastMillis = 0, currentMillis = 0;
int connectivityChecks = 0;

// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

void IRAM_ATTR handleInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&mux);
}

void debounceInterrupt() {
 if((long)(micros() - last_micros) >= debouncing_time * 1000) {
   handleInterrupt();
   last_micros = micros();
 }
}

void setup() {
  Serial.begin(115200);

  // initialize the DHT sensor
  dht.begin();

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
  /* set the interrupt pin as input pullup*/
  pinMode(interruptPin, INPUT_PULLUP);  
  /* attach interrupt to the pin
  function blink will be invoked when interrupt occurs
  interrupt occurs whenever the pin change value */

   // Notify wifi connection
  digitalWrite(ledPin, HIGH);    
  delay(2000);
  digitalWrite(ledPin, LOW);    
  
  attachInterrupt(digitalPinToInterrupt(interruptPin), debounceInterrupt, FALLING);

  server.on("/rainfall", []() {
    server.send(200, "text/plain", String(millimetersPerTick * numberOfInterrupts));
    numberOfInterrupts = 0;
  });

  server.on("/temperature", []() {
    float t = dht.readTemperature();
    // Check if any reads failed and exit early (to try again).
    if (isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      server.send(500, "text/plain", "Failed to read from DHT sensor!");
    } else {            
      Serial.print("Temperature: ");
      Serial.print(t);
      Serial.print(" *C ");      
      server.send(200, "text/plain", String(t));
    }
    
  });

  server.on("/humidity", []() {
    float h = dht.readHumidity();
    if (isnan(h)) {
      Serial.println("Failed to read from DHT sensor!");
      server.send(500, "text/plain", "Failed to read from DHT sensor!");
    } else {      
      Serial.print("Humidity: ");
      Serial.print(h);
      server.send(200, "text/plain", String(h));
    }    
  });

  server.on("/connectivity-checks", []() {
    server.send(200, "text/plain", String(connectivityChecks));        
  });

  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });

  server.begin();
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
  // Check connectivity
  currentMillis = millis();
  if (currentMillis - lastMillis >= HEALTH_CHECK_INTERVAL)
  {      
    lastMillis = currentMillis;      
    http.begin("http://ipinfo.io/ip");
    int httpCode = http.GET(); 
    if (httpCode > 0) { //Check for the returning code 
      String payload = http.getString();
      Serial.println(httpCode);
      Serial.println(payload);
      connectivityChecks++;
    } else {
      Serial.println("Error on HTTP request, restarting ESP32");
      ESP.restart();
    } 
    http.end(); //Free the resources
  }
}
