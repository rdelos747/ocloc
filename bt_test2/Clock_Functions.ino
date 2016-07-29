int hours                 = -1; //ensure the led update always happens by setting -1
int minutes               = -1; //ensure the led update always happens by setting -1
int seconds               = -1; //ensure the led update always happens by setting -1

int hourColor             = 255;
int minuteColor           = 255;
int secondColor           = 100;

int activeLength          = 120;
int minuteLen             = 8;
int hourLen               = 2;

int secLength             = -30;
int secJump               = 5;
int delayTime             = 1;

bool pulse = false;
int secStart = 0;
int secEnd = secLength;

//STATE 1/ 2 VARS
int maxDelay = 5;
int currentDelay = 0;
int colorDir = 0;
int sColor = 0;
int rep = 3;

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
  
  colorDir = 0;
  currentDelay = 0;
  sColor = 0;
  rep = 3;

  hours = -1;
  minutes = -1;
  seconds = -1;
}

void clockStateZero() {
  strip.setPixelColor(119, 100, 100, 100);
  strip.setPixelColor(0, 100, 100, 100);
  strip.show();
}
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

void clockStateTwo() {
  /*
  for (int i = 0; i < 12; i++) {
    strip.setPixelColor(i * 10, 100, 100, 100);
  }
  */
  //Serial.println(currentDelay);
  if (rep > 0) {
    if (currentDelay > 0) {
      currentDelay--;
    } else {
      //Serial.println("==");
      //Serial.println(rep);
      currentDelay = maxDelay;
      if (colorDir == 0) {
        if (sColor < 100) {
          sColor += 50;
        } else {
          colorDir = 1;
        }
      } else {
        if (sColor > 0) {
          sColor -= 50;
        } else {
          colorDir = 0;
          rep--;
        }
      }
      //Serial.println(sColor);
      for (int i = 0; i < 12; i++) {
        strip.setPixelColor(i * 10, sColor);
      }
    }
  } else {
    clearClock();
    CLOCK_STATE = 3;
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
          strip.setPixelColor(i, secondColor, secondColor, secondColor);
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
