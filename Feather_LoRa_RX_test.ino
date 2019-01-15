//Feather M0 w/ LoRa
//Heavily based off Adafruit example for Testing

#include <SPI.h>
#include <RH_RF95.h>

//Radio Setup for Feather M0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

//Operation frequency- must match for network as well as unit capabilities
#define RF95_FREQ 915.0

//Radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

//blinky for receipt of message- useful to headless range testing
#define LED 13

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(115200);
  //Uncomment the following section to enable wait for serial
  while (!Serial) {
    delay(1);
  }
  
  delay(100);
  Serial.println("Feather LoRa RX Test");

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

void loop() {
  if (rf95.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if(rf95.recv(buf, &len)) {
      digitalWrite(LED, HIGH);
      RH_RF95::printBuffer("Received: ", buf, len);
      Serial.print("Got: ");
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);

      //Send reply
      uint8_t data[] = "And hello back to you";
      rf95.send(data, sizeof(data));
      rf95.waitPacketSent();
      Serial.println("Sent a reply");
      digitalWrite(LED, LOW);
    }
    else {
      Serial.println("Receive failed");
    }
  }
  else {
    digitalWrite(LED, LOW);
  }
}
