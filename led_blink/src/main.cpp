#include <Arduino.h>

/*
  ESP8266 LED Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
  The ESP32 has an internal blue LED at D2 (GPIO 02)
*/

int LED = 0;

void setup()
{ 
  digitalWrite(LED, HIGH);   // turn the LED off 
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  delay(10000);               // wait for a second
}

void loop()
{
  Serial.println("Loop");
  digitalWrite(LED, LOW);    // turn the LED on
  delay(1000);               // wait for a second
  digitalWrite(LED, HIGH);   // turn the LED off
  delay(1000);               // wait for a second
}
