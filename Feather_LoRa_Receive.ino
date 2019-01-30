//Feather M0 w/ LoRa
//Partially based off of several code examples online

#include <SPI.h>
#include <RH_RF95.h>
#include <Adafruit_AM2315.h>

//Radio Setup for Feather M0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

//Operation frequency- must match for network as well as unit capabilities
#define RF95_FREQ 915.0

//Radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

//Adafruit AM2315 sensor- temp and humidity
Adafruit_AM2315 am2315;
/**Red lead to 3.0V
 * Black lead to Ground
 * White lead to i2c clock
 * Yellow lead to i2c data
 */

//blinky for receipt of message- useful to headless range testing
#define LED 13

volatile uint32_t idNum;
char message[50];
String bleh;
String ident;

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(115200);
  delay(100);

  /**while (!Serial) {
    delay(1);
  } */
  
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

  if (!am2315.begin()) {
    Serial.println("am2315 sensor not found");
    while(1);
  }

  //Transmitter power
  rf95.setTxPower(23, false);

  //Get identity
  chipId(2);
  chipId(1);
  //Serial.println(ident);
}

void loop() {
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
      String datum = getValue(bleh, ',', 1);
      Serial.println(datum);
      
      //Is message for me?
      if (recvID == ident) { //Remove hardcoded ID when the above crash is solved.
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
          float randData = hehehe / 10.0;
          Serial.println(randData);
          sprintf(message, "%8x,Info,%d", idNum, hehehe);
          Serial.println(message);
          rf95.send((uint8_t *) message, sizeof(message));
          rf95.waitPacketSent();
          Serial.println("Sent data reply");
        }
        else if (datum == "Hum") {
          float temp=am2315.readHumidity();
          Serial.println(temp);
          sprintf(message, "%8x,Hum,%d", idNum, temp);
          Serial.println(message);
          rf95.send((uint8_t *) message, sizeof(message));
          rf95.waitPacketSent();
          Serial.println("Sent data reply");
        }
        else if (datum == "Temp") {
          float temp=am2315.readTemperature();
          Serial.println(temp);
          sprintf(message, "%8x,Temp,%d", idNum, temp);
          Serial.println(message);
          rf95.send((uint8_t *) message, sizeof(message));
          rf95.waitPacketSent();
          Serial.println("Sent data reply");
        }
        else {
          Serial.println("Unhandled Exception");
        }
        digitalWrite(LED, LOW);
      }
      else if (recvID == "Request ID") {
        //Initial ID Request
        Serial.println("Request for ID received");
        sprintf(message, "%8x", idNum);
        rf95.send((uint8_t *) message, sizeof(message));
        rf95.waitPacketSent();
        Serial.println("Sent a reply");
        digitalWrite(LED, LOW);
      }
      else {
        //If message is for another station, rebroadcast once. 
        int waitTime = random(100, 2000);
        delay(waitTime);
        rf95.send(buf, sizeof(buf));
        rf95.waitPacketSent();
        Serial.print("Forwarded message after delay of "); Serial.print(waitTime); Serial.println("ms");
        digitalWrite(LED, LOW);
      }
    }
  }
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
    //Prints full processor ID OR assigns end of value to String ident.
    //Serial.print("chip id: 0x");
    char buf[33];
    char bleh[12];
    //sprintf(buf, "%8x%8x%8x%8x", val1, val2, val3, val4);
    sprintf(bleh, "%8x", val4);
    //Serial.println(buf); 
    //Serial.println(bleh);
    ident = bleh;
  }
  else if (option == 2) {
    //assigns end of value to variable idNum
    idNum = val4;
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


