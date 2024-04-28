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
#define THRESHOLD_ADDRESS 0

const char* ssid = STR(WIFI_SSID);
const char* psk = STR(WIFI_PASSWORD);
const char* user = STR(SMARTHOME_USER);
const char* password = STR(SMARTHOME_PASSWORD);
const String smarthomeHost = STR(SMARTHOME_HOST);
const int smarthomePort = SMARTHOME_PORT;
const String smarthomeUrl = "http://" + smarthomeHost + ":" + smarthomePort + "/api/v1";
const String deviceName = STR(DEVICE_NAME);
int threshold;
bool notifyStatus = false;
const byte ledPin = 2;
const byte adcPin = 34;
const int mVperAmp = 66; // This the 5A version of the ACS712 -use 100 for 20A Module and 66 for 30A Module
int watt = 0;
double voltage = 0;
double vrms = 0;
double ampsRms = 0;
const double acVoltage = 238; // This is the main AC power voltage – this parameter changes locally
const double error = 0.25; // error from the sensor
const double calibrationFactor = 0.8; // This is own empirically established calibration factor as the voltage measured at D34 depends on the length of the OUT-to-D34 wire
const int sampleInterval = 1000; // sample interval in milliseconds
WebServer server(80);

void blink_led() {  
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
  delay(500);
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
}

void handlePutThreshold() {
  StaticJsonDocument<48> doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    server.send(400, "application/json", "{\"message\":\"Bad Request\"}");
    return;
  }
  JsonObject config = doc.as<JsonObject>();
  threshold = config["threshold"];
  EEPROM.put(THRESHOLD_ADDRESS, threshold);
  EEPROM.commit();
  Serial.printf("Threshold set to %d\n", threshold);
  server.send(200, "application/json", "{\"message\":\"OK\"}");
}

void handleGetThreshold() {
  StaticJsonDocument<16> doc;
  doc["threshold"] = threshold;
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

void setup_server() {
  server.on("/threshold", HTTP_PUT, handlePutThreshold);
  server.on("/threshold", HTTP_GET, handleGetThreshold);
  server.begin();
  Serial.println("HTTP server started");
}

// generate a function that invoke a remote api
void sendNotification(String message) {
  HTTPClient http;
  // Escape query parameters
  message.replace(" ", "%20");
  String authUrl = String(smarthomeUrl) + "/auth";
  String notificationUrl = String(smarthomeUrl) + "/configurations/1/notification/slack?message=" + message;
  http.begin(authUrl);
  http.addHeader("Content-Type", "application/json");
  String payload = "{\"username\":\"" + String(user) + "\",\"password\":\"" + String(password) + "\"}";
  int httpCode = http.POST(payload);
  if (httpCode == 200) {
    // deserialize the response as JSON and read the token attribute
    StaticJsonDocument<400> doc;
    DeserializationError error = deserializeJson(doc, http.getString());
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }
    String token = doc["token"];
    http.end();
    Serial.println("Sending notification");
    http.begin(notificationUrl);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + token);
    httpCode = http.POST("{}");
    if (httpCode == 200) {
      Serial.println("Notification sent successfully");
    } else {
      Serial.printf("Error on send message request. Error code: %d", httpCode);
      return;
    }
  } else {
    Serial.printf("Error on auth request. Error code: %d", httpCode);
  }
}

void setup() {

  // Init EEPROM  
  EEPROM.begin(EEPROM_SIZE);
  pinMode (ledPin, OUTPUT);
  pinMode (adcPin, INPUT);

  Serial.begin(115200);
  Serial.println("Configuration:");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(psk);
  threshold = EEPROM.read(THRESHOLD_ADDRESS);
  Serial.printf("Threshold: %d\n", threshold);
  Serial.printf("Smarthome URL: %s\n", smarthomeUrl.c_str());

  // WIFI initialization
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, psk);  

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

float getVPP()
{
  float result;
  int readValue;                // value read from the sensor
  int maxValue = 0;             // store max value here
  int minValue = 4096;          // store min value here ESP32 ADC resolution
  
  uint32_t start_time = millis();
  while((millis()-start_time) < sampleInterval) // sampling
  {
      readValue = analogRead(adcPin);
      // see if you have a new maxValue
      if (readValue > maxValue) 
      {
          /*record the maximum sensor value*/
          maxValue = readValue;
      }
      if (readValue < minValue) 
      {
          /*record the minimum sensor value*/
          minValue = readValue;
      }
   }
   
   // Subtract min from max
   result = ((maxValue - minValue) * 3.3)/4096.0; //ESP32 ADC resolution 4096
   return result;
}

void notify(int watt) {
  if (watt >= threshold && !notifyStatus) {
    notifyStatus = true;
    String message = "Power consumption is over the threshold " + String(threshold) + ". Current consumption: " + String(watt) + "W";
    sendNotification(message);
  } else if (watt < threshold && notifyStatus) {
    notifyStatus = false;
    String message = "Power consumption is under the threshold " + String(threshold) + ". Current consumption: " + String(watt) + "W";
    sendNotification(message);
  }
}

void loop() {
  server.handleClient();
  voltage = getVPP();
  vrms = (voltage/2.0) * 0.707;   //root 2 is 0.707
  ampsRms = ((vrms * 1000)/mVperAmp)-error;
  Serial.print("Voltage: ");
  Serial.print(voltage);
  Serial.print(" - Amps RMS: ");
  Serial.print(ampsRms);
  watt = (ampsRms * acVoltage / calibrationFactor);
  Serial.print(" - Watts: ");
  Serial.println(watt);
  notify(watt);
  delay(500);
}