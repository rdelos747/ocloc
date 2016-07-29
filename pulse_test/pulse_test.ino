#include <Adafruit_DotStar.h>
#include <SPI.h> 
#include <Wire.h>
//=============================
// G L O B A L   V A R S
//======================================================
#define DS3231_I2C_ADDRESS 0x68
#define NUMPIXELS 120 
#define DATAPIN    4 //YELLOW
#define CLOCKPIN   5 //GREEN
Adafruit_DotStar strip = Adafruit_DotStar(
  NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);

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
  Serial.begin(9600);
  setDS3231time(55, 10, 6, 0, 0, 0, 0);
}

//=============================
// M A I N   L O O P
//======================================================
bool pulse = false;
int secStart = 0;
int secEnd = secLength;
int minutePoint = 0;
void loop() {
  
  byte newSecond, newMinute, newHour, dayOfWeek, dayOfMonth, newMonth, newYear;
  readDS3231time(&newSecond, &newMinute, &newHour, &dayOfWeek, &dayOfMonth, &newMonth, &newYear);
  
  handleHour(newHour);                                             // HANDLE HOURS
  handleMinutes(newMinute);                                        // HANDLE MINUTES
  handleSeconds(newSecond);                                        // HANDLE SECONDS
  handlePulse(pulse);                                             // HANDLE PULSE

  //printTime();
  strip.show();
  delay(delayTime);
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
// F U N C T I O N S
//======================================================
void printTime() {
  Serial.print(hours);
  Serial.print(" ");
  Serial.print(minutes);
  Serial.print(" ");
  Serial.print(seconds);
  Serial.println("");
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
      if (notAtHour(i + (minutes * 2))) {
        strip.setPixelColor((((i + (minutes * 2)) + 120) % 120) , 0);
      } else {
        strip.setPixelColor((((i + (minutes * 2)) + 120) % 120) , hourColor, hourColor, hourColor);
      }
    }
    minutes = newMinute;
    for (int i = minuteHalfLen * -1; i < minuteHalfLen; i++) {
      if (notAtHour(i + (minutes * 2))) {
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
