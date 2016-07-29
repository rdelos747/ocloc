#include <Adafruit_DotStar.h>
#include <SPI.h> 
#include <Wire.h>
#include "Adafruit_BLE_UART.h"
#include <EEPROM.h>
//=============================
// G L O B A L   V A R S
//======================================================
//LED
#define DS3231_I2C_ADDRESS 0x68
#define NUMPIXELS 120 
#define DATAPIN    4 //YELLOW
#define CLOCKPIN   5 //GREEN
//BLUETOOTH
// Connect CLK/MISO/MOSI to hardware SPI
// e.g. On UNO & compatible: CLK = 13, MISO = 12, MOSI = 11
#define ADAFRUITBLE_REQ 10
#define ADAFRUITBLE_RDY 2     // This should be an interrupt pin, on Uno thats #2 or #3
#define ADAFRUITBLE_RST 9

Adafruit_DotStar strip = Adafruit_DotStar(
  NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);

Adafruit_BLE_UART BTLEserial = Adafruit_BLE_UART(ADAFRUITBLE_REQ, ADAFRUITBLE_RDY, ADAFRUITBLE_RST);

int CLOCK_STATE           = 0;
int CLOCK_SET             = 255;
int RESET                 = 1;

//=============================
// I N I T
//======================================================
void setup() {
  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
    clock_prescale_set(clock_div_1);                              // Enable 16 MHz on Trinket
  #endif
  strip.begin();                                                  // Initialize pins for output
  strip.show();                                                   // Turn all LEDs off ASAP
  Wire.begin();
  pinMode(13, OUTPUT);                                            // Sanity check LED
  randomSeed(analogRead(0));
  Serial.begin(9600);
  while(!Serial); // Leonardo/Micro should wait for serial init

  BTLEserial.setDeviceName("OCLOC"); /* 7 characters max! */

  BTLEserial.begin();

  if (RESET == 1) {
    EEPROM.write(0, 255);
  }

  int epAddr = 0;
  Serial.println(CLOCK_SET);
  Serial.println(EEPROM.read(epAddr));
  CLOCK_SET = EEPROM.read(epAddr);
  Serial.println(CLOCK_SET);
  if (CLOCK_SET == 0) {
    CLOCK_STATE = 3;
  }
}

//=============================
// M A I N   L O O P
//======================================================
aci_evt_opcode_t laststatus = ACI_EVT_DISCONNECTED;
void loop() {
  /* STEP 1:
   * Find status of BLE
   */
  BTLEserial.pollACI();
  aci_evt_opcode_t status = BTLEserial.getState();                                  

  /* STEP 2 A:
   * If clock not set <EEPROM(0) == 255>
   * Coordinate first time pairing structure
   */
  if (CLOCK_SET == 255) {
    
    if (status != laststatus) {
      if (status == ACI_EVT_DEVICE_STARTED) {
         Serial.println(F("* Advertising started"));
         clearClock();
         CLOCK_STATE = 1;
      }
      if (status == ACI_EVT_CONNECTED) {
         Serial.println(F("* Connected!"));    
         clearClock();
         CLOCK_STATE = 2;
         CLOCK_SET = 0;
         EEPROM.write(0, 0);
      }
      if (status == ACI_EVT_DISCONNECTED) {
          Serial.println(F("* Disconnected or advertising timed out"));
          clearClock();
          CLOCK_STATE = 0;
      }
      laststatus = status;
    }
  /* STEP 2 B:
   * If clock is set <EEPROM(0) == 0>
   * Display normal clock based on RTC
   * Run advertising in background
   */  
  } else {
    if (status != laststatus) {
      if (status == ACI_EVT_DEVICE_STARTED) {Serial.println(F("* Advertising started"));}
      if (status == ACI_EVT_CONNECTED)      {Serial.println(F("* Connected!"));clearClock();CLOCK_STATE = 2;}
      if (status == ACI_EVT_DISCONNECTED)   {Serial.println(F("* Disconnected or advertising timed out"));}
      laststatus = status;
    }
    if (status == ACI_EVT_CONNECTED) {getNewTime();}
  }

  /* STEP 3:
   * Handle display states of clock/led
   * 0 = startup/ not connected
   * 1 = advertising/ searching for ble
   * 2 = found device, show quick flash, then jump to state 3
   * 3 = normal clock
   */
   //Serial.println(CLOCK_SET);
   switch (CLOCK_STATE) {
    case 0:
      clockStateZero();
      break;
    case 1:
      clockStateOne();
      break;
    case 2:
      clockStateTwo();
      break;
    case 3:
      runClock();
      break;
   }
}
