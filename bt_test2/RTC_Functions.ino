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

