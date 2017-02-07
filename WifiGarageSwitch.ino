 /**
 *  ESP8266 Wifi Garage Switch 
 *  Source code can be found here: https://github.com/mbeckner554/SmartThingsGarageDoor/blob/master/WifiGarageSwitch.ino
 *  Copyright 2017 Mbeckner
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 *  in compliance with the License. You may obtain a copy of the License at:
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
 *  on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License
 *  for the specific language governing permissions and limitations under the License.
 *  
 *  Derived From:
 *    - https://github.com/JZ-SmartThings/SmartThings/blob/master/Devices/Generic%20HTTP%20Device
 */
#include <EEPROM.h>

// SET YOUR NETWORK MODE TO USE WIFI OR ENC28J60
#define useWIFI true
const char* ssid = "networkIDhere";
const char* password = "passwordGOEShere";

// DECIDE WHETHER TO SEND HIGH OR LOW TO THE PINS AND WHICH PINS ARE USED FOR TRIGGERS
// IF USING 3.3V RELAY, TRANSISTOR OR MOSFET THEN SET THE BELOW VARIABLE TO FALSE
const bool use5Vrelay = true;
int alarmPin1 = D6; // GPIO12 = D6
int relayPin1 = D1; // GPIO5 = D1
int relayPin2 = D2; // GPIO4 = D2
int notificationPin1 = D9; // GPIO3 = D9

// USE BASIC HTTP AUTH?
const bool useAuth = false;

// USE DHT TEMP/HUMIDITY SENSOR? DESIGNATE WHICH PIN. MAKE SURE TO DEFINE WHICH SENSOR MODEL BELOW BY UNCOMMENTING IT.
#define useDHT false
#define DHTPIN D2     // what pin is the DHT on?
#if useDHT==true
  #include <DHT.h>
  // Uncomment whatever type of temperature sensor you're using!
  //#define DHTTYPE DHT11   // DHT 11
  #define DHTTYPE DHT22   // DHT 22  (AM2302)
  //#define DHTTYPE DHT21   // DHT 21 (AM2301)
  DHT dht(DHTPIN, DHTTYPE);
#endif

// OTHER VARIALBES
String currentIP, request, fullrequest;

// LOAD UP NETWORK LIB & PORT
#if useWIFI==true
  #include <ESP8266WiFi.h>
  WiFiServer server(80);
#else
  #include <UIPEthernet.h>
  EthernetServer server = EthernetServer(80);
#endif

void setup()
{
  Serial.begin(115200);

  #if useDHT==true
    dht.begin();
  #endif

  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  pinMode(alarmPin1, OUTPUT);
  pinMode(notificationPin1, OUTPUT);
  digitalWrite(alarmPin1, LOW);
  digitalWrite(relayPin1, use5Vrelay==true ? HIGH : LOW);
  digitalWrite(relayPin2, use5Vrelay==true ? HIGH : LOW);
  digitalWrite(notificationPin1, LOW);
  
  #if useWIFI==true
    // Connect to WiFi network
    Serial.println(); Serial.print("Connecting to "); Serial.println(ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    // Start the server
    server.begin();
    Serial.println("Server started");
    // Print the IP address
    Serial.print("Use this URL to connect: ");
    Serial.print("http://"); Serial.print(WiFi.localIP()); Serial.println("/");
    currentIP=WiFi.localIP().toString();
  #else
    uint8_t mac[6] = {0xFF,0x01,0x02,0x03,0x04,0x05};
    IPAddress myIP(192,168,200,226);
    Ethernet.begin(mac,myIP);
    /* //DHCP NOT WORKING
    if (Ethernet.begin(mac) == 0) {
      while (1) {
        Serial.println("Failed to configure Ethernet using DHCP");
        delay(10000);
      }
    }
    */
    server.begin();
    Serial.print("localIP: "); Serial.println(Ethernet.localIP());
    Serial.print("subnetMask: "); Serial.println(Ethernet.subnetMask());
    Serial.print("gatewayIP: "); Serial.println(Ethernet.gatewayIP());
    Serial.print("dnsServerIP: "); Serial.println(Ethernet.dnsServerIP());
    currentIP=Ethernet.localIP().toString();
  #endif
}

void loop()
{

  // NOTIFICATION LIGHT
  if (millis()%90000==0) { // every 1.5 minutes
    digitalWrite(notificationPin1, HIGH);
    delay(250);
    digitalWrite(notificationPin1, LOW);
    delay(200);
    digitalWrite(notificationPin1, HIGH);
    delay(250);
    digitalWrite(notificationPin1, LOW);
  }

  
  // SERIAL MESSAGE
  if (millis()%900000==0) { // every 15 minutes
    Serial.print("UpTime: "); Serial.println(uptime());
  }

  // REBOOT
  EEPROM.begin(1);
  int days=EEPROM.read(0);
  int RebootFrequencyDays=0;
  RebootFrequencyDays=days;
  if (RebootFrequencyDays > 0 && millis() >= (86400000*RebootFrequencyDays)) { //86400000 per day
    while(true);
  }

  #if useWIFI==true
    // Check if a client has connected
    WiFiClient client = server.available();
    if (!client) {
      return;
    }
   
    // Wait until the client sends some data
    Serial.println("New client");
    while(!client.available()){
      delay(1);
    }
    //Serial.println("---FULL REQUEST---"); Serial.println(client.readString()); Serial.println("---END OF FULL REQUEST---");
  
    request = client.readStringUntil('\r');  // Read the first line of the request
    fullrequest = client.readString();
    Serial.println(request);
    request.replace("GET ", ""); request.replace(" HTTP/1.1", ""); request.replace(" HTTP/1.0", "");
    client.flush();
  #else
    EthernetClient client = server.available();  // try to get client
  
    if (client) {  // got client?
      boolean currentLineIsBlank = true;
      String HTTP_req;          // stores the HTTP request
      while (client.connected()) {
        if (client.available()) {   // client data available to read
          char c = client.read(); // read 1 byte (character) from client
          HTTP_req += c;  // save the HTTP request 1 char at a time
          // last line of client request is blank and ends with \n
          // respond to client only after last line received
          if (c == '\n' && currentLineIsBlank) { //&& currentLineIsBlank --- first line only on low memory like UNO/Nano otherwise get it all for AUTH
            fullrequest=HTTP_req;
            request = HTTP_req.substring(0,HTTP_req.indexOf('\r'));
            //auth = HTTP_req.substring(HTTP_req.indexOf('Authorization: Basic '),HTTP_req.indexOf('\r'));
            HTTP_req = "";    // finished with request, empty string
            request.replace("GET ", ""); request.replace(" HTTP/1.1", ""); request.replace(" HTTP/1.0", "");
  #endif

  Serial.println(request);
  handleRequest();
  delay(10); // pause to make sure pin status is updated for response below
  client.println(clientResponse());

  #if useWIFI==true
    delay(1);
    Serial.println("Client disonnected");
    Serial.println("");
  #else
            break;
          }
          // every line of text received from the client ends with \r\n
          if (c == '\n') {
            // last character on line of received text
            // starting new line with next character read
            currentLineIsBlank = true;
          } else if (c != '\r') {
            // a text character was received from client
            currentLineIsBlank = false;
          }
        } // end if (client.available())
      } // end while (client.connected())
      delay(1);      // give the web browser time to receive the data
      client.stop(); // close the connection
    } // end if (client)
  #endif
} //loop

void handleRequest() {
  //variables for the alarm buzzer
  
  int alarmVar = 0;  // initialize alarmVar for alarm loop
  int alarmRepeat = 6; //Number of warning beeps

  // Match the request
  if (request.indexOf("/favicon.ico") > -1)  {
    return;
  }
  if (request.indexOf("/RebootNow") != -1)  {
    while(true);
  }
  if (request.indexOf("RebootFrequencyDays=") != -1)  {
    EEPROM.begin(1);
    String RebootFrequencyDays=request;
    RebootFrequencyDays.replace("RebootFrequencyDays=","");
    RebootFrequencyDays.replace("/","");
    RebootFrequencyDays.replace("?","");
    //for (int i = 0 ; i < 512 ; i++) { EEPROM.write(i, 0); } // fully clear EEPROM before overwrite
    EEPROM.write(0,atoi(RebootFrequencyDays.c_str()));
    EEPROM.commit();
  }


  //Serial.print("use5Vrelay == "); Serial.println(use5Vrelay);
  if (request.indexOf("RELAY1=ON") != -1 || request.indexOf("MainTriggerOn=") != -1)  {
    digitalWrite(relayPin1, use5Vrelay==true ? LOW : HIGH);
  }
  if (request.indexOf("RELAY1=OFF") != -1 || request.indexOf("MainTriggerOff=") != -1)  {
    digitalWrite(relayPin1, use5Vrelay==true ? HIGH : LOW);
  }
  if (request.indexOf("RELAY1=MOMENTARY") != -1 || request.indexOf("MainTrigger=") != -1)  {
    digitalWrite(alarmPin1, HIGH);
    delay(1000);
    digitalWrite(alarmPin1, LOW);
    alarmVar = 0;
    while(alarmVar < alarmRepeat){
      // play alarm
      alarmVar++;
      digitalWrite(alarmPin1, HIGH);
      delay(500);
      digitalWrite(alarmPin1, LOW);
      delay(500);
    }
    alarmVar = 0;
    while(alarmVar < alarmRepeat){
      // play alarm
      alarmVar++;
      digitalWrite(alarmPin1, HIGH);
      delay(100);
      digitalWrite(alarmPin1, LOW);
      delay(100);
    }    
    digitalWrite(relayPin1, use5Vrelay==true ? LOW : HIGH);
    delay(300);
    digitalWrite(relayPin1, use5Vrelay==true ? HIGH : LOW);
  }
  
  if (request.indexOf("RELAY2=ON") != -1 || request.indexOf("CustomTriggerOn=") != -1)  {
    digitalWrite(relayPin2, use5Vrelay==true ? LOW : HIGH);
  }
  if (request.indexOf("RELAY2=OFF") != -1 || request.indexOf("CustomTriggerOff=") != -1)  {
    digitalWrite(relayPin2, use5Vrelay==true ? HIGH : LOW);
  }
  if (request.indexOf("RELAY2=MOMENTARY") != -1 || request.indexOf("CustomTrigger=") != -1)  {
    digitalWrite(alarmPin1, HIGH);
    delay(1000);
    digitalWrite(alarmPin1, LOW);
    alarmVar = 0;
    while(alarmVar < alarmRepeat){
      // play alarm
      alarmVar++;
      digitalWrite(alarmPin1, HIGH);
      delay(500);
      digitalWrite(alarmPin1, LOW);
      delay(500);
    }
    alarmVar = 0;
    while(alarmVar < alarmRepeat){
      // play alarm
      alarmVar++;
      digitalWrite(alarmPin1, HIGH);
      delay(100);
      digitalWrite(alarmPin1, LOW);
      delay(100);
    }    
    digitalWrite(relayPin2, use5Vrelay==true ? LOW : HIGH);
    delay(300);
    digitalWrite(relayPin2, use5Vrelay==true ? HIGH : LOW);
  }
}

String clientResponse () {
  String clientResponse;
  // BASIC AUTHENTICATION
  if (useAuth==true)  {
  // The below Base64 string is gate:gate1 for the username:password
    if (fullrequest.indexOf("Authorization: Basic Z2F0ZTpnYXRlMQ==") == -1)  {
      clientResponse.concat("HTTP/1.1 401 Access Denied\n");
      clientResponse.concat("WWW-Authenticate: Basic realm=\"ESP8266\"\n");
      clientResponse.concat("Content-Type: text/html\n");
      clientResponse.concat("\n"); //  do not forget this one
      clientResponse.concat("Failed : Authentication Required!\n");
      return clientResponse;
    }
  }

  // Return the response
  clientResponse.concat("HTTP/1.1 200 OK\n");
  clientResponse.concat("Content-Type: text/html\n");
  clientResponse.concat("\n"); //  do not forget this one
  clientResponse.concat("<!DOCTYPE HTML>\n");
  clientResponse.concat("<html><head><title>ESP8266 & ");
  #if useWIFI==true
    clientResponse.concat("WIFI");
  #else
    clientResponse.concat("ENC28J60");
  #endif
  clientResponse.concat(" DUAL SWITCH</title></head><meta name=viewport content='width=500'>\n<style type='text/css'>\nbutton {line-height: 1.8em; margin: 5px; padding: 3px 7px;}");
  clientResponse.concat("\nbody {text-align:center;}\ndiv {border:solid 1px; margin: 3px; width:150px;}\n.center { margin: auto; width: 400px; border: 3px solid #73AD21; padding: 3px;");
  clientResponse.concat("</style></head>\n<h2><a href='/'>ESP8266 & ");
  #if useWIFI==true
    clientResponse.concat("WIFI");
  #else
    clientResponse.concat("ENC28J60");
  #endif
  clientResponse.concat(" DUAL SWITCH</h2><h3>");
  clientResponse.concat(currentIP);
  clientResponse.concat("</h3>\n</a>\n");

  clientResponse.concat("<i>Current Request:</i><br><b>\n");
  clientResponse.concat(request);
  clientResponse.concat("\n</b><hr>");

  clientResponse.concat("<pre>\n");
  clientResponse.concat("UpTime="); clientResponse.concat(uptime()); clientResponse.concat("\n");
  clientResponse.concat(freeRam());
  clientResponse.concat("\n");
  // HANDLE DHT
  #if useDHT==true
    // Reading temperature or humidity takes about 250 milliseconds! Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    float tc = dht.readTemperature();
    // SENSOR RETRY LOGIC
    int counter=1;
    while((isnan(tc) || isnan(h)) || counter>5){
        h = dht.readHumidity();
        tc = dht.readTemperature();
        counter += 1;
    }
    float tf = (tc * 9.0 / 5.0) + 32.0;
    // check if returns are valid, if they are NaN (not a number) then something went wrong!
    if (isnan(tc) || isnan(h)) {
      Serial.println("Failed to read from DHT");
      clientResponse.concat("DHT Sensor Failed\n");
    } else {
      #if DHTTYPE==DHT11
        clientResponse.concat("<b><i>DHT11 Sensor Information:</i></b>\n");
      #elif DHTTYPE==DHT22
        clientResponse.concat("<b><i>DHT22 Sensor Information:</i></b>\n");
      #elif DHTTYPE==DHT21
        clientResponse.concat("<b><i>DHT21 Sensor Information:</i></b>\n");
      #endif
      clientResponse.concat("Temperature="); clientResponse.concat(String(tc,1)); clientResponse.concat((char)176); clientResponse.concat("C "); clientResponse.concat(round(tf)); clientResponse.concat((char)176); clientResponse.concat("F\n");
      clientResponse.concat("Humidity="); clientResponse.concat(round(h)); clientResponse.concat("%\n");
    }
  #else
    clientResponse.concat("DHT Sensor Not Used\n");
  #endif
  clientResponse.concat("</pre>\n"); clientResponse.concat("<hr>\n");

  clientResponse.concat("<div class='center'>\n");
  clientResponse.concat("RELAY1 pin is now: ");
  if(use5Vrelay==true) {
    if(digitalRead(relayPin1) == LOW) { clientResponse.concat("On"); } else { clientResponse.concat("Off"); }
  } else {
    if(digitalRead(relayPin1) == HIGH) { clientResponse.concat("On"); } else { clientResponse.concat("Off"); }
  }
  clientResponse.concat("\n<br><a href=\"/RELAY1=ON\"><button onClick=\"parent.location='/RELAY1=ON'\">Turn On</button></a>\n");
  clientResponse.concat("<a href=\"/RELAY1=OFF\"><button onClick=\"parent.location='/RELAY1=OFF'\">Turn Off</button></a>\n");
  clientResponse.concat("<a href=\"/RELAY1=MOMENTARY\"><button onClick=\"parent.location='/RELAY1=MOMENTARY'\">MOMENTARY</button></a><br/></div><hr>\n");
  
  clientResponse.concat("<div class='center'>\n");
  clientResponse.concat("RELAY2 pin is now: ");
  if(use5Vrelay==true) {
    if(digitalRead(relayPin2) == LOW) { clientResponse.concat("On"); } else { clientResponse.concat("Off"); }
  } else {
    if(digitalRead(relayPin2) == HIGH) { clientResponse.concat("On"); } else { clientResponse.concat("Off"); }
  }
  clientResponse.concat("\n<br><a href=\"/RELAY2=ON\"><button onClick=\"parent.location='/RELAY2=ON'\">Turn On</button></a>\n");
  clientResponse.concat("<a href=\"/RELAY2=OFF\"><button onClick=\"parent.location='/RELAY2=OFF'\">Turn Off</button></a>\n");
  clientResponse.concat("<a href=\"/RELAY2=MOMENTARY\"><button onClick=\"parent.location='/RELAY2=MOMENTARY'\">MOMENTARY</button></a></div><hr>\n");
  
  clientResponse.concat("<div class='center'><input id=\"RebootFrequencyDays\" type=\"text\" name=\"RebootFrequencyDays\" value=\"");
  EEPROM.begin(1);
  int days=EEPROM.read(0);
  clientResponse.concat(days);
  clientResponse.concat("\" maxlength=\"3\" size=\"2\" min=\"0\" max=\"255\">&nbsp;&nbsp;&nbsp;<button style=\"line-height: 1em; margin: 3px; padding: 3px 3px;\" onClick=\"parent.location='/RebootFrequencyDays='+document.getElementById('RebootFrequencyDays').value;\">SAVE</button><br>Days between reboots.<br>0 to disable & 255 days is max.");
  clientResponse.concat("<br><button onClick=\"javascript: if (confirm(\'Are you sure you want to reboot?\')) parent.location='/RebootNow';\">Reboot Now</button><br></div><hr>\n");

  clientResponse.concat("<div class='center'><a target='_blank' href='https://community.smartthings.com/t/raspberry-pi-to-php-to-gpio-to-relay-to-gate-garage-trigger/43335'>Project on SmartThings Community</a></br>\n");
  clientResponse.concat("<a target='_blank' href='https://github.com/JZ-SmartThings/SmartThings/tree/master/Devices/Generic%20HTTP%20Device'>Project on GitHub</a></br></div></html>\n");
  return clientResponse;
}

String freeRam () {
  #if defined(ARDUINO_ARCH_AVR)
    extern int __heap_start, *__brkval;
    int v;
    return "Free Mem="+String((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval))+"B of 2048B";
  #elif defined(ESP8266)
    return "Free Mem="+String(ESP.getFreeHeap()/1024)+"KB of 80KB";
  #endif
}

String uptime() {
  float d,hr,m,s;
  String dstr,hrstr, mstr, sstr;
  unsigned long over;
  d=int(millis()/(3600000*24));
  dstr=String(d,0);
  dstr.replace(" ", "");
  over=millis()%(3600000*24);
  hr=int(over/3600000);
  hrstr=String(hr,0);
  if (hr<10) {hrstr=hrstr="0"+hrstr;}
  hrstr.replace(" ", "");
  over=over%3600000;
  m=int(over/60000);
  mstr=String(m,0);
  if (m<10) {mstr=mstr="0"+mstr;}
  mstr.replace(" ", "");
  over=over%60000;
  s=int(over/1000);
  sstr=String(s,0);
  if (s<10) {sstr="0"+sstr;}
  sstr.replace(" ", "");
  if (dstr=="0") {
    return hrstr + ":" + mstr + ":" + sstr;
  } else if (dstr=="1") {
    return dstr + " Day " + hrstr + ":" + mstr + ":" + sstr;
  } else {
    return dstr + " Days " + hrstr + ":" + mstr + ":" + sstr;
  }
}

