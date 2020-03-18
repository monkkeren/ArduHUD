#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include "BluetoothSerial.h"
#include "SerialTransfer.h"
#include "ELMduino.h"


BluetoothSerial SerialBT;
#define DEBUG_PORT      Serial
#define LED_DRIVER_PORT Serial2
#define ELM_PORT        SerialBT


const char *ssid = "HUDuino";
const char MAIN_page[] PROGMEM = "<!DOCTYPE html>\n<html>\n<style>\nbody, html {\n  height: 100%;\n  margin: 0;\n  font-family: Arial;\n}\n\n/* Style tab links */\n.tablink {\n  background-color: #555;\n  color: white;\n  float: left;\n  border: none;\n  outline: none;\n  cursor: pointer;\n  padding: 14px 16px;\n  font-size: 17px;\n  width: 33.33%;\n}\n\n.tablink:hover {\n  background-color: #777;\n}\n\n.tabcontent {\n  color: white;\n  display: none;\n  padding: 100px 20px;\n  height: 100%;\n}\n\n#Home {background-color: white;}\n#PIDs {background-color: white;}\n#Console {background-color: white;}\n#About {background-color: white;}\n\n.card\n{\n    max-width: 400px;\n     min-height: 250px;\n     background: #02b875;\n     padding: 30px;\n     box-sizing: border-box;\n     color: #FFF;\n     margin:20px;\n     box-shadow: 0px 2px 18px -4px rgba(0,0,0,0.75);\n}\n\n#myProgress\n{\n  width: 100%;\n  background-color: #ddd;\n}\n\n#myBar\n{\n  width: 1%;\n  height: 30px;\n  background-color: #4CAF50;\n}\n\n/* Style the form - display items horizontally */\n.form-inline {\n  display: flex;\n  flex-flow: row wrap;\n  align-items: center;\n}\n\n/* Add some margins for each label */\n.form-inline label {\n  margin: 5px 10px 5px 0;\n}\n\n/* Style the input fields */\n.form-inline input {\n  vertical-align: middle;\n  margin: 5px 10px 5px 0;\n  padding: 10px;\n  background-color: #fff;\n  border: 1px solid #ddd;\n}\n\n/* Style the submit button */\n.form-inline button {\n  padding: 10px 20px;\n  background-color: dodgerblue;\n  border: 1px solid #ddd;\n  color: white;\n}\n\n.form-inline button:hover {\n  background-color: royalblue;\n}\n\n/* Add responsiveness - display the form controls vertically instead of horizontally on screens that are less than 800px wide */\n@media (max-width: 800px) {\n  .form-inline input {\n    margin: 10px 0;\n  }\n\n  .form-inline {\n    flex-direction: column;\n    align-items: stretch;\n  }\n}\n</style>\n\n<body>\n<button class=\"tablink\" onclick=\"openPage('Home', this, 'red')\" id=\"defaultOpen\">Home</button>\n<button class=\"tablink\" onclick=\"openPage('PIDs', this, 'green')\">PIDs</button>\n<button class=\"tablink\" onclick=\"openPage('Console', this, 'blue')\">Console</button>\n\n<div id=\"Home\" class=\"tabcontent\">\n  <h3>Home</h3>\n  <div class=\"card\">\n    <h4>ArduHUD</h4><br>\n    <h1>Speed: <span id=\"speed\">0</span></h1>\n    <div id=\"myProgress\">\n      <div id=\"myBar\"></div>\n    </div><br>\n  </div>\n</div>\n\n<div id=\"PIDs\" class=\"tabcontent\">\n  <h3>PIDs</h3>\n  <p>Some PIDs this fine day!</p>\n</div>\n\n<div id=\"Console\" class=\"tabcontent\">\n  <h3>Console</h3>\n  <form class=\"form-inline\" action=\"/action_page.php\">\n    <label for=\"email\">Email:</label>\n    <input type=\"email\" id=\"email\" placeholder=\"Enter email\" name=\"email\">\n    <button type=\"submit\">Submit</button>\n</form>\n</div>\n</body>\n\n<script>\nfunction openPage(pageName, elmnt, color)\n{\n  // Hide all elements with class=\"tabcontent\" by default */\n  var i, tabcontent, tablinks;\n  tabcontent = document.getElementsByClassName(\"tabcontent\");\n  for (i = 0; i < tabcontent.length; i++)\n  {\n    tabcontent[i].style.display = \"none\";\n  }\n\n  // Remove the background color of all tablinks/buttons\n  tablinks = document.getElementsByClassName(\"tablink\");\n  for (i = 0; i < tablinks.length; i++)\n  {\n    tablinks[i].style.backgroundColor = \"\";\n  }\n\n  // Show the specific tab content\n  document.getElementById(pageName).style.display = \"block\";\n\n  // Add the specific color to the button used to open the tab content\n  elmnt.style.backgroundColor = color;\n}\n\ndocument.getElementById(\"defaultOpen\").click();\n\nsetInterval(function()\n{\n  getSpeedData();\n  getRpmData();\n}, 100);\n\n\nfunction getSpeedData()\n{\n  var xhttp = new XMLHttpRequest();\n  \n  xhttp.onreadystatechange = function()\n  {\n    if (this.readyState == 4 && this.status == 200)\n      {\n      document.getElementById(\"speed\").innerHTML = this.responseText;\n    }\n  };\n  \n  xhttp.open(\"GET\", \"speed\", true);\n  xhttp.send();\n}\n\n\nfunction move(width)\n{\n  var elem = document.getElementById(\"myBar\");\n  var id = setInterval(frame, 10);\n  \n  function frame()\n  {\n    if (width > 100)\n    {\n      width = 100;\n    }\n    else if (width < 0)\n    {\n      width = 0;\n    }\n    \n    console.log(width);\n    elem.style.width = width + \"%\";\n  }\n}\n\nfunction getRpmData()\n{\n  var xhttp = new XMLHttpRequest();\n  \n  xhttp.onreadystatechange = function()\n  {\n    if (this.readyState == 4 && this.status == 200)\n       {\n       move(parseInt(this.responseText), 10);\n    }\n  };\n  \n  xhttp.open(\"GET\", \"rpm\", true);\n  xhttp.send();\n}\n</script>\n</html>";


WiFiServer server(80);
SerialTransfer myTransfer;
ELM327 myELM327;


enum fsm{get_speed, 
         get_rpm};
fsm state = get_rpm;


struct STRUCT {
  int8_t status;
  uint32_t rpm;
  float mph;
} carTelem;


void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  DEBUG_PORT.begin(115200);
  LED_DRIVER_PORT.begin(115200);

  WiFi.softAP(ssid);
  IPAddress myIP = WiFi.softAPIP();
  DEBUG_PORT.println(myIP);
  server.begin();

  //////////////////
  carTelem.rpm = 2000;
  carTelem.mph = 50;
  
  while(1)
    serverProcessing();
  //////////////////

  // Wait for ELM327 to init
  delay(3000);
  
  ELM_PORT.begin("ArduHUD", true);
  
  if (!ELM_PORT.connect("OBDII"))
  {
    DEBUG_PORT.println("Couldn't connect to OBD scanner");
    while(1);
  }
  
  myELM327.begin(ELM_PORT);
  myTransfer.begin(LED_DRIVER_PORT);

  while(!myELM327.connected)
    myELM327.initializeELM();
}


void loop()
{
  switch(state)
  {
    case get_rpm:
    {
      float tempRPM = myELM327.rpm();

      carTelem.status = myELM327.status;
      
      if(myELM327.status == ELM_SUCCESS)
        carTelem.rpm = tempRPM;
      else
        printError();
      
      state = get_speed;
      break;
    }

    case get_speed:
    {
      float tempMPH = myELM327.mph();

      carTelem.status = myELM327.status;
      
      if(myELM327.status == ELM_SUCCESS)
        carTelem.mph = tempMPH;
      else
        printError();
      
      state = get_rpm;
      break;
    }
  }

  myTransfer.txObj(carTelem, sizeof(carTelem));
  myTransfer.sendData(sizeof(carTelem));

  serverProcessing();
}


void serverProcessing()
{
  bool mainPage = false;
  bool speedCall = false;
  bool rpmCall = false;
  
  WiFiClient client = server.available();

  if (client)
  {
    String currentLine = "";
    
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        Serial.write(c);

        if (currentLine.endsWith("GET / HTTP"))
          mainPage = true;
        else if (currentLine.endsWith("GET /speed HTTP"))
          speedCall = true;
        else if (currentLine.endsWith("GET /rpm HTTP"))
          rpmCall = true;
        
        if (c == '\n')
        {
          if (!currentLine.length())
          {
            if (mainPage)
            {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();
        
              client.print(MAIN_page);
              client.println();
      
              mainPage = false;
              break;
            }
            else if (speedCall)
            {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();
        
              client.println(int(carTelem.mph + 0.5));
              client.println();
      
              speedCall = false;
              break;
            }
            else if (rpmCall)
            {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();
        
              client.println(carTelem.rpm);
              client.println();
      
              rpmCall = false;
              break;
            }
            else
            {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();
        
              client.print(MAIN_page);
              client.println();
              break;
            }
          }
          else
            currentLine = "";
        }
        else if (c != '\r')
          currentLine += c;
      }
    }

    client.stop();
  }
}


void printError()
{
  DEBUG_PORT.print("Received: ");
  for (byte i = 0; i < PAYLOAD_LEN; i++)
    DEBUG_PORT.write(myELM327.payload[i]);
  DEBUG_PORT.println();
  
  if (myELM327.status == ELM_SUCCESS)
    DEBUG_PORT.println(F("\tELM_SUCCESS"));
  else if (myELM327.status == ELM_NO_RESPONSE)
    DEBUG_PORT.println(F("\tERROR: ELM_NO_RESPONSE"));
  else if (myELM327.status == ELM_BUFFER_OVERFLOW)
    DEBUG_PORT.println(F("\tERROR: ELM_BUFFER_OVERFLOW"));
  else if (myELM327.status == ELM_UNABLE_TO_CONNECT)
    DEBUG_PORT.println(F("\tERROR: ELM_UNABLE_TO_CONNECT"));
  else if (myELM327.status == ELM_NO_DATA)
    DEBUG_PORT.println(F("\tERROR: ELM_NO_DATA"));
  else if (myELM327.status == ELM_STOPPED)
    DEBUG_PORT.println(F("\tERROR: ELM_STOPPED"));
  else if (myELM327.status == ELM_TIMEOUT)
    DEBUG_PORT.println(F("\tERROR: ELM_TIMEOUT"));
  else if (myELM327.status == ELM_TIMEOUT)
    DEBUG_PORT.println(F("\tERROR: ELM_GENERAL_ERROR"));

  delay(100);
}
