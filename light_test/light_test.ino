#include <Adafruit_DotStar.h>
#include <SPI.h> 
#include <Time.h>  

//=============================
// G L O B A L   V A R S
//======================================================
#define NUMPIXELS 144 
#define DATAPIN    4 //YELLOW
#define CLOCKPIN   5 //GREEN
Adafruit_DotStar strip = Adafruit_DotStar(
  NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);

uint32_t color            = 0xFFFFFF;
//int seconds               = 0;
//int minutes               = 0;
//int hours                 = 0;
int secondPoint           = 0;
int minutePoint           = 0;
int secondLength          = 1;
int minuteLength          = 2;

int activeLength          = 120;
int timeJump              = activeLength / 60;
int sanity                = 0; 

//=============================
// I N I T
//======================================================
void setup() {
  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
    clock_prescale_set(clock_div_1);                              // Enable 16 MHz on Trinket
  #endif
  strip.begin();                                                  // Initialize pins for output
  strip.show();                                                   // Turn all LEDs off ASAP
  pinMode(13, OUTPUT);                                            // Sanity check LED
  Serial.begin(9600);
  setTime(5, 5, 56, 0, 0, 0);
}

void loop() {
  
  
  //strip.setPixelColor(NUMPIXELS, 0);
  //strip.setPixelColor(1, color)
  
  
  
  // perform sanity check
  if (sanity == 0) {
     digitalWrite(13, HIGH);
     sanity = 1;
     getTime();
     //printTime();
     
     strip.show();
  } else {
    digitalWrite(13, LOW);
    sanity = 0;
  }

  // delay 500ms
  
  delay(500);
}

void printTime() {
  //Serial.print(hours);
  //Serial.print(" ");
  Serial.print(minute());
  Serial.print(" ");
  Serial.print(second());
  Serial.println("");
}

void getTime() {
  int newMinutePoint = minute() * timeJump;
  int newSecondPoint = second() * timeJump;
 
  if (minutePoint != newMinutePoint) {
    strip.setPixelColor(newMinutePoint, color);
    strip.setPixelColor(newMinutePoint + 1, color);
    strip.setPixelColor(minutePoint, 0);
    strip.setPixelColor(minutePoint + 1, 0);
    minutePoint = newMinutePoint;
  }

  if (secondPoint != newSecondPoint) {
    if (newSecondPoint != minutePoint && newSecondPoint != minutePoint +1) {
      strip.setPixelColor(newSecondPoint, color);
    }
    
    if (secondPoint != minutePoint && secondPoint != minutePoint + 1) {
      strip.setPixelColor(secondPoint, 0);
    }
    secondPoint = newSecondPoint;
  }
}

