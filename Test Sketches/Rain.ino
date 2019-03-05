/**
 * RAIN SENSOR TESTING
 */
 
int rainPin = 12;
int rainDumps = 0;
volatile long lastRainInt =0;

void setup() {
  Serial.begin(9600);
  pinMode(rainPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(rainPin), rainInt, FALLING);
  delay(1000);
  Serial.println("Finished Setup");
  interrupts();
}

void loop() {
  Serial.print("Rain Dumps: "); Serial.println(rainDumps);
  float rainFall = 0.2794 * rainDumps; //Convert to mm of rainfall
  //float rainFall = 0.011 * rainDumps; //Convert to inches of rainfall
  Serial.print("Rainfall: "); Serial.println(rainFall);
}

void rainInt() {
  //Interrupt routine to count rain gauge bucket tips
  if (millis() - lastRainInt > 10) {
    //Debounce the switch
    lastRainInt = millis(); //Current time
    rainDumps++;
  }
}
