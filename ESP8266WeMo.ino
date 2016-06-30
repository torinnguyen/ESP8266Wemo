#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUDP.h>

const char* ssid = "---";                       // your network SSID (name)
const char* pass = "---";                       // your network password

const char* friendlyName = "Arduino";           // Alexa will use this name to identify your device
const char* serialNumber = "221517K0101768";                  // anything will do
const char* uuid = "904bfa3c-1de2-11v2-8728-fd8eebaf492d";    // anything will do

// Multicast declarations
IPAddress ipMulti(239, 255, 255, 250);
const unsigned int portMulti = 1900;
const unsigned int webserverPort = 9876;

// Witty Cloud Board specifc pins
const int LED_PIN = 13;

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

//int status = WL_IDLE_STATUS;

WiFiUDP Udp;
byte packetBuffer[512]; //buffer to hold incoming and outgoing packets

ESP8266WebServer server(webserverPort);


//-----------------------------------------------------------------------
// UDP Multicast Server
//-----------------------------------------------------------------------

char* getDateString()
{
  //Doesn't matter which date & time, will work
  //Optional: replace with NTP Client implementation
  return "Wed, 29 Jun 2016 00:13:46 GMT";
}

void responseToSearchUdp(IPAddress& senderIP, unsigned int senderPort) 
{
  Serial.println("responseToSearchUdp");

  //This is absolutely neccessary as Udp.write cannot handle IPAddress or numbers correctly like Serial.print
  IPAddress myIP = WiFi.localIP();
  char ipChar[20];
  snprintf(ipChar, 20, "%d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);
  char portChar[7];
  snprintf(portChar, 7, ":%d", webserverPort);

  Udp.beginPacket(senderIP, senderPort);
  Udp.write("HTTP/1.1 200 OK\r\n");
  Udp.write("CACHE-CONTROL: max-age=86400\r\n");
  Udp.write("DATE: ");
  Udp.write(getDateString());
  Udp.write("\r\n");
  Udp.write("EXT:\r\n");
  Udp.write("LOCATION: ");
  Udp.write("http://");
  Udp.write(ipChar);
  Udp.write(portChar);
  Udp.write("/setup.xml\r\n");
  Udp.write("OPT: \"http://schemas.upnp.org/upnp/1/0/\"); ns=01\r\n");
  Udp.write("01-NLS: ");
  Udp.write(uuid);
  Udp.write("\r\n");
  Udp.write("SERVER: Unspecified, UPnP/1.0, Unspecified\r\n");
  Udp.write("X-User-Agent: redsonic\r\n");
  Udp.write("ST: urn:Belkin:device:**\r\n");
  Udp.write("USN: uuid:Socket-1_0-");
  Udp.write(serialNumber);
  Udp.write("::urn:Belkin:device:**\r\n");
  Udp.write("\r\n");
  Udp.endPacket();
}

void UdpMulticastServerLoop()
{
  int numBytes = Udp.parsePacket();
  if (numBytes <= 0)
    return;

  IPAddress senderIP = Udp.remoteIP();
  unsigned int senderPort = Udp.remotePort();
  
  // read the packet into the buffer
  Udp.read(packetBuffer, numBytes); 

  // print out the received packet
  //Serial.write(packetBuffer, numBytes);

  // check if this is a M-SEARCH for WeMo device
  String request = String((char *)packetBuffer);
  int mSearchIndex = request.indexOf("M-SEARCH");
  int mBelkinIndex = request.indexOf("urn:Belkin:device:");   //optional
  if (mSearchIndex < 0 || mBelkinIndex < 0)
    return;

  // send a reply, to the IP address and port that sent us the packet we received
  responseToSearchUdp(senderIP, senderPort);
}


//-----------------------------------------------------------------------
// HTTP Server
//-----------------------------------------------------------------------

void handleRoot() 
{
  Serial.println("handleRoot");

  server.send(200, "text/plain", "Tell Alexa to discover devices");
}

void handleSetupXml()
{
  Serial.println("handleSetupXml");
    
  String body = "<?xml version=\"1.0\"?>\r\n";
  body += "<root>\r\n";
  body += "  <device>\r\n";
  body += "    <deviceType>urn:OriginallyUS:device:controllee:1</deviceType>\r\n";
  body += "    <friendlyName>";
  body += friendlyName;
  body += "</friendlyName>\r\n";
  body += "    <manufacturer>Belkin International Inc.</manufacturer>\r\n";
  body += "    <modelName>Emulated Socket</modelName>\r\n";
  body += "    <modelNumber>3.1415</modelNumber>\r\n";
  body += "    <UDN>uuid:Socket-1_0-";
  body += serialNumber;
  body += "</UDN>\r\n";
  body += "  </device>\r\n";
  body += "</root>";

  String header = "HTTP/1.1 200 OK\r\n";
  header += "Content-Type: text/xml\r\n";
  header += "Content-Length: ";
  header += body.length();
  header += "\r\n";
  header += "Date: ";
  header += getDateString();
  header += "\r\n";
  header += "X-User-Agent: redsonic\r\n";
  header += "SERVER: Unspecified, UPnP/1.0, Unspecified\r\n";
  header += "connection: close\r\n";
  header += "LAST-MODIFIED: Sat, 01 Jan 2000 00:00:00 GMT\r\n";
  header += "\r\n";
  header += body;

  Serial.println(header);
  
  server.sendContent(header);
}

void handleUpnpControl()
{
  Serial.println("handleUpnpControl");

  //Extract raw body
  //Because there is a '=' before "1.0", it will give the following:
  //"1.0" encoding="utf-8"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body><u:SetBinaryState xmlns:u="urn:Belkin:service:basicevent:1"><BinaryState>1</BinaryState></u:SetBinaryState></s:Body></s:Envelope>
  String body = server.arg(0);

  //Check valid request
  boolean isOn = body.indexOf("<BinaryState>1</BinaryState>") >= 0;
  boolean isOff = body.indexOf("<BinaryState>0</BinaryState>") >= 0;
  boolean isValid = isOn || isOff;
  if (!isValid) {
    Serial.println("Bad request from Amazon Echo");
    Serial.println(body);
    server.send(400, "text/plain", "Bad request from Amazon Echo");
    return;
  }

  //On/Off Logic
  if (isOn) {
      digitalWrite(LED_PIN, 1);
      Serial.println("Alexa is asking to turn ON a device");
      server.send(200, "text/plain", "Turning ON device");
  }
  else {
      digitalWrite(LED_PIN, 0);
      Serial.println("Alexa is asking to turn OFF a device");
      server.send(200, "text/plain", "Turning OFF device");
  }
}

void handleNotFound()
{
  Serial.println("handleNotFound()");
  
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  Serial.println();

  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, 0);
  
  // setting up Station AP
  WiFi.begin(ssid, pass);

  // Wait for connect to AP
  Serial.print("[Connecting to ");
  Serial.print(ssid);
  Serial.print("]...");
  int tries=0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
    tries++;
    if (tries > 50)
      break;
  }
  
  // print your WiFi info
  IPAddress ip = WiFi.localIP();
  Serial.println();
  Serial.print("Connected to ");
  Serial.print(WiFi.SSID());
  Serial.print(" with IP: ");
  Serial.println(ip);
  
  //UDP Server
  Udp.beginMulticast(WiFi.localIP(),  ipMulti, portMulti);
  Serial.print("Udp multicast server started at ");
  Serial.print(ipMulti);
  Serial.print(":");
  Serial.println(portMulti);
  
  //Web Server
  server.on("/", handleRoot);
  server.on("/setup.xml", handleSetupXml);
  server.on("/upnp/control/basicevent1", handleUpnpControl);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.print("HTTP server started on port ");
  Serial.println(webserverPort);
}

void loop()
{
  UdpMulticastServerLoop();   //UDP multicast receiver

  server.handleClient();      //Webserver
}


