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
/*
 * 0 = startup/ not connected
 * 1 = advertising/ searching for ble
 * 2 = found device, start time at 0:0:0, user can send current time now
 */

uint32_t color            = 0x999999;
int hours                 = -1; //ensure the led update always happens by setting -1
int minutes               = -1; //ensure the led update always happens by setting -1
int seconds               = -1; //ensure the led update always happens by setting -1

int hourColor             = 50;
int minuteColor           = 20;
int secondColor           = 10;

int activeLength          = 120;
int minuteLen             = 8;
int hourLen               = 2;

int secLength             = -30;
int secJump               = 5;
int delayTime             = 1;

//STATE 1 VARS
int maxDelay = 5;
int currentDelay = 0;
int sColor = 0;
//int points[120];

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
  for (int i = 0; i < 120; i++) {
    //points[i] = 0;  
  }
  Serial.begin(9600);
  while(!Serial); // Leonardo/Micro should wait for serial init

  BTLEserial.setDeviceName("OCLOC"); /* 7 characters max! */

  BTLEserial.begin();

  int epAddr = 0;
  Serial.println(CLOCK_SET);
  Serial.println(EEPROM.read(epAddr));
  CLOCK_SET = EEPROM.read(epAddr);
  Serial.println(CLOCK_SET);
  if (CLOCK_SET == 0) {
    CLOCK_STATE = 2;
  }
}

//=============================
// M A I N   L O O P
//======================================================
aci_evt_opcode_t laststatus = ACI_EVT_DISCONNECTED;

bool pulse = false;
int secStart = 0;
int secEnd = secLength;
int minutePoint = 0;
void loop() {

  BTLEserial.pollACI();
  aci_evt_opcode_t status = BTLEserial.getState();                                  // Ask what is our current status
  if (status != laststatus) {                                                       // If the status changed.... print it out!
    if (status == ACI_EVT_DEVICE_STARTED) {
        Serial.println(F("* Advertising started"));                                 // Advertising started
        if (CLOCK_SET == 255) {                                                     // If clock hasn't been set yet
          clearClock();
          CLOCK_STATE = 1;                                                          // Set clock state to 1
        }
    }
    if (status == ACI_EVT_CONNECTED) {
        Serial.println(F("* Connected!"));                                          // Device found    
        if (CLOCK_SET == 255) {                                                     // If clock hasn't been set yet
          clearClock();
          //setDS3231time(0, 0, 0, 0, 0, 0, 0);                                       // Set RTC Time to 0:0:0 for initial
          CLOCK_STATE = 2;                                                          // Set clock state to 2
          CLOCK_SET = 0;                                                            // Clock is now set, and will stay in state 2 until power off
          EEPROM.write(0, 0);
        }
    }
    if (status == ACI_EVT_DISCONNECTED) {
        Serial.println(F("* Disconnected or advertising timed out"));               // Device disconnected
        if (CLOCK_SET == 255) {                                                     // If clock hasn't been set yet
          clearClock();
          CLOCK_STATE = 0;                                                          // Set clock state to 0
        }
    }
    laststatus = status;                                                            // OK set the last status change to this one
  }
  if (status == ACI_EVT_CONNECTED) {
    getNewTime();                                                                   // See if there is new time from user
  }
  
  /*
   * Clock states correspond to visual state of clock,
   * they do not describe the ble state.
   */
  if (CLOCK_STATE == 0) {
    clockStateZero();
  } else if (CLOCK_STATE == 1) {
    clockStateOne();
  } else if (CLOCK_STATE == 2) {
    runClock();
  }
}

//=============================
// R T C
//======================================================
// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}

void setDS3231time(byte newSecond, byte newMinute, byte newHour, byte dayOfWeek, byte
dayOfMonth, byte newMonth, byte newYear)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(newSecond)); // set seconds
  Wire.write(decToBcd(newMinute)); // set minutes
  Wire.write(decToBcd(newHour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(newMonth)); // set month
  Wire.write(decToBcd(newYear)); // set year (0 to 99)
  Wire.endTransmission();
}
void readDS3231time(byte *newSecond,
byte *newMinute,
byte *newHour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *newMonth,
byte *newYear)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *newSecond = bcdToDec(Wire.read() & 0x7f);
  *newMinute = bcdToDec(Wire.read());
  *newHour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *newMonth = bcdToDec(Wire.read());
  *newYear = bcdToDec(Wire.read());
}

//=============================
// C L O C K   F U N C T I O N S
//======================================================
void printTime() {
  Serial.print(hours);
  Serial.print(" ");
  Serial.print(minutes);
  Serial.print(" ");
  Serial.print(seconds);
  Serial.println("");
}

void clearClock() {
  for (int i = 0; i < activeLength; i++) {
    strip.setPixelColor(i, 0, 0, 0);
  }
  strip.show();
}

void clockStateZero() {
  strip.setPixelColor(119, 100, 100, 100);
  strip.setPixelColor(0, 100, 100, 100);
  strip.show();
}
int colorDir = 0;
void clockStateOne() {
  /*
  for (int i = 0; i < 12; i++) {
    strip.setPixelColor(i * 10, 100, 100, 100);
  }
  */
  //Serial.println(currentDelay);
  if (currentDelay > 0) {
    currentDelay--;
  } else {
    currentDelay = maxDelay;
    if (colorDir == 0) {
      if (sColor < 100) {
        sColor += 5;
      } else {
        colorDir = 1;
      }
    } else {
      if (sColor > 0) {
        sColor -= 5;
      } else {
        colorDir = 0;
      }
    }
    //Serial.println(sColor);
    for (int i = 0; i < 12; i++) {
      strip.setPixelColor(i * 10, sColor);
    }
  }
  strip.show();
}

void runClock() {
  byte newSecond, newMinute, newHour, dayOfWeek, dayOfMonth, newMonth, newYear;
  readDS3231time(&newSecond, &newMinute, &newHour, &dayOfWeek, &dayOfMonth, &newMonth, &newYear);
  
  handleHour(newHour);                                             // HANDLE HOURS
  handleMinutes(newMinute);                                        // HANDLE MINUTES
  handleSeconds(newSecond);                                        // HANDLE SECONDS
  handlePulse(pulse);                                              // HANDLE PULSE
  strip.show();
  delay(delayTime);
}

void handleHour(int newHour) {
  if  (hours != newHour) {
    int hourHalfLen = hourLen / 2;
    for (int i = hourHalfLen * -1; i < hourHalfLen; i++) {
      strip.setPixelColor((((i + (hours * 10)) + 120) % 120) , 0);
    }
    hours = newHour;
    for (int i = hourHalfLen * -1; i < hourHalfLen; i++) {
      strip.setPixelColor((((i + (hours * 10)) + 120) % 120) , hourColor, hourColor, hourColor);
    }
  }
}

void handleMinutes(int newMinute) {
  /*
   * if minute points are on top of hour points
   * show hour
   */
  if  (minutes != newMinute) {

    int minuteHalfLen = minuteLen / 2;
    for (int i = minuteHalfLen * -1; i < minuteHalfLen; i++) {
      if (notAtHour((((i + (minutes * 2)) + 120) % 120))) {
        strip.setPixelColor((((i + (minutes * 2)) + 120) % 120) , 0);
      } else {
        strip.setPixelColor((((i + (minutes * 2)) + 120) % 120) , hourColor, hourColor, hourColor);
      }
    }
    minutes = newMinute;
    for (int i = minuteHalfLen * -1; i < minuteHalfLen; i++) {
      if (notAtHour((((i + (minutes * 2)) + 120) % 120))) {
        strip.setPixelColor((((i + (minutes * 2)) + 120) % 120) , minuteColor, minuteColor, minuteColor);
      }
    }
  }
}

void handleSeconds(int newSecond) {
  if (seconds != newSecond) {
    seconds = newSecond;
    pulse = true;
    digitalWrite(13, HIGH);
    //printTime();
  } else {
    digitalWrite(13, LOW);
  }
}

void handlePulse(bool p) {
  if (p) {
    //SHOW PULSE
    for (int i = secStart; i > secStart - secJump; i--) {
      if (i < activeLength) {
        if (notAtMinute(i) && notAtHour(i)) {
          strip.setPixelColor(i, 1, 1, 1);
        }
      }
    }
    //DELETE PULSE
    for (int i = secEnd; i <= secEnd + secJump; i++) {
      if (notAtMinute(i) && notAtHour(i)) {
        strip.setPixelColor(i, 0);
      }
    }
    secStart += secJump;
    secEnd += secJump;
    if (secEnd >= activeLength) {
      pulse = false;
      secStart = 0;
      secEnd = secLength;
    }
  }
}

bool notAtHour(int i) {
  int hourHalfLen = hourLen / 2;
  bool test = true;
  for (int j = hourHalfLen * -1; j < hourHalfLen; j++) {
    if (i == (((j + (hours * 10)) + 120) % 120)) {
      test = false;
    }
  }
  return test;
}

bool notAtMinute(int i) {
  int minuteHalfLen = minuteLen / 2;
  bool test = true;
  for (int j = minuteHalfLen * -1; j < minuteHalfLen; j++) {
    if (i == (((j + (minutes * 2)) + 120) % 120)) {
      test = false;
    }
  }
  return test;
}

//=============================
// B L E   F U N C T I O N S
//======================================================
void handleBle() {
  BTLEserial.pollACI();

  // Ask what is our current status
  aci_evt_opcode_t status = BTLEserial.getState();
  // If the status changed....
  if (status != laststatus) {
    // print it out!
    clearClock();
    if (status == ACI_EVT_DEVICE_STARTED) {
        Serial.println(F("* Advertising started"));
        CLOCK_STATE = 1;
    }
    if (status == ACI_EVT_CONNECTED) {
        Serial.println(F("* Connected!"));
        CLOCK_STATE = 2;
    }
    if (status == ACI_EVT_DISCONNECTED) {
        Serial.println(F("* Disconnected or advertising timed out"));
        CLOCK_STATE = 3;
    }
    // OK set the last status change to this one
    laststatus = status;
  }
  /*
  if (status == ACI_EVT_CONNECTED) {
    // Lets see if there's any data for us!
    if (BTLEserial.available()) {
      Serial.print("* "); Serial.print(BTLEserial.available()); Serial.println(F(" bytes available from BTLE"));
    }
    // OK while we still have something to read, get a character and print it out
    while (BTLEserial.available()) {
      char c = BTLEserial.read();
      Serial.print(c);
    }

    // Next up, see if we have any data to get from the Serial console

    if (Serial.available()) {
      // Read a line from Serial
      Serial.setTimeout(100); // 100 millisecond timeout
      String s = Serial.readString();

      // We need to convert the line to bytes, no more than 20 at this time
      uint8_t sendbuffer[20];
      s.getBytes(sendbuffer, 20);
      char sendbuffersize = min(20, s.length());

      Serial.print(F("\n* Sending -> \"")); Serial.print((char *)sendbuffer); Serial.println("\"");

      // write the data
      BTLEserial.write(sendbuffer, sendbuffersize);
    }
  }
  */
}

void getNewTime() {
  if (BTLEserial.available()) {
    Serial.print("* "); Serial.print(BTLEserial.available()); Serial.println(F(" bytes available from BTLE"));
  }
  // OK while we still have something to read, get a character and print it out
  int state = 0; // 0 = hour, 1 = minute, 2 = second
  String usrHour;
  String usrMinute;
  String usrSecond;
  while (BTLEserial.available()) {
    char c = BTLEserial.read();
    Serial.print(c);
    if (state == 0) {
      if (c == ':') {
        state = 1;
      } else {
        usrHour += c;
      }
    } else if (state == 1) {
      if (c == ':') {
        state = 2;
      } else {
        usrMinute += c;
      }
    } else if (state == 2) {
      if (c == ':') {
        state = 3;
      } else {
        usrSecond += c;
      }
    }
  }
  if (state == 3) {
    Serial.println("\nstate 3");
    Serial.println(usrHour.toInt());
    Serial.println(usrMinute.toInt());
    Serial.println(usrSecond.toInt());
    setDS3231time(usrSecond.toInt(), usrMinute.toInt(), usrHour.toInt(), 0, 0, 0, 0);
    //setDS3231time(55, 30, 6, 0, 0, 0, 0);     //debug
    state = 0;
  }
}

