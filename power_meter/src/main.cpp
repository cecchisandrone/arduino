#include "EmonLib.h"
#include <Arduino.h>
#include <driver/adc.h>
#include <WebServer.h>
EnergyMonitor emon1;

#define ADC_INPUT 36
#define HOME_VOLTAGE 240.0
#define XSTR(x) #x
#define STR(x) XSTR(x)

const char* ssid = STR(WIFI_SSID);
const char* password = STR(WIFI_PASSWORD);

WebServer server(80);
double power = 0;
double current = 0;
byte ledPin = 2;

void get_metrics_handler() {
  
  char data[60];
  sprintf(data, "{\"power\":%f,\"current\":%f}", power, current);
  server.send(200, "application/json", data);
}

void setup_server() {
  server.on("/metrics", HTTP_GET, get_metrics_handler);
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

void setup()
{  
  Serial.begin(115200);  
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
  analogReadResolution(ADC_BITS);
  pinMode(ADC_INPUT, INPUT);
  pinMode(ledPin, OUTPUT);
  emon1.current(ADC_INPUT, 60);

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

void loop()
{
  server.handleClient();
  current = emon1.calcIrms(1480);
  power = current * HOME_VOLTAGE;
}