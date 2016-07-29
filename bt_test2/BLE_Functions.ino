
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
