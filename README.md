# ESP8266Wemo
Make ESP8266 works directly with Amazone Echo by emulating a WeMo switch using nothing but standard ESP8266 libraries


### Getting Started

1. Fill in your SSID & Password at the begining of the sketch

2. Change the LED pin to suit your board

3. Power it on and wait for it to be connected to WiFi

4. Ask "Alexa, discover devices"

5. After the discovery is successful, ask "Alexa, turn on/off your <your-device-name>"


### [Advanced] Control multiple pins/devices

Here's the outline of the steps:

1. Modify responseToSearchUdp function, use a for-loop to give 1 response for each GPIO pin you want to control. Each pin must be assign a unique serialNumber, uuid & port. (IP Address is the same for all, it is always the IP Address of your ESP8266)

2. For each of the port/pin you have setup above, create a ESP8266WebServer for it.

3. Modify handleUpnpControl function to handle on/off logic for each different port/webserver.


### Demo

[![Demo video](http://img.youtube.com/vi/cTZIDS0zHwI/0.jpg)](https://www.youtube.com/watch?v=cTZIDS0zHwI "Demo video")


### Reference

http://www.makermusings.com/2015/07/18/virtual-wemo-code-for-amazon-echo/
