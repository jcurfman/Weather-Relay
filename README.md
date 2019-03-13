# Weather-Relay
*Building a weather relay mesh network using LoRa and other low cost technologies*

### The Components
An Adafruit Feather M0 with onboard RFM95 LoRa radio forms the backbone of this project. Using omnidirectional antennas, ranges of over 2.5 kilometers have been attained, which is sufficient for this project. An RTC is required for each station node, as well as the hub- the mesh code relies upon it for operation. An Adalogger Featherwing is being used for each of the nodes, combining the RTC with a microSD reader/writer for local data backup.

Several weather sensors are being implemented for the receiver stations. The following list is not complete, but reflects the project at its current state of development.

Sensor | Dataset | Interface
--- | --- | ---
AM2315 | Humidity and Temperature | I2C
MPL115A2 | Barometric Pressure | I2C
Sparkfun Weather Meters | Wind Speed, Direction, and Rainfall | Analog/Digital Inputs
TSL2591 | Luminosity | I2C

Additional information on all of these may be found in this repository's [wiki section](https://github.com/jcurfman/Weather-Relay/wiki/Components).

### The Code
The RX and TX test files have both been deprecated, as have a number of other testing files for both sensors and the hub- while useful, the project has moved past them. They files are being kept for posterity. Additionally, a single station simplified version of the Hub code also resides here. This is intended for demonstration purposes only. 

The Hub file is designed to act as the master controller for the relay network. The Receive file is designed as the code for each station, or node.

Additional information about this project may be found at the [Hackster.io](https://www.hackster.io/129087/lora-based-weather-station-a332d9) page linked at the top of the repository, as well as in this repository's [wiki section](https://github.com/jcurfman/Weather-Relay/wiki). 
