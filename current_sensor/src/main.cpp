#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

#define XSTR(x) #x
#define STR(x) XSTR(x)
#define EEPROM_SIZE 2
#define THRESHOLD_ADDRESS 0

const char* ssid = STR(WIFI_SSID);
const char* password = STR(WIFI_PASSWORD);
float threshold = 0.0;
byte ledPin = 2;
byte adcPin = 34;
int mVperAmp = 66; // This the 5A version of the ACS712 -use 100 for 20A Module and 66 for 30A Module
int watt = 0;
double voltage = 0;
double vrms = 0;
double ampsRms = 0;
double error = 0.25; // error from the sensor
double calibrationFactor = 0.9; // This is own empirically established calibration factor as the voltage measured at D34 depends on the length of the OUT-to-D34 wire

void blink_led() {  
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
  delay(500);
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
}

void setup() {
    
  EEPROM.begin(EEPROM_SIZE);
  pinMode (ledPin, OUTPUT);
  pinMode (adcPin, INPUT);

  Serial.begin(115200);
  Serial.println("Configuration:");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);

  // Init EEPROM
  threshold = EEPROM.read(THRESHOLD_ADDRESS);
  Serial.printf("Threshold value is %d\n", threshold);

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
}

float getVPP()
{
  float result;
  int readValue;                // value read from the sensor
  int maxValue = 0;             // store max value here
  int minValue = 4096;          // store min value here ESP32 ADC resolution
  
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 1 Sec
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

void loop() {
  Serial.println (""); 
  voltage = getVPP();
  vrms = (voltage/2.0) * 0.707;   //root 2 is 0.707
  ampsRms = ((vrms * 1000)/mVperAmp)-error;
  Serial.print("Voltage: ");
  Serial.print(voltage);
  Serial.print(" - Amps RMS: ");
  Serial.print(ampsRms);
  watt = (ampsRms * 238 / calibrationFactor); // 240 is the main AC power voltage – this parameter changes locally
  Serial.print(" - Watts: ");
  Serial.print(watt);
  delay (100);
}