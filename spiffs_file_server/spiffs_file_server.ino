#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "FS.h"

const char* ssid = "Tp-Link";
const char* password = "16071607";

ESP8266WebServer server(80);

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

void handleFileWrite() {
  File fileToWrite = SPIFFS.open("/log.txt", "a");
 
  if(!fileToWrite){
      Serial.println("There was an error opening the file for writing");
      return;
  }
   
  if(fileToWrite.println(server.arg(0))) {
      Serial.println("File was written");
  } else {
      Serial.println("File write failed");
  }   
  fileToWrite.close();
}

void handleFileRead() {
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS failed to mount !\r\n");                    
  }
  else {
    String str = "";
    File f = SPIFFS.open("/log.txt", "r");
    if (!f) {
      Serial.println("Can't open SPIFFS file !\r\n");          
    }
    else {
      char buf[1024];
      int siz = f.size();
      while(siz > 0) {
        size_t len = std::min((int)(sizeof(buf) - 1), siz);
        f.read((uint8_t *)buf, len);
        buf[len] = 0;
        str += buf;
        siz -= sizeof(buf) - 1;
      }
      f.close();
      server.send(200, "text/plain", str);
    }
  }
}

void setup(void){
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

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

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/file_read", handleFileRead);

  server.on("/file_write", handleFileWrite);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();
}
