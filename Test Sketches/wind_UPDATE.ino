/**
 * WIND SENSOR TESTING
 * Currently, anemometer is reading slow,
 * And windvane is not reading properly/ reading intermittently based on housing motion
 */

#include <math.h>

int vanePin = A0;    // select the input pin for the windvane
int anemPin = 11; 
float WindSpeed; // speed in miles per hour 
int WindDirection; //in degrees
volatile byte windClicks = 0;
volatile long lastWindInt = 0;
long lastWindCheck = 0;

void setup() {
  Serial.begin(9600);
  pinMode(anemPin, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(anemPin), isr_rotation, RISING);
  attachInterrupt(digitalPinToInterrupt(anemPin), windSpeedInt, FALLING);
  delay(1000);
  Serial.println("Setup finished");
  interrupts();
}

void loop() {
  //interrupts();
  //delay (1000);
  //Serial.print("Rotations: "); Serial.println(Rotations);
  //noInterrupts();

  //WindSpeed = Rotations * 2.4;
  float WSpeed = windSpeed();
  Serial.print("Windspeed: "); Serial.println(WSpeed);
  //Serial.println(windClicks);
  WindDirection = windVane();
  Serial.print("Wind Direction: "); Serial.println(WindDirection);
  Serial.println("");
  delay(1000);
}

void windSpeedInt() {
  //Activated by a reed switch, attached to pin 11
  if (millis() - lastWindInt > 10) {
    //Debouncing the switch
    lastWindInt = millis(); //current time
    windClicks++; //1.492MPH for each click per second
  }
}

float windSpeed() {
  float deltaTime = millis() - lastWindCheck; 
  deltaTime /= 1000.0; //convert to seconds
  float windSpeed = (float)windClicks / deltaTime;
  windClicks = 0;
  lastWindCheck = millis();
  windSpeed *= 1.492;
  //windSpeed *= 10.0;
  return(windSpeed);
}

 int windVane() {
  //First obtain value of pin
  int rawVal = averageAnalogRead(vanePin);
  float voltage = rawVal * (3.3 / 4096.0); //12 bit ADC on Feather M0
  //Serial.print("Raw: "); Serial.println(rawVal);
  //Serial.print("Voltage: "); Serial.println(voltage);
  int Direction = 0;
  /**
   * The following has little room for error, and may vary depending on ultimate setup
   * Based on external resistor of 10k Ohms and a ref voltage of 3.3V.
   */
   /** Old values, before accounting for potential loss over cord length
   if (rawVal < 265) return (112);
   if (rawVal < 337) return (67);
   if (rawVal < 375) return (90);
   if (rawVal < 508) return (157);
   if (rawVal < 740) return (135);
   if (rawVal < 980) return (202);
   if (rawVal < 1151) return (180);
   if (rawVal < 1626) return (22);
   if (rawVal < 1848) return (45);
   if (rawVal < 2400) return (247);
   if (rawVal < 2525) return (225);
   if (rawVal < 2815) return (337);
   if (rawVal < 3145) return (0);
   if (rawVal < 3315) return (292);
   if (rawVal < 3551) return (315);
   if (rawVal < 3782) return (270); */
   
   if (rawVal < 70) return (247);
   if (rawVal < 90) return (292);
   if (rawVal < 100) return (270);
   if (rawVal < 130) return (202);
   if (rawVal < 190) return (225);
   if (rawVal < 250) return (157);
   if (rawVal < 295) return (180);
   if (rawVal < 410) return (337);
   if (rawVal < 470) return (315);
   if (rawVal < 615) return (112);
   if (rawVal < 640) return (135);
   if (rawVal < 710) return (22);
   if (rawVal < 790) return (0);
   if (rawVal < 830) return (67);
   if (rawVal < 890) return (45);
   if (rawVal < 1000) return (90);
 }

 int averageAnalogRead(int pinToRead) {
    byte numberOfReadings = 8;
    unsigned int runningValue = 0;

    for(int x = 0 ; x < numberOfReadings ; x++)
        runningValue += analogRead(pinToRead);
        delay(1);
    runningValue /= numberOfReadings;
    return(runningValue);  
}

