/******************
  This sketch has an ESP8266 receiving either HTTP requests
  or Amazon Echo commands via WiFi to control 433MHz 
  commands to RF remote sockets and IR transmissions for 
  STB or other devices. 
  The 433MHz transmission is initiated on GPIO2 on the ESP (Arduino Pin 16) if using discreet components
  A SSD1306 OLED display is used to show the state of commands to the 
  devices (TV, HiFi and AV). The OLED is using I2C connectivity
  on device address 0x3C. 
  Included is the IR library (IRremoteESP8266.h) so that we can trigger IR remote commands
  and emulate hand held remotes. This one is using the NEC protocol for a BT YouView set top box
******************/



#include <ESP8266WiFi.h>
#include "fauxmoESP.h"
#include <RCSwitch.h>
#include <IRremoteESP8266.h>
//#include "IRremote.h"
//#include <SPI.h>
//#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans24pt7b.h>


IRsend irsend(12); //an IR led is connected to GPIO pin 12 (Wemos pin D6) if using discreet components


// the IP address for the shield:
//IPAddress ip(192, 168, 1, 100)......fixed IP so that we can access without DNS changes.....

// RCSwitch configuration
RCSwitch mySwitch = RCSwitch();  // 433MHz Transmitter
RCSwitch s3Switch = RCSwitch();  // Energenie sockets 433MHz Transmitter

// Create an instance of the server on Port 85 for webpage
WiFiServer server(85); // using port 85 so that router can have port forwarding set for particular device
IPAddress ip(192, 168, 1, 100); // where xx is the desired IP Address
IPAddress gateway(192, 168, 1, 254); // set gateway to match your network

WiFiClient client;
unsigned long ulReqcount;        // how often has a valid page been requested


// Network configuration
fauxmoESP fauxmo;
#define WIFI_SSID "BTHub5-C7JX"
#define WIFI_PASS "834f4e89af"



// Declared for the webpage


// Allow access to ESP8266 SDK
extern "C" 
{
#include "user_interface.h"
}

// No IO Pin for Reset nedded, if oled Res 100nF to ground and 10k to 3.3V
#define OLED_RESET -1 
Adafruit_SSD1306 display(OLED_RESET);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif        

void setup() {
    irsend.begin();
    Serial.begin(115200);
    // initialise OLED display by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
     display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //)0x3C is the I2C address of the OLED display
    // init done

    // Clear the buffer.
    display.clearDisplay();
    
    // text display setup
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    Serial.println();
    Serial.println();
    IPAddress subnet(255, 255, 255, 0); // set subnet mask to match your network
    WiFi.config(ip, gateway, subnet); 
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }

    Serial.printf("\r\n\r\n[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
   display.println( WiFi.SSID()); //serial display of your routers SSID
   display.println( WiFi.localIP()); //serial display of your ESP8266 IP address
   display.display();
    server.begin();
    mySwitch.enableTransmit( 4 );   // 433MHz Transmitter
    s3Switch.enableTransmit( 16 );   // Energenie 433MHz Transmitter for Tv,HiFi....uses D2 on Wemos D1 board.
    
    // Fauxmo....these are going to be the names that Echo is going to recognise for your devices
    fauxmo.addDevice("lamp");
    fauxmo.addDevice("hall");
    fauxmo.addDevice("Sat");
    fauxmo.addDevice("TV");
    fauxmo.addDevice("hifi");
    fauxmo.addDevice("AV");
    
    
    // fauxmoESP 2.0.0 has changed the callback signature to add the device_id
    // it's easier to match devices to action without having to compare strings.
    fauxmo.onMessage([](unsigned char device_id, const char * device_name, bool state)
    {
        Serial.printf("[MAIN] Device #%d (%s) state: %s\n", device_id, device_name, state ? "ON" : "OFF");

    SwItchRF( device_id, state );
    });

}
// -----------------------------------------------------------------------------
// Switch the RF433/Energenie 433MHz Sockets on/off
// -----------------------------------------------------------------------------
void SwItchRF( unsigned char deviceid, boolean Mystate ){
        if(deviceid == 0){          // Livingroom Lamp (433MHz)
        if(Mystate==true){
        mySwitch.switchOn(1,1);
        }else{
        mySwitch.switchOff(1,1);
        }
        }else if(deviceid==1){      // Hall White LED's (433MHz)
        if(Mystate==true){
        mySwitch.switchOn(1,2);
        }else{
        mySwitch.switchOff(1,2);
        }
        }else if(deviceid==2){      // SAT IR
        if(Mystate==true){
        irsend.sendNEC(0x0800FF, 32);        //send NEC code to IR tx pin (Sat On)
        Serial.println ("NEC IR on");
        delay(1000);
        display.setTextColor(0xFFFF, 0);    //set the colour to black so we overwrite the previous line with nothing to blank the display    
        display.setCursor(0,20);
        display.println("              ");
        display.display();
        display.setTextColor(1);
        display.setCursor(0,20);
        display.println("Turn Sat On");
        display.display();
        }else{ 
        irsend.sendNEC(0xFFA25D, 36);     
        display.setTextColor(0xFFFF, 0);    // repeat of procedure above
        display.setCursor(0,30);
        display.println("              ");
        display.display();
        display.setTextColor(1);
        display.setCursor(0,30);
        display.println("Turn Sat Off");
        display.display();
       }   
        }else if(deviceid == 3){            // TV (433MHz) Turns one Energenie 433MHz socket on/off
        if(Mystate==true){
        s3Switch.send(7585927, 24);         //this is the 24bit RF code that switches the 1st socket on
        display.setTextColor(0xFFFF, 0);    //set the colour to black so we overwrite the previous line with nothing
        display.setCursor(0,20);            //position the cursor in the same position as the last printed line
        display.println("              ");  //overwrite spaces so that the line is blank
        display.display();                  // print the blank buffer
        display.setTextColor(1);            // change the colour back to white so we can display the next command
        display.setCursor(0,20);  
        display.println("Turn TV On");
        display.display();
        }else{
        s3Switch.send(7585926, 24);         //this is the 24bit RF code that switches the 1st socket off
        display.setTextColor(0xFFFF, 0);    // repeat of procedure above
        display.setCursor(0,30);
        display.println("              ");
        display.display();
        display.setTextColor(1);
        display.setCursor(0,30);
        display.println("Turn TV Off");
        display.display();
        }
        }else if(deviceid==4){      // HiFi Turns 2nd Energenie 433MHz socket on/off
        if(Mystate==true){
        s3Switch.send(7585935, 24);
        display.setTextColor(0xFFFF, 0);
        display.setCursor(0,20);
        display.println("              ");
        display.display();
        display.setTextColor(1);
        display.setCursor(0,20);
        display.println("Turn HiFi On");
        display.display();
        }else{
        s3Switch.send(7585934, 24);
        display.setTextColor(0xFFFF, 0);
        display.setCursor(0,30);
        display.println("              ");
        display.display();
        display.setTextColor(1);
        display.setCursor(0,30);
        display.print("Turn HiFi Off");
        display.display();
        }
        }else if(deviceid == 5){    // AV Turns ALL Energenie 433MHz socket on/off
        if(Mystate==true){
        s3Switch.send(7585933, 24);
        display.setTextColor(0xFFFF, 0);
        display.setCursor(0,20);
        display.println("              ");
        display.display();
        display.setTextColor(1);
        display.setCursor(0,20);
        display.println("Turn AV On");
        display.display();
        }else{
        s3Switch.send(7585932, 24);
        display.setTextColor(0xFFFF, 0);
        display.setCursor(0,30);
        display.println("              ");
        display.display();
        display.setTextColor(1);
        display.setCursor(0,30);
        display.println("Turn AV Off");
        display.display();
        }
        }else{return;}
}


void loop() {

    // Since fauxmoESP 2.0 the library uses the "compatibility" mode by
    // default, this means that it uses WiFiUdp class instead of AsyncUDP.
    // The later requires the Arduino Core for ESP8266 staging version
    // whilst the former works fine with current stable 2.3.0 version.
    // But, since it's not "async" anymore we have to manually poll for UDP
    // packets
    fauxmo.handle();

  //----------------------------------------------------------------
  // Check if a client has connected
  //----------------------------------------------------------------
  WiFiClient client = server.available();
  if (!client) 
  {
    return;
  }
  
  // Wait until the client sends some data
  unsigned long ultimeout = millis()+250;
  while(!client.available() && (millis()<ultimeout) )
  {
    delay(1);
  }
  if(millis()>ultimeout) 
  { 
    return; 
  }
  
  //----------------------------------------------------------------
  // Read the first line of the request
  //----------------------------------------------------------------
  String sRequest = client.readStringUntil('\r');
  client.flush();
  
  // stop client, if request is empty
  if(sRequest=="")
  {
    client.stop();
    return;
  }
  
  // get path; end of path is either space or ?
  // Syntax is e.g. GET /?show=1234 HTTP/1.1
  String sPath="",sParam="", sCmd="";
  String sGetstart="GET ";
  int iStart,iEndSpace,iEndQuest;
  iStart = sRequest.indexOf(sGetstart);
  if (iStart>=0)
  {
    iStart+=+sGetstart.length();
    iEndSpace = sRequest.indexOf(" ",iStart);
    iEndQuest = sRequest.indexOf("?",iStart);
    
    // are there parameters?
    if(iEndSpace>0)
    {
      if(iEndQuest>0)
      {
        // there are parameters
        sPath  = sRequest.substring(iStart,iEndQuest);
        sParam = sRequest.substring(iEndQuest,iEndSpace);
      }
      else
      {
        // NO parameters
        sPath  = sRequest.substring(iStart,iEndSpace);
      }
    }
  }
 
  
  //----------------------------------------------------------------
  // format the html response
  //----------------------------------------------------------------


if(sPath=="/")
  {   
   ulReqcount++;
   String diagdat="";
   String duration1 = " ";
   int hr,mn,st;
   st = millis() / 1000;
   mn = st / 60;
   hr = st / 3600;
   st = st - mn * 60;
   mn = mn - hr * 60;
   if (hr<10) {duration1 += ("0");}
   duration1 += (hr);
   duration1 += (":");
   if (mn<10) {duration1 += ("0");}
   duration1 += (mn);
   duration1 += (":");
   if (st<10) {duration1 += ("0");}
   duration1 += (st);     
   client.println("HTTP/1.1 200 OK"); 
   client.println("Content-Type: text/html");
   client.println("Connection: close");
   client.println();
   client.println("<!DOCTYPE HTML>");
   diagdat+="<html><head><title>Home Monitor</title></head><body>";
   diagdat+="<font color=\"#000000\"><body bgcolor=\"#a0dFfe\">";
   diagdat+="<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\">";
   diagdat+="<h1>Alexa Home Voice / Webpage Control<BR>ESP8266 on Wemos D1 board</h1>";
   diagdat+="<BR>  Web Page Requests = ";
   diagdat+=ulReqcount;                            
   diagdat+="<BR>  Free RAM = ";
   client.println(diagdat);
   client.print((uint32_t)system_get_free_heap_size()/1024);
   diagdat=" KBytes";            
   diagdat+="<BR>  System Uptime =";
   diagdat+=duration1;                                                             
   client.print(diagdat);
   diagdat="<BR><hr><BR><table><tr><td>";//  Webpage buttons for RF 433MHz and IR devices in HTML Table
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=1-on\"><INPUT TYPE=\"submit\" VALUE=\"Lamp On\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=2-on\"><INPUT TYPE=\"submit\" VALUE=\"Hall LED On\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=3-on\"><INPUT TYPE=\"submit\" VALUE=\"Sat On\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=4-on\"><INPUT TYPE=\"submit\" VALUE=\"TV On\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=5-on\"><INPUT TYPE=\"submit\" VALUE=\"hifi On\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=6-on\"><INPUT TYPE=\"submit\" VALUE=\"AV On\"></FORM><br></td></tr><tr><td>";                                
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=1-off\"><INPUT TYPE=\"submit\" VALUE=\"Lamp Off\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=2-off\"><INPUT TYPE=\"submit\" VALUE=\"Hall LED Off\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=3-off\"><INPUT TYPE=\"submit\" VALUE=\"Sat Off\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=4-off\"><INPUT TYPE=\"submit\" VALUE=\"TV Off\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=5-off\"><INPUT TYPE=\"submit\" VALUE=\"hifi Off\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=6-off\"><INPUT TYPE=\"submit\" VALUE=\"AV Off\"></FORM><br></td></tr></table>";
   client.print(diagdat);
   diagdat="<hr><BR>";
   diagdat+="<BR><FONT SIZE=-1>environmental.monitor.log@gmail.com<BR><FONT SIZE=-1>ESP8266 With RF 433MHz + 315MHz <BR> FauxMO(WEMO Clone) to Amazon ECHO Dot Gateway<BR>";
   diagdat+="<FONT SIZE=-2>Compiled Using ESP ver. 2.2.3(Arduino 1.6.13), built March, 2017<BR></body></html>";
   client.println(diagdat);
   diagdat = "";
   duration1 = "";
   // and stop the client
   delay(1);
   client.stop();
  }
  
if (sPath.startsWith("/&socKet=")){          // Request from webpage buttons
   client.println("HTTP/1.1 204 OK");        // No Data response to buttons request
   client.println("Connection: close");      // We are done Close the connection
   delay(1);                                 // Give time for connection to close
   client.stop();                            // Stop the client
   ulReqcount++;                             // Tracking counter for page requests
   unsigned char device_id;
   boolean state;
   if (sPath.startsWith("/&socKet=1-on")) {
    device_id=0;
    state=true;
    SwItchRF( device_id, state );            // Button request to SwItchRF()
  } else if (sPath.startsWith("/&socKet=1-off")) {
    device_id=0;
    state=false;
    SwItchRF( device_id, state );
  } else if (sPath.startsWith("/&socKet=2-on")) {
    device_id=1;
    state=true;
    SwItchRF( device_id, state );
  } else if (sPath.startsWith("/&socKet=2-off")) {
    device_id=1;
    state=false;
    SwItchRF( device_id, state );
  } else if (sPath.startsWith("/&socKet=3-on")) {
    device_id=2;
    state=true;
    SwItchRF( device_id, state );
  } else if (sPath.startsWith("/&socKet=3-off")) {
    device_id=2;
    state=false;
    SwItchRF( device_id, state );
  } else if (sPath.startsWith("/&socKet=4-on")) {
    device_id=3;
    state=true;
    SwItchRF( device_id, state );
  } else if (sPath.startsWith("/&socKet=4-off")) {
    device_id=3;
    state=false;
    SwItchRF( device_id, state );
  } else if (sPath.startsWith("/&socKet=5-on")) {
    device_id=4;
    state=true;
    SwItchRF( device_id, state );
  } else if (sPath.startsWith("/&socKet=5-off")) {
    device_id=4;
    state=false;
    SwItchRF( device_id, state );
  } else if (sPath.startsWith("/&socKet=6-on")) {
    device_id=5;
    state=true;
    SwItchRF( device_id, state );
  } else if (sPath.startsWith("/&socKet=6-off")) {
    device_id=5;
    state=false;
    SwItchRF( device_id, state );
  }else{return;}
    
}
} 
