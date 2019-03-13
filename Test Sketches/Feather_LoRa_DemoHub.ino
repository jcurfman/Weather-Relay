/**Feather M0 w/ LoRa
 * Designed as basis for network hub
 * DEMONSTRATION LOOP
 * currently designed for a single station- to be changed going forward
 */

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
};

 #include <SPI.h>
 #include <Wire.h>
 #include "RTClib.h"
 #include <RH_RF95.h>

 //LoRa Radio for Feather M0
 #define RFM95_CS 8
 #define RFM95_RST 4
 #define RFM95_INT 3
 #define RF95_FREQ 915.0
 RH_RF95 rf95(RFM95_CS, RFM95_INT);

 //Real Time Clock
 RTC_PCF8523 rtc;

 const int numStations = 1;
 char* sensors[] = {"Temp","Humid","Pres","WDir","WSpeed","Rain"};

 bool haveStation = false;
 char message[50];
 int16_t packetnum = 0;
 String ident;

void setup() {
  pinMode(RFM95_RST, OUTPUT);
  Serial.begin(115200);
  delay(10);
  while (!Serial) {
    delay(1);
  }
  Serial.println("Feather LoRa Hub Demo!");
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
  Serial.println("Radio reset");

  //Setup RTC
  while (! rtc.begin()) {
    Serial.println("Could not find RTC");
    while(1);
  }
  if (! rtc.initialized()) {
    Serial.println("The RTC was NOT running. Replace battery.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  DateTime now = rtc.now();
  Serial.print("RTC running. Current time is ");
  Serial.print(now.hour(), DEC); Serial.print(":");
  Serial.println(now.minute(), DEC);

  //Setup LoRa radio
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed!");
    Serial.println("Please fix radio error before running program.");
    while (1);
  }
  Serial.println("LoRa radio init OK");
  if (! rf95.setFrequency(RF95_FREQ)) {
    Serial.println("Set Frequency Failed");
  }
  Serial.print("Set Frequency to: "); Serial.println(RF95_FREQ);
  rf95.setTxPower(23, false);
  Serial.println("Setup complete.");
}

Station sta;

void loop() {
  DateTime now = rtc.now();
  Serial.println("");
  
  if (haveStation == false) {
    //Setup mode- need to acquire node ID's
    Serial.println("Need to acquire station ID's");
    char radiopacket[20] = "Request ID";
    Serial.print("Sending: "); Serial.println(radiopacket);
    rf95.send((uint8_t *)radiopacket, 20);
    Serial.println("Waiting for packet to complete...");
    delay(10);
    rf95.waitPacketSent();

    //Wait for reply
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    //Serial.println("Waiting for reply...");
    if (rf95.waitAvailableTimeout(1000)) {
      if (rf95.recv(buf, &len)) {
        Serial.print("Got reply: ");
        Serial.println((char*)buf);
        Serial.print("RSSI (signal strength): "); Serial.println(rf95.lastRssi(), DEC);

        String recv = (char*)buf;
        String stationId = getValue(recv, ',', 0);
        sta.addId(stationId);
        ident = stationId;
        //Serial.println(ident);
        haveStation = true;
        delay(100);
      }
      else {
        Serial.println("Receive failed");
      }
    }
  }
  else if (haveStation == true) {
    //Normal operation mode- Start sensor read in
    String id = ident;
    int str_len = id.length() +1;
    char StationId[str_len];
    id.toCharArray(StationId, str_len);

    for (auto &item : sensors) {
      //Serial.println(item);
      transmit(item);

      //Listen for return message with data
      uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
      uint8_t len = sizeof(buf);
      //Serial.println("Waiting for reply...");
      if (rf95.waitAvailableTimeout(1000)) {
        //Should be reply message
        if (rf95.recv(buf, &len)) {
          //Serial.print("RSSI (signal strength): "); Serial.println(rf95.lastRssi(), DEC);

          String returnMessage = (char*)buf;
          String recvID = getValue(returnMessage, ',', 0);
          String timing = getValue(returnMessage, ',', 1);
          String type = getValue(returnMessage, ',', 2);
          String datum = getValue(returnMessage, ',', 3);

          if (type == "Humid") {
            float hum = datum.toFloat()/100;
            sta.addHum(hum);
            Serial.print("Humidity (%): "); Serial.println(hum);
          }
          else if (type == "Temp") {
            float temp = datum.toFloat()/100;
            sta.addTempE(temp);
            Serial.print("Temperature (C): "); Serial.println(temp);
          }
          else if (type == "Pres") {
            float pres = datum.toFloat()/1000;
            sta.addPres(pres);
            Serial.print("Pressure (KPA): "); Serial.println(pres);
          }
          else if (type == "WDir") {
            sta.addWDir(datum.toInt());
            Serial.print("Wind Direction (deg): "); Serial.println(datum);
          }
          else if (type == "WSpeed") {
            float wSpeed = datum.toFloat()/100;
            sta.addWSpeed(wSpeed);
            Serial.print("Wind Speed: "); Serial.println(wSpeed);
          }
          else if (type == "Rain") {
            float rain = datum.toFloat()/1000;
            sta.addRain(rain);
            Serial.print("Rain (mm): "); Serial.println(rain);
          }
        }
      }
    }
  }
}

void transmit(String type) {
  DateTime now = rtc.now();
  //String to char is a PITA- done for sprintf function
  int str_len = type.length() + 1;
  char newType[str_len];
  type.toCharArray(newType, str_len);

  str_len = ident.length() +1;
  char newIdent[str_len];
  ident.toCharArray(newIdent, str_len);
  //Serial.println(newIdent);

  //Assembles and sends message
  sprintf(message, "%s,%d,%s", newIdent, now.unixtime(), newType);
  //Serial.print("Sending: "); Serial.println(message);
  rf95.send((uint8_t *) message, sizeof(message));
  rf95.waitPacketSent();
  //Serial.println("Transmitted");
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
