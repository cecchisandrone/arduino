#include <Arduino.h>
#include <stdio.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <Pinger.h>
extern "C"
{
  #include <lwip/icmp.h> // needed for icmp packet definitions
}

#define XSTR(x) #x
#define STR(x) XSTR(x)
#define HTTP_REST_PORT 1337
#define WIFI_RETRY_DELAY 500
#define MAX_WIFI_INIT_RETRY 200
#define GPIO_PIN 0
#define LED_PIN 2
#define HEALTH_CHECK_INTERVAL 30000

const char* wifi_ssid = STR(WIFI_SSID);
const char* wifi_passwd = STR(WIFI_PASSWORD);
const char* deviceName = STR(DEVICE_NAME);

byte relayStatus = HIGH;
int duration = -1;
int lastMillis = 0, currentMillis = 0;
ESP8266WebServer http_rest_server(HTTP_REST_PORT);
Pinger pinger;
IPAddress gateway;

void blinkLed(int times, int millis) {
  for(int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, LOW);
    delay(millis);
    digitalWrite(LED_PIN, HIGH);
    delay(millis);
  }
}

int init_wifi() {
    delay(3000);
    int retries = 0;
    Serial.println("Connecting to WiFi AP:");
    Serial.println(wifi_ssid);
    Serial.println(wifi_passwd);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_passwd);
    // check the status of WiFi connection to be WL_CONNECTED
    while ((WiFi.status() != WL_CONNECTED) && (retries < MAX_WIFI_INIT_RETRY)) {
        retries++;
        delay(WIFI_RETRY_DELAY);
        Serial.print("#");
        blinkLed(1, 200);
    }
    Serial.println();
    return WiFi.status(); // return the WiFi connection status
}

void get_relay() {
    
    relayStatus = digitalRead(GPIO_PIN);
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& jsonObj = jsonBuffer.createObject();
    char JSONmessageBuffer[200];
    jsonObj["gpio"] = GPIO_PIN;
    // Logic is negated in ESP01S
    jsonObj["status"] = relayStatus ^ 1;
    jsonObj.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    http_rest_server.send(200, "application/json", JSONmessageBuffer);
}

void put_relay() {
    StaticJsonBuffer<500> jsonBuffer;
    String post_body = http_rest_server.arg("plain");
    Serial.println(post_body);

    JsonObject& jsonBody = jsonBuffer.parseObject(http_rest_server.arg("plain"));

    Serial.print("HTTP Method: ");
    Serial.println(http_rest_server.method());
    
    if (!jsonBody.success()) {
        Serial.println("error in parsing json body");
        http_rest_server.send(400);
    }
    else {   
        if (http_rest_server.method() == HTTP_PUT) {
            relayStatus = jsonBody["status"];
            duration = jsonBody["duration"];
            // Logic is negated in ESP01S
            relayStatus = relayStatus ^ 1;
            digitalWrite(GPIO_PIN, relayStatus);
            if(duration > 0) {
              delay(duration);
              relayStatus = relayStatus ^ 1;
              Serial.print("New value: " );
              Serial.println(relayStatus);
              digitalWrite(GPIO_PIN, relayStatus);
            }
            http_rest_server.send(200);
        }
    }
}

void config_rest_server_routing() {
    http_rest_server.on("/", HTTP_GET, []() {
        http_rest_server.send(200, "text/html",
            "Welcome to the ESP8266 REST Web Server");
    });
    http_rest_server.on("/relay", HTTP_GET, get_relay);
    http_rest_server.on("/relay", HTTP_PUT, put_relay);    
}

void setup(void) {
    
    pinMode(GPIO_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(GPIO_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);

    Serial.begin(115200);
    pinger.OnEnd([](const PingerResponse& response)
      {
        // Evaluate lost packet percentage
        float loss = 100;
        if(response.TotalReceivedResponses > 0)
        {
          loss = (response.TotalSentRequests - response.TotalReceivedResponses) * 100 / response.TotalSentRequests;
        }
        if(loss == 100) {
          Serial.println("Connection to gateway is lost...rebooting");
          ESP.restart();
        }    
        return true;
      });
    if (init_wifi() == WL_CONNECTED) {
        Serial.print("Connected to ");
        Serial.println(wifi_ssid);
        Serial.print("--- IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("--- Gateway: ");
        Serial.println(WiFi.gatewayIP());
        gateway = WiFi.gatewayIP();

        blinkLed(3, 500);

        if (MDNS.begin(deviceName, WiFi.localIP())) {
          Serial.print("MDNS responder started with device name: ");
          Serial.println(deviceName);
          MDNS.addService("relay", "tcp", HTTP_REST_PORT);
        }
        else {
          Serial.println("MDNS.begin failed");
        }
    }
    else {
        Serial.print("Error connecting to: ");
        Serial.println(wifi_ssid);
    }

    config_rest_server_routing();
    http_rest_server.begin();
    Serial.println("HTTP REST Server Started");
}

void loop(void) {    
    currentMillis = millis();
    if (currentMillis - lastMillis >= HEALTH_CHECK_INTERVAL)
    {      
      Serial.println("Executing healthcheck");        
      lastMillis = currentMillis;      
      if(pinger.Ping(gateway) == false) {
        Serial.println("Error during ping command");        
      }
      blinkLed(1, 300);
    }
    http_rest_server.handleClient();
    MDNS.update();
}
