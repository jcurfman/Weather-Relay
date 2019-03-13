/**Feather M0 w/ LoRa
 *Partially based off of several code examples online
 */

#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <RH_RF95.h>
#include <Adafruit_AM2315.h>
#include <Adafruit_MPL115A2.h>

//Radio Setup for Feather M0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

//Operation frequency- must match for network as well as unit capabilities
#define RF95_FREQ 915.0

//Wind and Rain Sensor Setup
#define vanePin A0
#define anemPin 11
#define rainPin 12

// SD card library variables
Sd2Card card;
SdVolume volume;
SdFile root;
const int SD_CS = 10;

//RTC
RTC_PCF8523 rtc;

//Radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

//Adafruit AM2315 sensor- temp and humidity
Adafruit_AM2315 am2315;
/**Red lead to 3.0V
 * Black lead to Ground
 * White lead to i2c clock
 * Yellow lead to i2c data
 */

//Adafruit MPL115A2 sensor- temp and pressure
Adafruit_MPL115A2 mpl115a2;

//blinky for receipt of message- useful to headless range testing
#define LED 13

volatile uint32_t idNum;
char message[50];
String bleh;
String ident;
String oldTiming;
volatile byte windClicks = 0;
volatile long lastWindInt = 0;
long lastWindCheck = 0;
int rainDumps = 0;
volatile long lastRainInt = 0;
long lastIdSent;

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(115200);
  delay(100);

  /**while (!Serial) {
    //Comment this out before standalone operations deployment
    delay(1);
  } */
  
  Serial.println("Feather LoRa RX Test");

  //SD setup
  pinMode(SD_CS, OUTPUT);
  digitalWrite(RFM95_CS, HIGH); //Deselect LoRa radio for SD function.
  digitalWrite(SD_CS, HIGH);
  Serial.print("\nInitializing SD card...");
  if (! card.init(SPI_HALF_SPEED, SD_CS)) {
    Serial.println("initialization failed.");
  }
  else {
    Serial.println("Card present.");
    if (!volume.init(card)) {
      Serial.println("Could not find FAT32 partition. \nPlease properly format the card.");
    }
  }
  digitalWrite(SD_CS, LOW);
  digitalWrite(RFM95_CS, LOW); //Switch back to radio after verification of card.

  //reset radio
  //digitalWrite(RFM95_RST, LOW);
  delay(10);
  //digitalWrite(RFM95_RST, HIGH);
  delay(10);

  //Wind and Rain Sensor Setup
  pinMode(anemPin, INPUT_PULLUP);
  pinMode(rainPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(anemPin), windSpeedInt, FALLING);
  attachInterrupt(digitalPinToInterrupt(rainPin), rainInt, FALLING);

  while (!rf95.init()) {
    //One radio refuses to initialize with an SD card inserted currently. Might be circuit fluke? Further investigation required.
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK");

  //Setup RTC
  if (! rtc.begin()) {
    Serial.println("Could not find RTC");
    while (1);
  }
  if (! rtc.initialized()) {
    //If RTC not running, make statement and set the RTC to the date and time this
    //sketch was compiled
    Serial.println("The RTC was NOT running. Replace battery");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  } 
  DateTime now = rtc.now();
  Serial.print("RTC running. Current Time is ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.println(now.minute(), DEC); 

  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  if (!am2315.begin()) {
    Serial.println("am2315 sensor not found");
  }

  mpl115a2.begin();

  /**if (!mpl115a2.begin()) {
    //Bug- "Could not convert 'mpl115a2.Adafruit_MPL115A2::begin()' from void to bool"
    //Further research required
    Serial.println("mpl115a2 sensor not found");
  } */

  //Transmitter power
  rf95.setTxPower(23, false);

  //Get identity
  chipId(2);
  chipId(1);
  Serial.println(ident);

  interrupts(); //enable interrupts 
}

void testloop() {
  //Sensor Testing Loop
  DateTime now = rtc.now();
  Serial.println(Time());
  float pressureKPA = 0, temperatureC = 0;
  float Temp = am2315.readTemperature();
  Serial.print("Temp AM2315: "); Serial.println(Temp);
  delay(10);
  float Humid = am2315.readHumidity();
  mpl115a2.getPT(&pressureKPA,&temperatureC);
  Serial.print("Temp MPL115A2: "); Serial.println(temperatureC, 1);
  Serial.print("Presssure: "); Serial.println(pressureKPA, 4);
  delay(10);
  Humid = am2315.readHumidity();
  Serial.print("Humidity: "); Serial.println(Humid);
  delay(10);
  int windDirection = windVane();
  Serial.print("Wind Direction: "); Serial.println(windDirection);
  float wSpeed = windSpeed();
  Serial.print("Wind Speed: "); Serial.println(wSpeed);
  logging(String(Temp), String(temperatureC), String(pressureKPA), String(Humid), String(windDirection), String(wSpeed));
  delay(10);
  Serial.println("...");
  delay(1000);
}

void loop() {
  DateTime now = rtc.now();
  digitalWrite(LED, LOW);
  if (rf95.available()) {
    //If radio is operational
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (rf95.recv(buf, &len)) {
      //Message is available
      digitalWrite(LED, HIGH);
      //RH_RF95::printBuffer("Received: ", buf, len);
      Serial.print("Got: "); Serial.println((char*)buf);
      Serial.print("RSSI: "); Serial.println(rf95.lastRssi(), DEC);

      //Split into constituent parts
      bleh = (char*)buf;
      String recvID = getValue(bleh, ',', 0);
      Serial.println(recvID);
      String timing = getValue(bleh, ',', 1);
      Serial.println(timing);
      String datum = getValue(bleh, ',', 2);
      Serial.println(datum);
      
      //Is message for me?
      if (recvID == ident && timing != oldTiming) { 
        Serial.println("Is for me");

        //Decide what to do with message
        if (datum == "Acknowledged") {
          //Acknowledgement of ID- uncomment delay if range testing for better LED visibility
          Serial.println("Received acknowledgement, no reply");
          delay(500);
        }
        else if (datum == "Info") {
          //Dummy data requested. Generate and send.
          int hehehe = random(40,1200);
          Serial.println(hehehe);
          transmit(datum, hehehe);
        }
        else if (datum == "Humid") {
          float humid=am2315.readHumidity();
          humid *= 100;
          Serial.print("Humidity: "); Serial.println(humid);
          transmit(datum, humid);
        }
        else if (datum == "Temp") {
          float temp=am2315.readTemperature();
          temp *= 100;
          Serial.println(temp);
          transmit(datum, temp);
        }
        else if (datum == "Pres") {
          float presKPA = mpl115a2.getPressure();
          presKPA *= 1000;
          Serial.println(presKPA);
          transmit(datum, presKPA);
          //poll temp on this sensor for potential overheating here?
        }
        else if (datum == "WDir") {
          int WindDirection = windVane();
          Serial.println(WindDirection);
          transmit(datum, WindDirection);
        }
        else if (datum == "WSpeed") {
          float WindSpeed = windSpeed();
          WindSpeed *= 100;
          Serial.println(WindSpeed);
          transmit(datum, WindSpeed);
        }
        else if (datum == "Rain") {
          float rainFall = 0.2794 * rainDumps; //Convert to mm of rainfall
          //float rainFall = 0.011 * rainDumps; //Convert to inches of rainfall
          rainFall *= 1000;
          Serial.println(rainFall);
          transmit(datum, rainFall);
        }
        else if (datum == "Log") {
          logging("hi","bye","huh","blah","bleh","eh");
        }
        else {
          Serial.println("Unhandled Exception");
        }
        digitalWrite(LED, LOW);
      }
      else if (recvID == "Request ID") {
        if (millis()-lastIdSent > 60000) {
          //Initial ID Request
          int waitTime = random(100, 1000);
          delay(waitTime);
          Serial.println("Request for ID received");
          transmit(recvID, 42);
          digitalWrite(LED, LOW);
          lastIdSent = millis();
        }
      }
      else {
        //If message is new and for another station, rebroadcast once.
        if (timing != oldTiming) {
          //Wait between 0.1 and 1 seconds to see if another station rebroadcasts first. 
          int waitTime = random(100, 1000);
          delay(waitTime);
          rf95.send(buf, sizeof(buf));
          rf95.waitPacketSent();
          Serial.print("Forwarded message after delay of "); Serial.print(waitTime); Serial.println("ms");
          digitalWrite(LED, LOW);
          oldTiming = timing;
        }
        else {
          Serial.println("Already retransmitted. Do nothing.");
        }
      }
    }
  }
  /**if (now.second() % 10 == 0) {
    logging();
    delay(990);
    return;
  }*/
}

String Time() {
  DateTime now = rtc.now();
  String Times;
  Times += String(now.hour(), DEC); Times += ":";
  Times += String(now.minute(), DEC); Times += ":";
  Times += String(now.second(), DEC);
  return Times;
}

void transmit(String type, int datum) {
  DateTime now = rtc.now();
  //String to char is a PITA- done for sprintf function
  int str_len = type.length() + 1;
  char newType[str_len];
  type.toCharArray(newType, str_len);

  //Assembles and sends message
  sprintf(message, "%8x,%d,%s,%d", idNum, now.unixtime(), newType, datum);
  Serial.println(message);
  rf95.send((uint8_t *) message, sizeof(message));
  rf95.waitPacketSent();
  Serial.println("Sent reply.");
}

void chipId(int option) {
  //Assembles the SAMD processor chip ID value
  volatile uint32_t val1, val2, val3, val4;
  volatile uint32_t *ptr1 = (volatile uint32_t *)0x0080A00C;
  val1 = *ptr1;
  volatile uint32_t *ptr = (volatile uint32_t *)0x0080A040;
  val2 = *ptr;
  ptr++;
  val3 = *ptr;
  ptr++;
  val4 = *ptr;

  if (option == 1) {
    //Prints full processor ID if uncomment bits OR assigns end of value to String ident.
    //Serial.print("chip id: 0x");
    char buf[33];
    char bleh[12];
    //sprintf(buf, "%8x%8x%8x%8x", val1, val2, val3, val4);
    sprintf(bleh, "%8x", val4);
    //Serial.println(buf); 
    ident = bleh;
  }
  else if (option == 2) {
    //assigns end of value to variable idNum
    idNum = val4;
  }
}

String getValue(String data, char separator, int index) {
  //Takes in a string and divides it at each seperator value. 
  //Index denotes which divided section to take, from 0 to n.
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;
  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void logging(String str1, String str2, String str3, String str4, String str5, String str6) {
  //Assemble instance data- be sure to comment out whichever timekeeping you don't want.
  String dataLog = "";
  DateTime now = rtc.now();
  dataLog += String(ident); dataLog += ",";
  delay(5);
  //dataLog += String(now.unixtime()); dataLog += ",";
  dataLog += Time(); dataLog += ",";
  delay(5);
  dataLog += str1; dataLog += ",";
  delay(5);
  dataLog += str2; dataLog += ",";
  delay(5);
  dataLog += str3; dataLog += ",";
  delay(5);
  dataLog += str4; dataLog += ",";
  delay(5);
  dataLog += str5; dataLog += ",";
  delay(5);
  dataLog += str6;

  digitalWrite(RFM95_CS, HIGH); //Deselect LoRa radio for SD function.
  //digitalWrite(SD_CS, HIGH);

  if (!SD.begin(SD_CS)) {
    //Serial.println("SD initialization failed");
  }

  //Open or create a file
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println(dataLog);
    dataFile.close();
    Serial.println("Saved to SD");
  }
  else {
    Serial.println("Error opening datalog.txt");
  }

  digitalWrite(RFM95_CS, LOW); //Re-enable radio
  digitalWrite(SD_CS, LOW);
}

void rainInt() {
  //Activated by a reed switch, attached to pin 12
  if (millis() - lastRainInt > 10) {
    //Debounce the switch
    lastRainInt = millis(); //current time
    rainDumps++;
  }
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
  return(windSpeed);
}

int windVane() {
  //First obtain value of pin
  int rawVal = averageAnalogRead(vanePin);
  float voltage = rawVal * (3.3 / 4096.0); //12 bit ADC on Feather M0
  int Direction = 0;
  //Serial.print(voltage); Serial.print("; "); Serial.println(rawVal);
  /**
   * The following has little room for error, and may vary depending on ultimate setup.
   * Maps the wind direction based on 
   * Based on external resistor of 10k Ohms and a ref voltage of 3.3V.
   * Common error mode: Returning value in 1000's. Not sure why, but always indicator something is disconnected.
   */
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
