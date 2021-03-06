/**Feather M0 w/ LoRa
 * Designed as basis for network hub
 * Uses object iteration for tracking of station data
 */

//Currently requires the number of stations nodes in use- hopefully temporary
const int numStations = 1;

//Design a class for each station's information- work in progress as sensors are implemented.
class Station {
  //Class member variables
  String identifier; //The identifying string for each station
  int randData; //Dummy datapoint for now
  float humid; //Humidity
  float tempE; //External Temperature
  float presKPA; //Pressure
  int WindDirec; //Wind Direction, in degrees
  float WindSpeed; 
  int Lum; //Luminosity
  float rainFall; //rainfall, in mm (can be converted to inches on receiver code)

  //Constructor
  public: Station() {
  }

  //Data entry functions
  void addId(String id) {
    identifier = id;
  }
  void addData(float datum) {
    //Adds a dummy datapoint
    randData = datum; 
  }
  void addHum(float datum) {
    //Adds a humidity value
    humid = datum;
  }
  void addTempE(float datum) {
    //Adds an external temperature value
    tempE = datum;
  }
  void addPres(float datum) {
    //Adds a pressure value
    presKPA = datum;
  }
  void addWDir(int datum) {
    //Adds a wind direction value
    WindDirec = datum;
  }
  void addWSpeed(float datum) {
    //Adds a wind speed value
    WindSpeed = datum;
  }
  void addRain(float datum) {
    //Adds a rainfall value
    rainFall = datum;
  }
  void addLum(int datum) {
    //Adds luminosity value
    Lum = datum;
  }

  //Data return functions
  String ident() {
    return identifier;
  }
  float datum() {
    return randData;
  }
  float reHum() {
    return humid;
  }
  float reTempE() {
    return tempE;
  }
  float rePres() {
    return presKPA;
  }
  int reWDir() {
    return WindDirec;
  }
  float reWSpeed() {
    return WindSpeed;
  }
  float reRain() {
    return rainFall;
  }
  int reLum() {
    return Lum;
  }
};

#include <SPI.h>
#include <Wire.h>
#include "RTClib.h"
#include <RH_RF95.h>

//Radio setup for Feather M0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

//Operational frequency- must match for network as well as unit capabilities
#define RF95_FREQ 915.0

//Real time clock
RTC_PCF8523 rtc;

//Radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

bool haveStation = false;
char message[50];
char* sensors[] = {"Temp","Humid","Pres","WDir","WSpeed","Rain","Light"};
long lastLog = 0;

Station stations[numStations];

void setup() {
  pinMode(RFM95_RST, OUTPUT);
  Serial.begin(115200);
  delay(10);
  while (!Serial) {
    delay(1);
  }

  Serial.println("Feather LoRa Hub Test!");

  //reset radio
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  //Setup RTC
  if (! rtc.begin()) {
    Serial.println("Could not find RTC");
    while (1);
  }
  else {
    if (! rtc.initialized()) {
      Serial.println("The RTC was NOT running. Replace battery.");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    DateTime now = rtc.now();
    Serial.print("RTC running. Current Time is ");
    Serial.print(now.hour(), DEC);
    Serial.print(":");
    Serial.println(now.minute(), DEC);
  }

  //Setup Radio parameters
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed!");
    Serial.println("Please fix radio error before running program.");
    while (1);
  }
  Serial.println("LoRa radio init OK");
  if (! rf95.setFrequency(RF95_FREQ)) {
    Serial.println("Set Frequency failed");
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  rf95.setTxPower(23, false);
}

int16_t packetnum = 0; //packet counter

void loop() {
  DateTime now = rtc.now();
  Serial.println("...");
  if (haveStation == false) {
    //Setup mode- need to acquire node ID's.
    for (int i=0; i<numStations; i++) {
      char radiopacket[20] = "Request ID";
      Serial.print("Sending "); Serial.println(radiopacket);
      rf95.send((uint8_t *)radiopacket, 20);
      Serial.println("Waiting for packet to complete...");
      delay(10);
      rf95.waitPacketSent();

      //Wait for reply
      uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
      uint8_t len = sizeof(buf);
      Serial.println("Waiting for reply...");
      if (rf95.waitAvailableTimeout(1000)) {
        if (rf95.recv(buf, &len)) {
          Serial.print("Got reply: ");
          Serial.println((char*)buf);
          Serial.print("RSSI: "); Serial.println(rf95.lastRssi(), DEC);

          String recv = (char*)buf;
          String stationId = getValue(recv, ',', 0);
          String timing = getValue(recv, ',', 1);
          String info = getValue(recv, ',', 2);
          String datum = getValue(recv, ',', 3);

          stations[i].addId(stationId);
        }
        else {
          Serial.println("Recieve failed.");
        }
      }
    }
    haveStation = true;
  }
  else if (haveStation == true) {
    if (now.minute()%5 == 0 && millis()-lastLog > 60000) {
      Serial.println("Take 30s, Pause for logging");
      delay(30000);
      lastLog = millis();
    }
    //Normal operating mode- Start sensor read in
    for (auto &item : stations) {
      String id = item.ident();
      int str_len = id.length() + 1;
      char StationId[str_len];
      id.toCharArray(StationId, str_len);

      for (auto &info : sensors) {
        //iterates through Sensor array for all valid onboard sensors
        Serial.println(info);
        sprintf(message, "%s,%d,%s", StationId, now.unixtime(), info);
        Serial.println(message);
        rf95.send((uint8_t *)message, sizeof(message));
        rf95.waitPacketSent();
        Serial.println("Sent");

        //Listen for return message with data
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        Serial.println("Waiting for reply...");
        if (rf95.waitAvailableTimeout(1000)) {
          if (rf95.recv(buf, &len)) {
            Serial.print("RSSI: "); Serial.println(rf95.lastRssi(), DEC);

            //Parse message into components
            String returnMessage = (char*)buf;
            String recvID = getValue(returnMessage, ',', 0);
            String timing = getValue(returnMessage, ',', 1);
            String type = getValue(returnMessage, ',', 2);
            String datum = getValue(returnMessage, ',', 3);

            //Sort data into whatever category it should be in.
            if (type == "Humid") {
              float hum = datum.toFloat()/100.0;
              item.addHum(hum);
            }
            else if (type == "Temp") {
              float temp = datum.toFloat()/100.0;
              item.addTempE(temp);
            }
            else if (type == "Pres") {
              float pres = datum.toFloat()/1000.0;
              item.addPres(pres);
            }
            else if (type == "WDir") {
              item.addWDir(datum.toInt());
            }
            else if (type == "WSpeed") {
              float wSpeed = datum.toFloat()/100.0;
              item.addWSpeed(wSpeed);
            }
            else if (type == "Rain") {
              float rain = datum.toFloat()/1000.0;
              item.addRain(rain);
            }
            else if (type == "Light") {
              item.addLum(datum.toInt());
            }
          }
        }
      }

      //Request dummy data
      /**sprintf(message, "%s,%d,Info", StationId, now.unixtime());
      Serial.println(message);
      rf95.send((uint8_t *)message, sizeof(message));
      rf95.waitPacketSent();
      Serial.println("Sent");

      //Listen for return message with data
      uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
      uint8_t len = sizeof(buf);
      Serial.println("Waiting for reply...");
      if (rf95.waitAvailableTimeout(1000)) {
        //Should be reply message?
        if (rf95.recv(buf, &len)) {
          Serial.print("RSSI: ");
          Serial.println(rf95.lastRssi(), DEC);

          String returnMessage = (char*)buf;
          String recvID = getValue(returnMessage, ',', 0);
          String timing = getValue(returnMessage, ',', 1);
          String info = getValue(returnMessage, ',', 2);
          String datum = getValue(returnMessage, ',', 3);

          item.addData(datum.toInt());
        }
        else {
          Serial.println("Receive failed");
        }

        //Request station to log data
        sprintf(message, "%s,%d,Log", StationId, now.unixtime());
        delay(1000);
      } */
    }
  }
}

String getValue(String data, char separator, int index)
{
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
