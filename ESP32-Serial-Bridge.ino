// ESP32 WiFi <-> 3x UART Bridge
// by AlphaLima
// www.LK8000.com

// Disclaimer: Don't use  for life support systems
// or any other situations where system failure may affect
// user or environmental safety.

#include "config.h"
#include <esp_wifi.h>
#include <WiFi.h>
#include <WebServer.h>

WebServer wserver(80);
#ifdef BLUETOOTH
#include <BluetoothSerial.h>
BluetoothSerial SerialBT; 
#endif

#ifdef OTA_HANDLER  
#include <ArduinoOTA.h> 

#endif // OTA_HANDLER

String clientIP = "192.168.1.180";
String hostname = "ESP32toRS232";
const char* clientSSID = "your network";

HardwareSerial Serial_one(1);
HardwareSerial Serial_two(2);
HardwareSerial* COM[NUM_COM] = {&Serial, &Serial_one , &Serial_two};

#define MAX_NMEA_CLIENTS 4
#ifdef PROTOCOL_TCP
#include <WiFiClient.h>
WiFiServer server_0(SERIAL0_TCP_PORT);
WiFiServer server_1(SERIAL1_TCP_PORT);
WiFiServer server_2(SERIAL2_TCP_PORT);
WiFiServer *server[NUM_COM]={&server_0,&server_1,&server_2};
WiFiClient TCPClient[NUM_COM][MAX_NMEA_CLIENTS];
#endif


uint8_t buf1[NUM_COM][bufferSize];
uint16_t i1[NUM_COM]={0,0,0};

uint8_t buf2[NUM_COM][bufferSize];
uint16_t i2[NUM_COM]={0,0,0};

uint8_t BTbuf[bufferSize];
uint16_t iBT =0;




void setup() {


  delay(500);
  
  COM[0]->begin(UART_BAUD0, SERIAL_PARAM0, SERIAL0_RXPIN, SERIAL0_TXPIN);
  COM[1]->begin(UART_BAUD1, SERIAL_PARAM1, SERIAL1_RXPIN, SERIAL1_TXPIN);
  COM[2]->begin(UART_BAUD2, SERIAL_PARAM2, SERIAL2_RXPIN, SERIAL2_TXPIN);
  
  if(debug) COM[DEBUG_COM]->println("\n\nLK8000 WiFi serial bridge V1.00");
  #ifdef MODE_AP 
   if(debug) COM[DEBUG_COM]->println("Open ESP Access Point mode");
  //AP mode (phone connects directly to ESP) (no router)
  WiFi.setHostname(hostname.c_str());
  WiFi.mode(WIFI_AP);
   
  WiFi.softAP(ssid, pw); // configure ssid and password for softAP
  delay(2000); // VERY IMPORTANT
  WiFi.softAPConfig(ip, ip, netmask); // configure ip address for softAP
 

  #endif


  #ifdef MODE_STA
   if(debug) COM[DEBUG_COM]->println("Open ESP Station mode");
  // STATION mode (ESP connects to router and gets an IP)
  // Assuming phone is also connected to that router
  // from RoboRemo you must connect to the IP of the ESP
  WiFi.setHostname(hostname.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pw);
  if(debug) COM[DEBUG_COM]->print("try to Connect to Wireless network: ");
  if(debug) COM[DEBUG_COM]->println(ssid);
  while (WiFi.status() != WL_CONNECTED) {   
    delay(500);
    if(debug) COM[DEBUG_COM]->print(".");
  }
  if(debug) COM[DEBUG_COM]->println("\nWiFi connected");
  
  wserver.on("/", handle_root);             // ###############
  wserver.begin();


  #endif
#ifdef BLUETOOTH
  if(debug) COM[DEBUG_COM]->println("Open Bluetooth Server");  
  SerialBT.begin(ssid); //Bluetooth device name
 #endif
#ifdef OTA_HANDLER  
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request

  ArduinoOTA.begin();
#endif // OTA_HANDLER    

  #ifdef PROTOCOL_TCP
  COM[0]->println("Starting TCP Server 1");  
  if(debug) COM[DEBUG_COM]->println("Starting TCP Server 1");  
  server[0]->begin(); // start TCP server 
  server[0]->setNoDelay(true);
  COM[1]->println("Starting TCP Server 2");
  if(debug) COM[DEBUG_COM]->println("Starting TCP Server 2");  
  server[1]->begin(); // start TCP server 
  server[1]->setNoDelay(true);
  COM[2]->println("Starting TCP Server 3");
  if(debug) COM[DEBUG_COM]->println("Starting TCP Server 3");  
  server[2]->begin(); // start TCP server   
  server[2]->setNoDelay(true);

 
   #endif

  esp_err_t esp_wifi_set_max_tx_power(50);  //lower WiFi Power
}

void handle_root() {
 clientIP=WiFi.localIP().toString();
 clientSSID=ssid;
  String output = "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><link rel=\"icon\" href=\"data:,\">";
  output += "<style>body { text-align: center; font-family: \"Trebuchet MS\", Arial;}";
  output += "table { border-collapse: collapse; width:35%; margin-left:auto; margin-right:auto; }";
  output += "th { padding: 12px; background-color: #0043af; color: white; }tr { border: 1px solid #ddd; padding: 12px; }tr:hover { background-color: #bcbcbc; }";
  output += "td { border: none; padding: 12px; }.sensor { color:white; font-weight: bold; background-color: #bcbcbc; padding: 1px; }";
  output += "</style></head><body><h1>RS232 to WiFi Converter ESP32</h1>";
  output += "<table><tr><th>Konfiguration:</th><th>";
  output += "<tr><td>IP Adresse</td><td><span class=\"sensor\">";
  output += String(clientIP)+"</span></td></tr>";
  output += "<tr><td>Verbunden mit</td><td><span class=\"sensor\">";
  output += String(clientSSID);
  output +="</span></td></tr>";
  output +="<tr><td>COM0: TCP Port:</td><td><span class=\"sensor\">";
  output += String(SERIAL0_TCP_PORT);
  output += " ; "+String(UART_BAUD0)+" Baud";
  output += " ;  RX Pin: "+String(SERIAL0_RXPIN)+" , TX Pin: "+String(SERIAL0_TXPIN);
  output +="</span></td></tr>";
  output +="<tr><td>COM1: TCP Port:</td><td><span class=\"sensor\">";
  output += String(SERIAL1_TCP_PORT);
  output += " ; "+String(UART_BAUD1)+" Baud";
  output += " ;  RX Pin: "+String(SERIAL1_RXPIN)+" , TX Pin: "+String(SERIAL1_TXPIN);
  output +="</span></td></tr>";
  output +="<tr><td>COM2: TCP Port:</td><td><span class=\"sensor\">";
  output += String(SERIAL2_TCP_PORT);
  output += " ; "+String(UART_BAUD2)+" Baud";
  output += " ;  RX Pin: "+String(SERIAL2_RXPIN)+" , TX Pin: "+String(SERIAL2_TXPIN);
  output +="</span></td></tr>";
  
  
  
  wserver.send(200, "text/html", output);
 
}

void loop() 
{  

wserver.handleClient();      // ################

#ifdef OTA_HANDLER  
  ArduinoOTA.handle();
#endif // OTA_HANDLER
  
#ifdef BLUETOOTH
  // receive from Bluetooth:
  if(SerialBT.hasClient()) 
  {
    while(SerialBT.available())
    {
      BTbuf[iBT] = SerialBT.read(); // read char from client (LK8000 app)
      if(iBT <bufferSize-1) iBT++;
    }          
    for(int num= 0; num < NUM_COM ; num++)
      COM[num]->write(BTbuf,iBT); // now send to UART(num):          
    iBT = 0;
  }  
#endif  
#ifdef PROTOCOL_TCP
  for(int num= 0; num < NUM_COM ; num++)
  {
    if (server[num]->hasClient())
    {
      for(byte i = 0; i < MAX_NMEA_CLIENTS; i++){
        //find free/disconnected spot
        if (!TCPClient[num][i] || !TCPClient[num][i].connected()){
          if(TCPClient[num][i]) TCPClient[num][i].stop();
          TCPClient[num][i] = server[num]->available();
          if(debug) COM[DEBUG_COM]->print("New client for COM"); 
          if(debug) COM[DEBUG_COM]->print(num); 
          if(debug) COM[DEBUG_COM]->println(i);
          continue;
        }
      }

      
      //no free/disconnected spot so reject
      WiFiClient TmpserverClient = server[num]->available();
      TmpserverClient.stop();
    }
  }
#endif
 
  for(int num= 0; num < NUM_COM ; num++)
  {
    if(COM[num] != NULL)          
    {
      for(byte cln = 0; cln < MAX_NMEA_CLIENTS; cln++)
      {               
        if(TCPClient[num][cln]) 
        {
          while(TCPClient[num][cln].available())
          {
            buf1[num][i1[num]] = TCPClient[num][cln].read(); // read char from client (LK8000 app)
            if(i1[num]<bufferSize-1) i1[num]++;
          } 

          COM[num]->write(buf1[num], i1[num]); // now send to UART(num):
          i1[num] = 0;
        }
      }
  
      if(COM[num]->available())
      {
        while(COM[num]->available())
        {     
          buf2[num][i2[num]] = COM[num]->read(); // read char from UART(num)
          if(i2[num]<bufferSize-1) i2[num]++;
        }
        // now send to WiFi:
        for(byte cln = 0; cln < MAX_NMEA_CLIENTS; cln++)
        {   
          if(TCPClient[num][cln])                     
            TCPClient[num][cln].write(buf2[num], i2[num]);
        }
#ifdef BLUETOOTH        
        // now send to Bluetooth:
        if(SerialBT.hasClient())      
          SerialBT.write(buf2[num], i2[num]);               
#endif  
        i2[num] = 0;
      }
    }    
  }
}
