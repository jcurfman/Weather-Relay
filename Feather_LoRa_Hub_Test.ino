/**Feather M0 w/ LoRa
 * Radio code based off Adafruit example
 * Designed as basis for network hub
 */

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
String stationId;
char message[50];

//Design a class for each station's information- work in progress
class Station {
  //Class member variable
  String identifier; //The identifying string for each station
  //Datapoints
  int randData; //Dummy datapoint for now

  //Constructor
  public: Station(String id) {
    identifier = id;
  }
  
  void addData(float datum) {
    //Adds a dummy datapoint
    randData = datum; 
  }
  String ident() {
    return identifier;
  }
};

void setup() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  Serial.begin(115200);
  delay(100);
  
  while (!Serial) {
    delay(1);
  } 
  
  Serial.println("Feather LoRa TX Test!");

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
  if (! rtc.initialized()) {
    Serial.println("The RTC was NOT running. Replace battery");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  DateTime now = rtc.now();
  Serial.print("RTC running. Current Time is ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.println(now.minute(), DEC);

  //Setup Radio parameters
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK");
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  rf95.setTxPower(23, false);
}

int16_t packetnum = 0; //packet counter

void loop() {
  DateTime now = rtc.now();
  delay(100);
  Serial.println("Operating");
  if (haveStation == false) {
    //Transmit ID Request in order to find node
    char radiopacket[20] = "Request ID";
    Serial.print("Sending "); Serial.println(radiopacket);
    Serial.println("Sending...");
    delay(10);
    rf95.send((uint8_t *)radiopacket, 20);
    Serial.println("Waiting for packet to complete...");
    delay(10);
    rf95.waitPacketSent();
  
    //Wait for Reply
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    Serial.println("Waiting for reply...");
    if (rf95.waitAvailableTimeout(1000)) {
      //Should be reply message?
      if (rf95.recv(buf, &len)) {
        //Serial.print("Got reply: ");
        //Serial.println((char*)buf);
        Serial.print("RSSI: ");
        Serial.println(rf95.lastRssi(), DEC);

        stationId = (char*)buf;
        Serial.println(stationId);
        
        haveStation = true;
        Station alpha(stationId); //HAVING PROBLEMS WITH CLASS SETUP HERE VERSUS LATER
      }
      else {
        Serial.println("Receive failed");
      }
    }
  }
  else if (haveStation == true) {
    //Normal Operating mode- this is where the sensor read in will end up going.
    //String to char is a PITA.
    int str_len = stationId.length() + 1;
    char blah[str_len];
    stationId.toCharArray(blah, str_len);
    /**sprintf(message, "%s,Acknowledged", blah);
    Serial.println(message);

    rf95.send((uint8_t *) message, sizeof(message));
    rf95.waitPacketSent();
    Serial.println("Sent a reply"); */
    
    //Request dummy data
    sprintf(message, "%s,%d,Info", blah, now.unixtime());
    Serial.println(message);
    rf95.send((uint8_t *) message, sizeof(message));
    rf95.waitPacketSent();
    Serial.println("Sent a reply");

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
        Serial.println(recvID);
        String timing = getValue(returnMessage, ',', 1);
        Serial.println(timing);
        String info = getValue(returnMessage, ',', 2);
        Serial.println(info);
        String datum = getValue(returnMessage, ',', 3);
        Serial.println(datum);

        Station alpha(recvID); //HAVING PROBLEMS WITH CLASS SETUP HERE VERSUS EARLIER
        alpha.addData(datum.toInt());
      }
      else {
        Serial.println("Receive failed");
      }
      delay(5000);
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

