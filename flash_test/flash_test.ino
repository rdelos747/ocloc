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

const int hourLen                     = 2;
const int minuteLen                   = 8;

const int colorStep                   = 1;
const int colorMax                    = 70;
const int colorMin                    = 0;

int hourPoints[hourLen]               = {-1, -1};
int minutePoints[minuteLen]           = {-1, -1, -1, -1, -1, -1, -1, -1};
int secondPoint                       = -1;
int newHourPoints[hourLen];
int newMinutePoints[minuteLen];
int newSecondPoint;  

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
  setDS3231time(50, 58, 2, 0, 0, 0, 0);
}

//=============================
// M A I N   L O O P
//======================================================
byte newSecond, newMinute, newHour, dayOfWeek, dayOfMonth, newMonth, newYear;
int hours       = -1;
int minutes     = -1;
int seconds     = -1;
int pulse       = 0; //0 = no movement, 1 = go down, 2 = go up
int color       = colorMax;
void loop() {
  readDS3231time(&newSecond, &newMinute, &newHour, &dayOfWeek, &dayOfMonth, &newMonth, &newYear);

  // HOURS
  if (hours != newHour) {
    hours = newHour;
    int hourHalfLen = hourLen / 2;
    int j = hourHalfLen * -1;
    for (int i = 0; i < hourLen; i++) {
      newHourPoints[i] = (((j + (hours * 10)) + 120) % 120);
      j++;
    }
  }

  // MINUTES
  if (minutes != newMinute) {
    minutes = newMinute;
    int minuteHalfLen = minuteLen / 2;
    int j = minuteHalfLen * -1;
    for (int i = 0; i < minuteLen; i++) {
      newMinutePoints[i] = (((j + (minutes * 2)) + 120) % 120);
      j++;
    }
  }

  // SECONDS
  if (seconds != newSecond) {
    seconds = newSecond;
    newSecondPoint = seconds * 2;
    if (seconds % 2 == 0) {
      //Serial.println("here");
      pulse = 1;
    } else {
      pulse = 2;
    }
  }
  
  if (pulse == 1) {
    if (color > colorMin) {
      /*
      for (int i = 0; i < hourLen; i++) {
        strip.setPixelColor(hourPoints[i], color, color, color);
      }
      for (int i = 0; i < minuteLen; i++) {
        strip.setPixelColor(minutePoints[i], color, color, color);
      }
      strip.setPixelColor(secondPoint, color, color, color);
      */
      color -= colorStep;
    } else {
      for (int i = 0; i < hourLen; i++) {
        //strip.setPixelColor(hourPoints[i], 0, 0, 0);
        hourPoints[i] = newHourPoints[i];
      }
      for (int i = 0; i < minuteLen; i++) {
        //strip.setPixelColor(minutePoints[i], 0, 0, 0);
        minutePoints[i] = newMinutePoints[i];
      }
      //strip.setPixelColor(secondPoint, 0, 0, 0);
      secondPoint = newSecondPoint;
      color = colorMin;
      pulse = 0;
    }
  } else if (pulse == 2) {
    if (color < colorMax) {
      /*
      for (int i = 0; i < hourLen; i++) {
        strip.setPixelColor(hourPoints[i], color, color, color);
      }
      for (int i = 0; i < minuteLen; i++) {
        strip.setPixelColor(minutePoints[i], color, color, color);
      }
      strip.setPixelColor(secondPoint, color, color, color);
      */
      color += colorStep;
    } else {
      color = colorMax;
      pulse = 0;
    }
  }
  for (int i = 0; i < hourLen; i++) {
    strip.setPixelColor(hourPoints[i], color, color, color);
  }
  for (int i = 0; i < minuteLen; i++) {
    strip.setPixelColor(minutePoints[i], color, color, color);
  }
  strip.setPixelColor(secondPoint, color, color, color);
  //Serial.println(color);
  strip.show();
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
