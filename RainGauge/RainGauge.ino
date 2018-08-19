#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>

/**
 * This constant is calculated with the following procedure:
 * - Area of pluviometer is 55 cm^2
 * - Volume of test water is 100 cm^2
 * Resulting height of water is 1.82 cm. Using this water quantity, generates 40 ticks of the pluviometer. So every tick is 0.455 mm of rain.
 */
const double millimetersPerTick = 0.455;
//const char* ssid = "CECCHI'S WIFI";
const char* ssid = "Tp-Link-Ext";
const char* password = "16071607";

WebServer server(80);

/* LED pin */
byte ledPin = 2;
/* pin that is attached to interrupt */
byte interruptPin = 14;
/* hold the state of LED when toggling */
volatile byte state = LOW;
volatile int interruptCounter = 0;
int numberOfInterrupts = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR handleInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&mux);
}

void setup() {
  Serial.begin(115200);

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
  
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, RISING);

  server.on("/", []() {
    server.send(200, "text/plain", String(millimetersPerTick * numberOfInterrupts));
    numberOfInterrupts = 0;
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
}
