void setup() {
  digitalWrite(33, LOW);
  pinMode(33, OUTPUT);  
  Serial.begin(9600);
  delay(1000); // give me time to bring up serial monitor
  Serial.println("ESP32 Touch Test");
}

void loop() {
  int value = touchRead(T7);
  Serial.println(value);  // get value using T7 (GPIO 27)
  if(value < 20) {
    digitalWrite(33, LOW);    
  } else {
    digitalWrite(33, HIGH);
  }
  delay(300);
}
