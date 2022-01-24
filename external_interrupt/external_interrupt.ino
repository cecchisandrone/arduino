/**
 * This sketch uses external interrupts to increment a counter and blink a led on every event. Interrupt can be emitted with a switch in this case
 * 
 */

/* LED pin */
byte ledPin = 2;
/* pin that is attached to interrupt */
byte interruptPin = 34;
/* hold the state of LED when toggling */
volatile byte state = LOW;
volatile int interruptCounter = 0;
int numberOfInterrupts = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

/* interrupt function toggle the LED */
void IRAM_ATTR handleInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&mux);
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  /* set the interrupt pin as input pullup*/
  pinMode(interruptPin, INPUT_PULLUP);
  /* attach interrupt to the pin
  function blink will be invoked when interrupt occurs
  interrupt occurs whenever the pin change value */
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, FALLING);
}

void loop() {
  
  if(interruptCounter>0) {
    portENTER_CRITICAL(&mux);
    interruptCounter--;
    portEXIT_CRITICAL(&mux);

    numberOfInterrupts++;
    Serial.print("An interrupt has occurred. Total: ");
    Serial.println(numberOfInterrupts);
    state = !state;
    digitalWrite(ledPin, state);    
    delay(100);
    state = !state;
    digitalWrite(ledPin, state);    
  }  
}
