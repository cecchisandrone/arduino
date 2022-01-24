
int LED_BUILTIN = 2;
int REED_SWITCH = 16;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(REED_SWITCH, INPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(500);  
  int reedStatus = digitalRead(REED_SWITCH);
  Serial.print("Reed status: ");
  Serial.println(reedStatus);
}
