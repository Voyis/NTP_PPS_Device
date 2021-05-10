/**
 * NTP_PPP_GENERATOR
 * 
 * 9 May 2020
 * 
 * This program connect the NTP-PPS board to a time server, and continually sends out PPS and ZDA pulses every second.
 * 
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


const int PPS_OUT_PIN = 17; //actually gpio 17, but I'm using the DOIT definitions.
const int PPS_OUT_LED = 16;

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16

static const unsigned char PROGMEM logo_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

void setup() {
  Serial.begin(115200);
  pinMode(PPS_OUT_PIN, OUTPUT);
  pinMode(PPS_OUT_LED, OUTPUT);

  //Set up screen.
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  // Clear the buffer
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor (0, 0);
  display.println ("VOYIS NTP PPS");
  display.println ("Generator V1.0");
  display.display ();
  delay (2000);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(PPS_OUT_PIN, HIGH);
  digitalWrite(PPS_OUT_LED, HIGH);
  delay(500);
  digitalWrite(PPS_OUT_PIN, LOW);
  digitalWrite(PPS_OUT_LED, LOW);
  delay(500);
}
