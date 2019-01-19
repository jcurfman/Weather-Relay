/**Feather M0 w/ LoRa
 * Radio code based off Adafruit example
 * Designed as basis for network hub
 */

#include <SPI.h>
#include <RH_RF95.h>

//Radio setup for Feather M0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

//Operational frequency- must match for network as well as unit capabilities
#define RF95_FREQ 915.0

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
};

void setup() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  Serial.begin(115200);
  delay(100);
  Serial.println("Feather LoRa TX Test!");

  //reset radio
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

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
  delay(1000);
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
    sprintf(message, "%s,Acknowledged", blah);
    Serial.println(message);

    rf95.send((uint8_t *) message, sizeof(message));
    rf95.waitPacketSent();
    Serial.println("Sent a reply");
  }
}

