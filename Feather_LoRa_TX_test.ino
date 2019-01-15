//Feather M0 w/ LoRa
//Heavily based off Adafruit example for testing

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

void setup() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(115200);
  //uncomment the following section to enable wait for serial
  //while (!Serial) {
    //delay(1);
  //}

  delay(100);
  Serial.println("Feather LoRa TX Test!");

  //reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

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

  //transmitter power? Tinker.
  rf95.setTxPower(23, false);
}

int16_t packetnum = 0; //packet counter

void loop() {
  delay(1000);
  Serial.println("Transmitting...");

  char radiopacket[20] = "Hello World #      ";
  itoa(packetnum++, radiopacket+13, 10);
  Serial.print("Sending "); Serial.println(radiopacket);
  radiopacket[19] = 0;

  Serial.println("Sending...");
  delay(10);
  rf95.send((uint8_t *)radiopacket, 20);

  Serial.println("Waiting for packet to complete...");
  delay(10);
  rf95.waitPacketSent();
  //Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  Serial.println("Waiting for reply...");
  if (rf95.waitAvailableTimeout(1000)) {
    //Should be reply message?
    if (rf95.recv(buf, &len)) {
      Serial.print("Got reply: ");
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
    }
    else {
      Serial.println("Receive failed");
    }
  }
  else {
    Serial.println("No reply, is there a reciever in range?");
  }
}
