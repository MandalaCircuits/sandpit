/*
 IMPORTANT!!!!
 You should have mc_loginInfo.h to be incuded that defines 
 MC_LOGININFO_SSID and MC_LOGININFO_PASSWORD,  the account 
 info to connect your wifi network. mc_loginInfo.h should
 NOT be under source control for security reasons.
*/ 

#include "mc_loginInfo.h"
#include <ESP8266WiFi.h>

class WebPort
{
private:
  byte channel;
  bool state; // note: one byte could store channel on first 4 bits and state on 5th
  
public:
  WebPort(byte channel) : channel(channel)
  {
    state = false;
    Serial.write("Initalizing WebPort: ");
    Serial.println(this->channel);
  }

  void setState(bool newState)
  {
    state = newState;
  }

  bool getState()
  {
    return state;
  }

  void toHTML(String& s)
  {
    if (0 == channel % 4)
      s += "<br>";
      
    s += "<a href=\"/";
    if (state)
      s += "OFF";
    else
      s += "ON";
    s += channel;
    s += "\"><button>";

    s += channel;
    s += ": ";
    if (state)
      s += "ON";
     else
      s += "OFF";
    s += "</button></a>";
  }
};

#define NumPorts 16
WebPort* webPorts[NumPorts];
WiFiServer server(80);
bool disabledState = false;

void initPorts()
{
  for (int i = 0; i < NumPorts; i++)
  {
    webPorts[i] = new WebPort(i);
  }
}

void applyState(byte portNumber, bool newState)
{
   webPorts[portNumber]->setState(newState);
}

void portsToWeb(String &s)
{
    for (int i = 0; i < NumPorts; i++)
    {
      webPorts[i]->toHTML(s);
    }
}

bool statesToRestore[NumPorts];
void disableAll()
{
  Serial.println("DISABLING ALL");
  for (int i = 0; i < NumPorts; i++)
  {
    statesToRestore[i] = webPorts[i]->getState();
    webPorts[i]->setState(false);
  }
  disabledState = true;
}

void restoreAll()
{
  Serial.println("RESTORING ALL");
  for (int i = 0; i < NumPorts; i++)
  {
    webPorts[i]->setState(statesToRestore[i]);
  }
  disabledState = false;
}

void disableRestoreToWeb(String& s)
{
    s += "<br><br>";
      
    s += "<a href=\"/";
    if (disabledState)
      s += "RESTORE";
    else
      s += "DISABLE";
    s += "\"><button>";

    if (disabledState)
      s += "RESTORE ALL";
     else
      s += "DISABLE ALL";
    s += "</button></a>";
}
 
void setup() 
{
  Serial.begin(115200);
  delay(10);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(MC_LOGININFO_SSID);

  WiFi.begin(MC_LOGININFO_SSID, MC_LOGININFO_PASSWORD);
 
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

   initPorts();

  // Start the server
  server.begin();
  Serial.println("Server started");
 
  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
 
}
 
void loop() 
{
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) 
  {
    return;
  }
 
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available())
  {
    delay(1);
  }
 
  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);

  int subStringIndex = request.indexOf("HTTP");
  if (-1 != subStringIndex)
  {
    request = request.substring(0, subStringIndex);
  }
 
  client.flush();
 
  // Match the request
  String portNumberAsString;
  bool newState;
  bool portCommandFound = false;

  subStringIndex = request.indexOf("/ON");
  if (-1 != subStringIndex)
  {
    portCommandFound = true;
    newState = true;
    portNumberAsString = request.substring(request.indexOf("/ON") + 3);
  }
  else 
  {
    subStringIndex = request.indexOf("/OFF");  
    if (-1 != subStringIndex)
    {
      portCommandFound = true;
      newState = false;
      portNumberAsString = request.substring(request.indexOf("/OFF") + 4);
    }
  }
  
  bool redirectRequired = false;
  if (portCommandFound)
  {
    int portNumber = portNumberAsString.toInt();
    Serial.write("portNumber: ");
    Serial.println(portNumber);
    Serial.write("new state: ");
    Serial.println(newState);

    applyState(portNumber, newState);
	redirectRequired = true;
  }
  else
  {
    if (0 < request.indexOf("/DISABLE"))
    {
      disableAll();
	  redirectRequired = true;
    }
    else if (0 < request.indexOf("/RESTORE"))
    {
      restoreAll();
	  redirectRequired = true;
    }
  }

  static const char* HTMLHeaderCommon = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML><html><head><meta HTTP-EQUIV=\"REFRESH\" content=\"";
  String html = HTMLHeaderCommon;

  if (redirectRequired)
  {
	  html += "0;url=/";
  }
  else
  {
	  html += "5";
  }

  html += "\"><style>button{-webkit-border-radius: 14;-moz-border-radius: 14;border-radius: 14px;font-family: Arial;color: #ffffff;font-size: 20px;background: #c957c1;padding: 10px 20px 10px 20px;text-decoration: none;width: 160px;}button:hover{background: #893cfc;text-decoration: none;}</style></head><body>";
  portsToWeb(html);
  disableRestoreToWeb(html);
  
  html += "</body></html>";
  client.println(html);
  client.flush();
 
  delay(1);
  Serial.println("Client disconnected");
  Serial.println("");
}
