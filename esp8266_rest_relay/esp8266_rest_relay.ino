 #include <stdio.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <Pinger.h>
extern "C"
{
  #include <lwip/icmp.h> // needed for icmp packet definitions
}

#define HTTP_REST_PORT 1337
#define WIFI_RETRY_DELAY 500
#define MAX_WIFI_INIT_RETRY 200
#define GPIO_PIN 0
#define HEALTH_CHECK_INTERVAL 30000
#define DEVICE_NAME "cib-door"

const char* wifi_ssid = "FRITZ!Box WLAN";
const char* wifi_passwd = "16071607";
byte relayStatus = LOW;
int duration = -1;
int lastMillis = 0, currentMillis = 0;
MDNSResponder mdns;
ESP8266WebServer http_rest_server(HTTP_REST_PORT);
Pinger pinger;
IPAddress gateway;

int init_wifi() {
    int retries = 0;
    Serial.println("Connecting to WiFi AP..........");
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_passwd);
    // check the status of WiFi connection to be WL_CONNECTED
    while ((WiFi.status() != WL_CONNECTED) && (retries < MAX_WIFI_INIT_RETRY)) {
        retries++;
        delay(WIFI_RETRY_DELAY);
        Serial.print("#");
    }
    Serial.println();
    return WiFi.status(); // return the WiFi connection status
}

void get_relay() {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& jsonObj = jsonBuffer.createObject();
    char JSONmessageBuffer[200];  
    jsonObj["gpio"] = GPIO_PIN;
    jsonObj["status"] = relayStatus;
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
    pinMode(GPIO_PIN, OUTPUT);
    if (init_wifi() == WL_CONNECTED) {
        Serial.print("Connected to ");
        Serial.println(wifi_ssid);
        Serial.print("--- IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("--- Gateway: ");
        Serial.println(WiFi.gatewayIP());
        gateway = WiFi.gatewayIP();

        if (mdns.begin(DEVICE_NAME, WiFi.localIP())) {
          Serial.println("MDNS responder started");
          mdns.addService("relay", "tcp", HTTP_REST_PORT);    
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
      lastMillis = currentMillis;      
      if(pinger.Ping(gateway) == false) {
        Serial.println("Error during ping command");        
      }
    }
    http_rest_server.handleClient();
}
