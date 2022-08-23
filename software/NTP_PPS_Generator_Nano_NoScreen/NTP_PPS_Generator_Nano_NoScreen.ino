/**
 * NTP_PPP_GENERATOR For Nano and Ethernet
 * 
 * 29 June 2022
 * 
 * This program connect the NTP-PPS board to a time server, and continually sends out PPS and ZDA pulses every second.
 * 
 * Pulse comes out on pin 13 on the Nano board. You need to update the ethernet library folowing the instructions TimerInterrupt_Generic library due ot timing differences.
 * 
 */
// Select the timers you're using, here ITimer1
#define USE_TIMER_1     true
#define USE_TIMER_2     false


#include <Arduino.h>
#include <string.h>
#include <Vector.h>
#include <SPI.h>
#include <Ethernet_Generic.h>
#include <Time.h>
#include "TimerInterrupt_Generic.h"
#include <NTPClient_Generic.h>
#include <TimeLib.h>

const char* ntpServer = "192.168.42.18";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;

const int PPS_OUT_PIN = 2; 
const int PPS_OUT_LED = LED_BUILTIN;

// the media access control (ethernet hardware) address for the shield:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  
//the IP address for the shield:
byte ip[] = { 192, 168, 42, 25 };    
// the router's gateway address:
byte gateway[] = { 192, 168, 42, 1 };
// the subnet:
byte subnet[] = { 255, 255, 255, 0 };

const int TCP_PORT = 6000;

volatile bool second_changed;
volatile bool enable;
char ip_address [16];



EthernetServer server (TCP_PORT);
const int ELEMENT_COUNT_MAX = 10;
EthernetClient storage_array[ELEMENT_COUNT_MAX];
Vector <EthernetClient> tcp_clients(storage_array);


EthernetUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer);


//
// Generate ZDA and RMC Strings
//
String generateGPS() {
  time_t rawtime = timeClient.getEpochTime();

  String zda_string;

  //ZDA setup
  char buf[150];
  sprintf(buf, "$GPZDA,%02d%02d%02d.00,%02d,%02d,%04d,00,00*", 
                  hour(rawtime), 
                  minute(rawtime), 
                  second(rawtime),
                  day(rawtime),
                  month(rawtime),
                  year(rawtime));
  zda_string = String(buf);
  size_t zda_length (zda_string.length());

  uint8_t checksum = 0;
  for (auto i = 0; i<zda_length; ++i) {
    if (buf[i] == '$' || buf[i] == 'I' || buf[i] == '*') {
      continue;
    }
    checksum = checksum ^ buf[i];
  }

  char checkbuf[3];
  sprintf(checkbuf, "%02X", checksum);

  zda_string += checkbuf;
  zda_string += "\r\n";


  //RMC setup
  String rmc_string;

  sprintf(buf, "$GPRMC,%02d%02d%02d.00,A,0000.01,N,00000.01,W,0.0,0.0,%02d,%02d,%04d,0.0,W*", 
                  hour(rawtime), 
                  minute(rawtime), 
                  second(rawtime),
                  day(rawtime),
                  month(rawtime),
                  year(rawtime));
  rmc_string = String(buf);
  size_t rmc_length (rmc_string.length());

  checksum = 0;
  for (auto i = 0; i<rmc_length; ++i) {
    if (buf[i] == '$' || buf[i] == 'I' || buf[i] == '*') {
      continue;
    }
    checksum = checksum ^ buf[i];
  }
  
  sprintf(checkbuf, "%02X", checksum);
  
  rmc_string += checkbuf;
  rmc_string += "\r\n"; 

  String gps_string;
  gps_string = zda_string + rmc_string;
  
  return gps_string;
}

//
// Method for updating the display.
//
void printLocalTime()
{
  String gps_string = generateGPS();
  Serial.print((gps_string).c_str());
  for (auto i = 0; i < tcp_clients.size(); ++i) {
    if (tcp_clients[i].connected()) {
      tcp_clients[i].print(gps_string.c_str());
    } else {
      tcp_clients[i].stop();
      tcp_clients.remove(i);
      --i;
    }
  }
}

#define TIMER_INTERVAL_MS        1000L
//
// Interrupt handler for the 1s interrupt.
//
void onTimer() {
  if(enable){     
    digitalWrite(PPS_OUT_PIN, HIGH);
    digitalWrite(PPS_OUT_LED, HIGH);
  }
  second_changed = true;
}

void setup() {
  enable = true;
  Serial.begin(9600);
  pinMode(PPS_OUT_PIN, OUTPUT);
  pinMode(PPS_OUT_LED, OUTPUT);

  /// initialize the ethernet device
  Ethernet.begin(mac, ip, gateway, gateway, subnet);
  
  Serial.println(" CONNECTED");
  sprintf(ip_address, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3] );
  Serial.print(ip_address);

  timeClient.begin();
  timeClient.update();

  // Init timer ITimer1
  ITimer1.init();
  // Interval in unsigned long millisecs
  if (ITimer1.attachInterruptInterval(TIMER_INTERVAL_MS, onTimer)) {
    Serial.println("Starting  ITimer OK, millis() = " + String(millis()));
  } else {
    Serial.println("Can't set ITimer. Select another freq. or timer");
  }
  
  delay(2000);

  server.begin();
  
}

void(* resetFunc) (void) = 0;

void loop() {
  // put your main code here, to run repeatedly:
  if (second_changed) {
    second_changed = false;
    if(enable){ 
      printLocalTime();
      delay(100);
      digitalWrite(PPS_OUT_PIN, LOW);
      digitalWrite(PPS_OUT_LED, LOW);
    }

  }

  EthernetClient client = server.accept();
  if (client) {
    tcp_clients.push_back(client);
  }


  for (auto i = 0; i < tcp_clients.size(); ++i){
    if(tcp_clients[i].available()){
      char c;
      String cmd = "";
      do {
        c = tcp_clients[i].read();
        if(c != '\n' && c != '\r' && c != -1){
          cmd += c;
        }
      }while(c != '\n');
      Serial.print("Data: ");
      Serial.println(cmd);
      if(cmd == "stop"){
        digitalWrite(PPS_OUT_PIN, LOW);
        digitalWrite(PPS_OUT_LED, LOW);
        pinMode(PPS_OUT_PIN, INPUT);
        enable = false;
      } else if (cmd == "start"){
        pinMode(PPS_OUT_PIN, OUTPUT);
        enable = true;
      } else if (cmd == "reset"){
        resetFunc();
      }
      
    }
  }
   
  delay(1);
  timeClient.update();
  
}
