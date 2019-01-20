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

String idNum;
volatile uint32_t blah;
char message[50];

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

  //Get identity
  chipId();
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

      //Generate message to send incorporating data- here, the CPU Identifier
      sprintf(message, "Hello, my ID is: %8x", blah);

      //Send reply
      rf95.send((uint8_t *) message, sizeof(message));
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
    Serial.println("No message recieved");
  }

  //The following block is for testing purposes only
  sprintf(message, "Hello, my ID is: %8x", blah);
  Serial.println(message);
}

/**
void sendReply(uint8_t data) {
  //Transmits "value" back over the network
  //uint8_t data[] = value;
  rf95.send(data, sizeof(data));
  rf95.waitPacketSent();
  Serial.println("Sent a reply");
  digitalWrite(LED, LOW);
}
*/

void chipId() {
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
  
  //Prints the chip ID if following block is uncommented.
  /** Serial.print("chip id: 0x");
  char buf[33];
  sprintf(buf, "%8x%8x%8x%8x", val1, val2, val3, val4);
  Serial.println(buf); */
  char buf2[12];
  sprintf(buf2, "%8x", val4);
  idNum = buf2;
  blah = val4;
}

