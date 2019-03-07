/**Feather M0 w/ LoRa
 * Designed as basis for network hub
 * Test revision 3- object iteration?
 */

//Currently requires the number of stations nodes in use- hopefully temporary
const int numStations = 5;

//Design a class for each station's information- work in progress
class Station {
  //Class member variables
  String identifier; //The identifying string for each station
  int randData; //Dummy datapoint for now

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

  //Data return functions
  String ident() {
    return identifier;
  }
  float datum() {
    return randData;
  }
};

Station stations[numStations];

void setup() {
  Serial.begin(115200);
  delay(10);
  while (!Serial) {
    delay(1);
  }

  Serial.println("Feather LoRa Hub Test!");
}

void loop() {
  const char *identList[] = {"aa", "bb", "cc", "dd", "ee", "ff"};
  for (int i=0; i<numStations; i++) {
    Serial.print("Number: "); Serial.println(i+1);
    stations[i].addId(identList[i]);
    int bleh = random(0,100);
    stations[i].addData(bleh);
    Serial.print(i+1); Serial.println(" complete");
  }
  for (auto &item : stations) {
    String identity = item.ident();
    float datum = item.datum();
    Serial.println(identity);
    Serial.println(datum);
  }
}
