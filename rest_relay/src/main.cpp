#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#define XSTR(x) #x
#define STR(x) XSTR(x)

const char* ssid = STR(WIFI_SSID);
const char* password = STR(WIFI_PASSWORD);

WebServer server(80);
byte ledPin = 2;
byte relayPins[] = {13, 14, 27, 26};

void put_relay_handler() {
  StaticJsonDocument<350> doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    server.send(400, "application/json", "{\"message\":\"Bad Request\"}");
    return;
  }
  JsonArray pins = doc.as<JsonArray>();
  for (JsonVariant pin : pins) {
    int pin_number = pin["pin"];
    bool pin_status = pin["status"];
    Serial.print("Pin: ");
    Serial.print(pin_number);
    Serial.print(" Status: ");
    Serial.println(pin_status);
    digitalWrite(relayPins[pin_number], pin_status ? LOW : HIGH);
  }
  server.send(200, "application/json", "{\"message\":\"OK\"}");
}

void get_relay_handler() {
  StaticJsonDocument<350> doc;
  JsonArray array = doc.to<JsonArray>();
  for (int i = 0; i < 4; i++) {
    JsonObject pin = array.createNestedObject();
    pin["pin"] = i;
    pin["status"] = digitalRead(relayPins[i]) == LOW;
  }
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

void setup_server() {
  server.on("/relay", HTTP_PUT, put_relay_handler);
  server.on("/relay", HTTP_GET, get_relay_handler);
  server.begin();
  Serial.println("HTTP server started");
}

void blink_led() {  
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
  delay(100);
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
}

void setup()
{  
  Serial.begin(115200);  
  pinMode(ledPin, OUTPUT);

  for(int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH);
  }

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
}